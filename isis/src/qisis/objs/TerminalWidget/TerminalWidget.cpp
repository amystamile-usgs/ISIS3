/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TerminalWidget.h"

#include <QDir>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>

namespace Isis {

  /**
   * Constructor
   */
  TerminalWidget::TerminalWidget(QWidget *parent)
      : QTextEdit(parent), m_promptPosition(0), m_historyIndex(-1), m_commandRunning(false) {

    setFont(QFont("Courier", 10));
    setWordWrapMode(QTextOption::WrapAnywhere);

    // Set initial working directory
    m_workingDirectory = QDir::homePath();

    // Create process
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDirectory);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalWidget::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalWidget::onProcessError);

    // Show initial prompt
    appendOutput("ISIS Workspace Terminal", "#4a9eff");
    appendOutput("Type commands below. Use Ctrl+C to cancel running command.\n", "#888");
    showPrompt();
  }


  /**
   * Destructor
   */
  TerminalWidget::~TerminalWidget() {
    if (m_process->state() == QProcess::Running) {
      m_process->kill();
      m_process->waitForFinished();
    }
  }


  /**
   * Append output text with optional color
   */
  void TerminalWidget::appendOutput(const QString &text, const QString &color) {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    if (!color.isEmpty()) {
      cursor.insertHtml(QString("<span style='color:%1;'>%2</span>")
                       .arg(color)
                       .arg(text.toHtmlEscaped()));
    } else {
      cursor.insertText(text);
    }

    cursor.insertText("\n");
    setTextCursor(cursor);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  }


  /**
   * Set working directory for command execution
   */
  void TerminalWidget::setWorkingDirectory(const QString &dir) {
    if (QDir(dir).exists()) {
      m_workingDirectory = dir;
      m_process->setWorkingDirectory(dir);
    }
  }


  /**
   * Get current working directory
   */
  QString TerminalWidget::workingDirectory() const {
    return m_workingDirectory;
  }


  /**
   * Show command prompt
   */
  void TerminalWidget::showPrompt() {
    if (m_commandRunning) {
      return;
    }

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    QString prompt = QString("[%1]$ ").arg(QDir(m_workingDirectory).dirName());
    cursor.insertHtml(QString("<span style='color:#4a9eff;font-weight:bold;'>%1</span>")
                     .arg(prompt.toHtmlEscaped()));

    setTextCursor(cursor);
    m_promptPosition = cursor.position();
  }


  /**
   * Get the command text after the prompt
   */
  QString TerminalWidget::getCurrentCommand() const {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.setPosition(m_promptPosition, QTextCursor::KeepAnchor);
    return cursor.selectedText();
  }


  /**
   * Handle key press events for terminal interaction
   */
  void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    // Don't allow editing before the prompt
    if (textCursor().position() < m_promptPosition &&
        event->key() != Qt::Key_C &&
        !event->matches(QKeySequence::Copy)) {
      return;
    }

    // Handle Ctrl+C
    if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
      if (m_commandRunning) {
        m_process->kill();
        appendOutput("^C", "#ff5555");
        m_commandRunning = false;
        showPrompt();
        return;
      }
    }

    // Handle Enter key
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      if (m_commandRunning) {
        return;
      }

      QString command = getCurrentCommand();

      // Move to end and add newline
      QTextCursor cursor = textCursor();
      cursor.movePosition(QTextCursor::End);
      cursor.insertText("\n");
      setTextCursor(cursor);

      if (!command.isEmpty()) {
        m_commandHistory.append(command);
        m_historyIndex = m_commandHistory.size();
        executeCommand(command);
        emit commandExecuted(command);
      } else {
        showPrompt();
      }
      return;
    }

    // Handle Up arrow - command history
    if (event->key() == Qt::Key_Up) {
      if (m_historyIndex > 0 && !m_commandHistory.isEmpty()) {
        m_historyIndex--;

        // Replace current command with history
        QTextCursor cursor = textCursor();
        cursor.setPosition(m_promptPosition);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(m_commandHistory[m_historyIndex]);
      }
      return;
    }

    // Handle Down arrow - command history
    if (event->key() == Qt::Key_Down) {
      if (m_historyIndex < m_commandHistory.size() - 1) {
        m_historyIndex++;

        QTextCursor cursor = textCursor();
        cursor.setPosition(m_promptPosition);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(m_commandHistory[m_historyIndex]);
      } else if (m_historyIndex == m_commandHistory.size() - 1) {
        m_historyIndex = m_commandHistory.size();

        QTextCursor cursor = textCursor();
        cursor.setPosition(m_promptPosition);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
      }
      return;
    }

    // Handle backspace - don't delete prompt
    if (event->key() == Qt::Key_Backspace) {
      if (textCursor().position() <= m_promptPosition) {
        return;
      }
    }

    // Handle Left arrow - don't go before prompt
    if (event->key() == Qt::Key_Left) {
      if (textCursor().position() <= m_promptPosition) {
        return;
      }
    }

    // Handle Home key - go to start of command
    if (event->key() == Qt::Key_Home) {
      QTextCursor cursor = textCursor();
      cursor.setPosition(m_promptPosition);
      setTextCursor(cursor);
      return;
    }

    // Default handling for other keys
    QTextEdit::keyPressEvent(event);
  }


  /**
   * Execute a shell command
   */
  void TerminalWidget::executeCommand(const QString &command) {
    if (m_commandRunning) {
      appendOutput("Command already running. Use Ctrl+C to cancel.", "#ff5555");
      return;
    }

    QString cmd = command.trimmed();

    // Handle built-in cd command
    if (cmd.startsWith("cd ")) {
      QString newDir = cmd.mid(3).trimmed();

      // Handle ~ and relative paths
      if (newDir.startsWith("~")) {
        newDir = QDir::homePath() + newDir.mid(1);
      } else if (!newDir.startsWith("/")) {
        newDir = m_workingDirectory + "/" + newDir;
      }

      QDir dir(newDir);
      if (dir.exists()) {
        m_workingDirectory = dir.absolutePath();
        m_process->setWorkingDirectory(m_workingDirectory);
      } else {
        appendOutput(QString("cd: no such directory: %1").arg(newDir), "#ff5555");
      }
      showPrompt();
      return;
    }

    // Handle pwd command
    if (cmd == "pwd") {
      appendOutput(m_workingDirectory, "#d4d4d4");
      showPrompt();
      return;
    }

    // Handle clear command
    if (cmd == "clear") {
      clear();
      showPrompt();
      return;
    }

    // Execute external command
    m_commandRunning = true;

    // Use shell to execute command for proper environment handling
#ifdef Q_OS_WIN
    m_process->start("cmd.exe", QStringList() << "/c" << cmd);
#else
    m_process->start("/bin/bash", QStringList() << "-c" << cmd);
#endif
  }


  /**
   * Handle process finished
   */
  void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_commandRunning = false;

    if (exitStatus == QProcess::CrashExit) {
      appendOutput("Process crashed", "#ff5555");
    } else if (exitCode != 0) {
      appendOutput(QString("Exit code: %1").arg(exitCode), "#ffaa00");
    }

    showPrompt();
  }


  /**
   * Handle process standard output
   */
  void TerminalWidget::onProcessOutput() {
    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!output.isEmpty()) {
      QTextCursor cursor = textCursor();
      cursor.movePosition(QTextCursor::End);
      cursor.insertText(output);
      setTextCursor(cursor);
      verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
  }


  /**
   * Handle process error output
   */
  void TerminalWidget::onProcessError() {
    QString error = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!error.isEmpty()) {
      appendOutput(error.trimmed(), "#ff5555");
    }
  }

}
