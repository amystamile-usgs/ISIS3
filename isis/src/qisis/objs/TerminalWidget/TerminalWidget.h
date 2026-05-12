#ifndef TerminalWidget_h
#define TerminalWidget_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QTextEdit>
#include <QProcess>

namespace Isis {

  /**
   * @brief Interactive terminal widget
   *
   * Provides an interactive terminal that can execute shell commands
   * and display output. Supports command history and basic terminal features.
   *
   * @author 2026-05-12 Amy Stamile
   */
  class TerminalWidget : public QTextEdit {
    Q_OBJECT

  public:
    TerminalWidget(QWidget *parent = nullptr);
    virtual ~TerminalWidget();

    void appendOutput(const QString &text, const QString &color = "#d4d4d4");
    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;

  signals:
    void commandExecuted(const QString &command);

  protected:
    void keyPressEvent(QKeyEvent *event) override;

  private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessOutput();
    void onProcessError();

  private:
    void executeCommand(const QString &command);
    void showPrompt();
    QString getCurrentCommand() const;
    void insertPrompt();

    QProcess *m_process;
    QString m_workingDirectory;
    int m_promptPosition;
    QStringList m_commandHistory;
    int m_historyIndex;
    bool m_commandRunning;
  };
}

#endif
