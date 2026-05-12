/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IsisWorkspaceMainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "ControlNetPanel.h"
#include "CubeViewPanel.h"
#include "FootprintPanel.h"
#include "TerminalWidget.h"
#include "WorkspaceProject.h"

namespace Isis {

  /**
   * Constructor - creates the main window with tab-based layout
   */
  IsisWorkspaceMainWindow::IsisWorkspaceMainWindow(QWidget *parent)
      : QMainWindow(parent), m_isDarkTheme(true) {

    setWindowTitle("ISIS Workspace");
    resize(1600, 1000);

    // Create the project
    m_project = new WorkspaceProject(this);

    // Setup UI components
    createActions();
    createMenus();
    createToolBars();
    createMainLayout();
    setupConnections();

    // Load settings and apply theme
    loadSettings();
    applyTheme(m_isDarkTheme ? "dark" : "light");
  }


  /**
   * Destructor
   */
  IsisWorkspaceMainWindow::~IsisWorkspaceMainWindow() {
    saveSettings();
  }


  /**
   * Create all actions for menus and toolbars
   */
  void IsisWorkspaceMainWindow::createActions() {
    // File actions with unicode icons
    m_openCubesAction = new QAction(tr("Open Cubes..."), this);
    m_openCubesAction->setShortcut(QKeySequence::Open);
    m_openCubesAction->setStatusTip(tr("Open one or more ISIS cube files"));
    m_openCubesAction->setProperty("icon", "▣");
    connect(m_openCubesAction, &QAction::triggered, this, &IsisWorkspaceMainWindow::onOpenCubes);

    m_saveProjectAction = new QAction(tr("Save Project"), this);
    m_saveProjectAction->setShortcut(QKeySequence::Save);
    m_saveProjectAction->setStatusTip(tr("Save current workspace project"));
    m_saveProjectAction->setProperty("icon", "💾");
    connect(m_saveProjectAction, &QAction::triggered, this, &IsisWorkspaceMainWindow::onSaveProject);

    m_openProjectAction = new QAction(tr("Open Project..."), this);
    m_openProjectAction->setShortcut(tr("Ctrl+Shift+O"));
    m_openProjectAction->setStatusTip(tr("Open an existing workspace project"));
    m_openProjectAction->setProperty("icon", "📂");
    connect(m_openProjectAction, &QAction::triggered, this, &IsisWorkspaceMainWindow::onOpenProject);

    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &QMainWindow::close);

    // View actions
    m_toggleThemeAction = new QAction(tr("Toggle Theme"), this);
    m_toggleThemeAction->setShortcut(tr("Ctrl+T"));
    m_toggleThemeAction->setStatusTip(tr("Toggle between dark and light theme"));
    m_toggleThemeAction->setProperty("icon", "◐");
    connect(m_toggleThemeAction, &QAction::triggered, this, &IsisWorkspaceMainWindow::onToggleTheme);

    // Help actions
    m_aboutAction = new QAction(tr("About"), this);
    m_aboutAction->setStatusTip(tr("Show application information"));
    m_aboutAction->setProperty("icon", "ⓘ");
    connect(m_aboutAction, &QAction::triggered, this, &IsisWorkspaceMainWindow::onAbout);
  }


  /**
   * Create the menu bar
   */
  void IsisWorkspaceMainWindow::createMenus() {
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openCubesAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_openProjectAction);
    m_fileMenu->addAction(m_saveProjectAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_toggleThemeAction);

    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
  }


  /**
   * Create toolbars
   */
  void IsisWorkspaceMainWindow::createToolBars() {
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->addAction(m_openCubesAction);
    m_mainToolBar->addAction(m_saveProjectAction);
    m_mainToolBar->addAction(m_openProjectAction);
  }


  /**
   * Create the main layout with dockable panels
   */
  void IsisWorkspaceMainWindow::createMainLayout() {
    // Enable nested docking for complex layouts
    setDockNestingEnabled(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    // Set dock options to prefer splitting over tabbing
    setDockOptions(QMainWindow::AnimatedDocks |
                   QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks);

    // Create central widget with welcome message
    m_centralWidget = new QWidget(this);
    m_centralWidget->setMinimumSize(400, 300);
    m_centralWidget->setStyleSheet("background-color: #1e1e1e;");

    QVBoxLayout *layout = new QVBoxLayout(m_centralWidget);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("ISIS Workspace");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
      "color: #4cafb4; "
      "font-size: 32pt; "
      "font-weight: bold; "
      "padding: 20px;"
    );
    layout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("Integrated Scientific Imaging System");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
      "color: #888; "
      "font-size: 14pt; "
      "padding-bottom: 30px;"
    );
    layout->addWidget(subtitleLabel);

    QLabel *instructionLabel = new QLabel(
      "⚡ Select an application from the left panel to begin\n\n"
      "📁 Open cubes: File → Open Cubes\n"
      "💾 Save/load projects: File → Save/Open Project"
    );
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet(
      "color: #aaa; "
      "font-size: 12pt; "
      "line-height: 1.8;"
    );
    layout->addWidget(instructionLabel);

    setCentralWidget(m_centralWidget);

    // Create launcher dock
    createLauncherPanel();
    m_launcherDock = new QDockWidget("Applications", this);
    m_launcherDock->setWidget(m_launcherPanel);
    m_launcherDock->setFeatures(QDockWidget::NoDockWidgetFeatures);  // Fixed, can't close or move
    m_launcherDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_launcherDock);

    // Create terminal dock
    m_terminal = new TerminalWidget();
    QString isisRoot = qEnvironmentVariable("ISISROOT");
    if (!isisRoot.isEmpty()) {
      m_terminal->setWorkingDirectory(isisRoot);
    }
    connect(m_terminal, &TerminalWidget::commandExecuted,
            this, &IsisWorkspaceMainWindow::onTerminalCommand);

    m_terminalDock = new QDockWidget("Terminal", this);
    m_terminalDock->setWidget(m_terminal);
    m_terminalDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
  }


  /**
   * Create the launcher panel with file explorer and app buttons
   */
  void IsisWorkspaceMainWindow::createLauncherPanel() {
    m_launcherPanel = new QWidget();
    m_launcherPanel->setMinimumWidth(180);
    m_launcherPanel->setMaximumWidth(250);

    QVBoxLayout *layout = new QVBoxLayout(m_launcherPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // File Explorer section
    QLabel *explorerLabel = new QLabel("FILE EXPLORER");
    explorerLabel->setStyleSheet(
      "font-weight: bold; "
      "font-size: 10pt; "
      "padding: 12px 10px 8px 10px; "
      "color: #aaa; "
      "background-color: #1e1e1e;"
    );
    layout->addWidget(explorerLabel);

    // Simple file tree (placeholder)
    QWidget *fileTree = new QWidget();
    fileTree->setMinimumHeight(200);
    fileTree->setStyleSheet("background-color: #252526; border: none;");
    QVBoxLayout *treeLayout = new QVBoxLayout(fileTree);
    treeLayout->setContentsMargins(10, 8, 10, 8);
    treeLayout->setSpacing(4);

    // Project folder
    QWidget *projectItem = new QWidget();
    QHBoxLayout *projectLayout = new QHBoxLayout(projectItem);
    projectLayout->setContentsMargins(4, 4, 4, 4);
    projectLayout->setSpacing(8);

    QLabel *folderIcon = new QLabel("▸");
    folderIcon->setStyleSheet("color: #888; font-size: 10pt;");
    projectLayout->addWidget(folderIcon);

    QLabel *folderLabel = new QLabel("●");
    folderLabel->setStyleSheet("color: #4cafb4; font-size: 12pt;");
    projectLayout->addWidget(folderLabel);

    QLabel *projectLabel = new QLabel("Project");
    projectLabel->setStyleSheet("color: #cccccc; font-size: 10pt; font-weight: 500;");
    projectLayout->addWidget(projectLabel);
    projectLayout->addStretch();
    treeLayout->addWidget(projectItem);

    // No cubes message
    QWidget *noCubesItem = new QWidget();
    QHBoxLayout *noCubesLayout = new QHBoxLayout(noCubesItem);
    noCubesLayout->setContentsMargins(30, 2, 4, 2);

    QLabel *cubesLabel = new QLabel("No cubes loaded");
    cubesLabel->setStyleSheet("color: #6e6e6e; font-size: 10pt; font-style: italic;");
    noCubesLayout->addWidget(cubesLabel);
    noCubesLayout->addStretch();
    treeLayout->addWidget(noCubesItem);

    treeLayout->addStretch();
    layout->addWidget(fileTree);

    // Separator
    QWidget *separator = new QWidget();
    separator->setFixedHeight(1);
    separator->setStyleSheet("background-color: #3f3f46;");
    layout->addWidget(separator);

    // Applications section
    QLabel *appsLabel = new QLabel("APPLICATIONS");
    appsLabel->setStyleSheet(
      "font-weight: bold; "
      "font-size: 10pt; "
      "padding: 12px 10px 8px 10px; "
      "color: #aaa; "
      "background-color: #1e1e1e;"
    );
    layout->addWidget(appsLabel);

    QWidget *appsContainer = new QWidget();
    appsContainer->setStyleSheet("background-color: #252526;");
    QVBoxLayout *appsLayout = new QVBoxLayout(appsContainer);
    appsLayout->setContentsMargins(8, 8, 8, 8);
    appsLayout->setSpacing(6);

    // QView button with Material Design-style layout
    QWidget *qviewWidget = new QWidget();
    QHBoxLayout *qviewLayout = new QHBoxLayout(qviewWidget);
    qviewLayout->setContentsMargins(12, 0, 12, 0);
    qviewLayout->setSpacing(12);

    QLabel *qviewIcon = new QLabel("◉");
    qviewIcon->setStyleSheet("font-size: 18pt; color: #888;");
    qviewLayout->addWidget(qviewIcon);

    QLabel *qviewLabel = new QLabel("QView");
    qviewLabel->setStyleSheet("font-size: 11pt; font-weight: 500; color: #cccccc;");
    qviewLayout->addWidget(qviewLabel);
    qviewLayout->addStretch();

    m_qviewButton = new QPushButton();
    m_qviewButton->setCheckable(true);
    m_qviewButton->setMinimumHeight(44);
    m_qviewButton->setLayout(qviewLayout);
    m_qviewButton->setStyleSheet(
      "QPushButton { "
      "  text-align: left; "
      "  border-radius: 6px; "
      "  background-color: transparent; "
      "  border: none; "
      "}"
    );
    connect(m_qviewButton, &QPushButton::clicked, this, &IsisWorkspaceMainWindow::onToggleQView);
    appsLayout->addWidget(m_qviewButton);

    // QNet button with Material Design-style layout
    QWidget *qnetWidget = new QWidget();
    QHBoxLayout *qnetLayout = new QHBoxLayout(qnetWidget);
    qnetLayout->setContentsMargins(12, 0, 12, 0);
    qnetLayout->setSpacing(12);

    QLabel *qnetIcon = new QLabel("⬡");
    qnetIcon->setStyleSheet("font-size: 18pt; color: #888;");
    qnetLayout->addWidget(qnetIcon);

    QLabel *qnetLabel = new QLabel("QNet");
    qnetLabel->setStyleSheet("font-size: 11pt; font-weight: 500; color: #cccccc;");
    qnetLayout->addWidget(qnetLabel);
    qnetLayout->addStretch();

    m_qnetButton = new QPushButton();
    m_qnetButton->setCheckable(true);
    m_qnetButton->setMinimumHeight(44);
    m_qnetButton->setLayout(qnetLayout);
    m_qnetButton->setStyleSheet(
      "QPushButton { "
      "  text-align: left; "
      "  border-radius: 6px; "
      "  background-color: transparent; "
      "  border: none; "
      "}"
    );
    connect(m_qnetButton, &QPushButton::clicked, this, &IsisWorkspaceMainWindow::onToggleQNet);
    appsLayout->addWidget(m_qnetButton);

    // QMos button with Material Design-style layout
    QWidget *qmosWidget = new QWidget();
    QHBoxLayout *qmosLayout = new QHBoxLayout(qmosWidget);
    qmosLayout->setContentsMargins(12, 0, 12, 0);
    qmosLayout->setSpacing(12);

    QLabel *qmosIcon = new QLabel("▦");
    qmosIcon->setStyleSheet("font-size: 18pt; color: #888;");
    qmosLayout->addWidget(qmosIcon);

    QLabel *qmosLabel = new QLabel("QMos");
    qmosLabel->setStyleSheet("font-size: 11pt; font-weight: 500; color: #cccccc;");
    qmosLayout->addWidget(qmosLabel);
    qmosLayout->addStretch();

    m_qmosButton = new QPushButton();
    m_qmosButton->setCheckable(true);
    m_qmosButton->setMinimumHeight(44);
    m_qmosButton->setLayout(qmosLayout);
    m_qmosButton->setStyleSheet(
      "QPushButton { "
      "  text-align: left; "
      "  border-radius: 6px; "
      "  background-color: transparent; "
      "  border: none; "
      "}"
    );
    connect(m_qmosButton, &QPushButton::clicked, this, &IsisWorkspaceMainWindow::onToggleQMos);
    appsLayout->addWidget(m_qmosButton);

    appsLayout->addStretch();
    layout->addWidget(appsContainer);
  }


  /**
   * Setup signal/slot connections between components
   */
  void IsisWorkspaceMainWindow::setupConnections() {
    // Connections will be made when panels are created
  }


  /**
   * Update central widget visibility based on open panels
   */
  void IsisWorkspaceMainWindow::updateCentralWidget() {
    if (m_openPanels.isEmpty()) {
      // No panels open - show welcome screen
      m_centralWidget->show();
    }
    else {
      // Panels open - hide welcome screen to give full space to docks
      m_centralWidget->hide();
    }
  }


  /**
   * Create an app panel widget
   */
  QWidget* IsisWorkspaceMainWindow::createAppPanel(const QString &appName) {
    QWidget *panel = nullptr;

    if (appName == "QView") {
      CubeViewPanel *cubePanel = new CubeViewPanel(m_project);
      connect(m_project, &WorkspaceProject::cubesChanged,
              cubePanel, &CubeViewPanel::onCubesChanged);
      panel = cubePanel;
    }
    else if (appName == "QNet") {
      ControlNetPanel *cnetPanel = new ControlNetPanel(m_project);
      connect(m_project, &WorkspaceProject::controlNetworkChanged,
              cnetPanel, &ControlNetPanel::onControlNetworkChanged);
      panel = cnetPanel;
    }
    else if (appName == "QMos") {
      FootprintPanel *footprintPanel = new FootprintPanel(m_project);
      connect(m_project, &WorkspaceProject::cubesChanged,
              footprintPanel, &FootprintPanel::onCubesChanged);
      connect(m_project, &WorkspaceProject::controlNetworkChanged,
              footprintPanel, &FootprintPanel::onControlNetworkChanged);
      panel = footprintPanel;
    }

    return panel;
  }


  /**
   * Load a cube file into the workspace
   */
  void IsisWorkspaceMainWindow::loadCube(const QString &cubePath) {
    try {
      m_project->addCube(cubePath);
      m_terminal->appendOutput(QString("Loaded cube: %1").arg(cubePath), "#88ff88");
    }
    catch (std::exception &e) {
      QMessageBox::critical(this, "Error Loading Cube", e.what());
      m_terminal->appendOutput(QString("ERROR: Failed to load %1").arg(cubePath), "#ff5555");
    }
  }


  /**
   * Handle open cubes action
   */
  void IsisWorkspaceMainWindow::onOpenCubes() {
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        tr("Open ISIS Cubes"),
        QString(),
        tr("ISIS Cubes (*.cub);;All Files (*)"));

    if (!fileNames.isEmpty()) {
      for (const QString &fileName : fileNames) {
        loadCube(fileName);
      }
    }
  }


  /**
   * Handle save project action
   */
  void IsisWorkspaceMainWindow::onSaveProject() {
    QString fileName = m_project->projectPath();

    if (fileName.isEmpty()) {
      fileName = QFileDialog::getSaveFileName(
          this,
          tr("Save Project"),
          QString(),
          tr("ISIS Workspace Projects (*.iwsp)"));
    }

    if (!fileName.isEmpty()) {
      try {
        m_project->save(fileName);
        m_terminal->appendOutput(QString("Project saved: %1").arg(fileName), "#88ff88");
      }
      catch (std::exception &e) {
        QMessageBox::critical(this, "Error Saving Project", e.what());
        m_terminal->appendOutput("ERROR: Failed to save project", "#ff5555");
      }
    }
  }


  /**
   * Handle open project action
   */
  void IsisWorkspaceMainWindow::onOpenProject() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Project"),
        QString(),
        tr("ISIS Workspace Projects (*.iwsp)"));

    if (!fileName.isEmpty()) {
      try {
        m_project->load(fileName);
        m_terminal->appendOutput(QString("Project loaded: %1").arg(fileName), "#88ff88");
      }
      catch (std::exception &e) {
        QMessageBox::critical(this, "Error Loading Project", e.what());
        m_terminal->appendOutput("ERROR: Failed to load project", "#ff5555");
      }
    }
  }


  /**
   * Handle toggle theme action
   */
  void IsisWorkspaceMainWindow::onToggleTheme() {
    m_isDarkTheme = !m_isDarkTheme;
    applyTheme(m_isDarkTheme ? "dark" : "light");
    m_terminal->appendOutput(QString("Theme changed to: %1").arg(m_isDarkTheme ? "dark" : "light"), "#4a9eff");
  }


  /**
   * Handle toggle icon style action
   */


  /**
   * Handle about action
   */
  void IsisWorkspaceMainWindow::onAbout() {
    QMessageBox::about(this, tr("About ISIS Workspace"),
                      tr("<h3>ISIS Workspace</h3>"
                         "<p>A modern integrated interface for ISIS cube analysis.</p>"
                         "<p>Combines functionality from qview, qnet, cneteditor, and qmos.</p>"
                         "<p>Built with Qt6 and ISIS.</p>"));
  }


  /**
   * Handle terminal command execution
   */
  void IsisWorkspaceMainWindow::onTerminalCommand(const QString &command) {
    // Log command execution - could add special handling for ISIS commands here
    (void)command; // Suppress unused warning for now
  }


  /**
   * Toggle QView panel
   */
  void IsisWorkspaceMainWindow::onToggleQView() {
    QString appName = "QView";

    if (m_openPanels.contains(appName)) {
      // Close the panel
      QDockWidget *dock = m_openPanels[appName];
      removeDockWidget(dock);
      dock->deleteLater();
      m_openPanels.remove(appName);
      m_qviewButton->setChecked(false);
      m_terminal->appendOutput(QString("Closed %1").arg(appName), "#ffaa00");
      updateCentralWidget();
    }
    else {
      // Open the panel
      QWidget *panel = createAppPanel(appName);
      if (panel) {
        QDockWidget *dock = new QDockWidget(appName, this);
        dock->setWidget(panel);
        dock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);

        // Add to right side - will fill the space
        addDockWidget(Qt::RightDockWidgetArea, dock);

        m_openPanels[appName] = dock;
        m_qviewButton->setChecked(true);
        m_terminal->appendOutput(QString("Opened %1 (Double-click title to fullscreen)").arg(appName), "#88ff88");

        // Connect close signal to update button state and central widget
        connect(dock, &QDockWidget::destroyed, this, [this, appName]() {
          m_openPanels.remove(appName);
          m_qviewButton->setChecked(false);
          updateCentralWidget();
        });

        updateCentralWidget();
      }
    }
  }


  /**
   * Toggle QNet panel
   */
  void IsisWorkspaceMainWindow::onToggleQNet() {
    QString appName = "QNet";

    if (m_openPanels.contains(appName)) {
      QDockWidget *dock = m_openPanels[appName];
      removeDockWidget(dock);
      dock->deleteLater();
      m_openPanels.remove(appName);
      m_qnetButton->setChecked(false);
      m_terminal->appendOutput(QString("Closed %1").arg(appName), "#ffaa00");
      updateCentralWidget();
    }
    else {
      QWidget *panel = createAppPanel(appName);
      if (panel) {
        QDockWidget *dock = new QDockWidget(appName, this);
        dock->setWidget(panel);
        dock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);

        // If QView is open, split below it; otherwise add to right
        if (m_openPanels.contains("QView")) {
          addDockWidget(Qt::BottomDockWidgetArea, dock);
          splitDockWidget(m_openPanels["QView"], dock, Qt::Vertical);
        }
        else {
          addDockWidget(Qt::RightDockWidgetArea, dock);
        }

        m_openPanels[appName] = dock;
        m_qnetButton->setChecked(true);
        m_terminal->appendOutput(QString("Opened %1 (Drag to reposition)").arg(appName), "#88ff88");

        connect(dock, &QDockWidget::destroyed, this, [this, appName]() {
          m_openPanels.remove(appName);
          m_qnetButton->setChecked(false);
          updateCentralWidget();
        });

        updateCentralWidget();
      }
    }
  }


  /**
   * Toggle QMos panel
   */
  void IsisWorkspaceMainWindow::onToggleQMos() {
    QString appName = "QMos";

    if (m_openPanels.contains(appName)) {
      QDockWidget *dock = m_openPanels[appName];
      removeDockWidget(dock);
      dock->deleteLater();
      m_openPanels.remove(appName);
      m_qmosButton->setChecked(false);
      m_terminal->appendOutput(QString("Closed %1").arg(appName), "#ffaa00");
      updateCentralWidget();
    }
    else {
      QWidget *panel = createAppPanel(appName);
      if (panel) {
        QDockWidget *dock = new QDockWidget(appName, this);
        dock->setWidget(panel);
        dock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);

        // Smart layout: create a grid
        if (m_openPanels.contains("QView") && m_openPanels.contains("QNet")) {
          // If both QView and QNet exist, split horizontally next to QView
          addDockWidget(Qt::RightDockWidgetArea, dock);
          splitDockWidget(m_openPanels["QView"], dock, Qt::Horizontal);
        }
        else if (m_openPanels.contains("QNet")) {
          // Split horizontally with QNet
          addDockWidget(Qt::RightDockWidgetArea, dock);
          splitDockWidget(m_openPanels["QNet"], dock, Qt::Horizontal);
        }
        else if (m_openPanels.contains("QView")) {
          // Split horizontally with QView
          addDockWidget(Qt::RightDockWidgetArea, dock);
          splitDockWidget(m_openPanels["QView"], dock, Qt::Horizontal);
        }
        else {
          addDockWidget(Qt::RightDockWidgetArea, dock);
        }

        m_openPanels[appName] = dock;
        m_qmosButton->setChecked(true);
        m_terminal->appendOutput(QString("Opened %1 (Drag to reposition)").arg(appName), "#88ff88");

        connect(dock, &QDockWidget::destroyed, this, [this, appName]() {
          m_openPanels.remove(appName);
          m_qmosButton->setChecked(false);
          updateCentralWidget();
        });

        updateCentralWidget();
      }
    }
  }


  /**
   * Apply theme styling
   */
  void IsisWorkspaceMainWindow::applyTheme(const QString &themeName) {
    m_currentTheme = themeName;

    if (themeName == "dark") {
      // Knoten-inspired dark theme
      QPalette darkPalette;
      darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));          // #1e1e1e
      darkPalette.setColor(QPalette::WindowText, QColor(204, 204, 204));   // #cccccc
      darkPalette.setColor(QPalette::Base, QColor(37, 37, 38));            // #252526
      darkPalette.setColor(QPalette::AlternateBase, QColor(42, 42, 43));   // #2a2a2b
      darkPalette.setColor(QPalette::ToolTipBase, QColor(37, 37, 38));
      darkPalette.setColor(QPalette::ToolTipText, QColor(204, 204, 204));
      darkPalette.setColor(QPalette::Text, QColor(204, 204, 204));         // #cccccc
      darkPalette.setColor(QPalette::Button, QColor(37, 37, 38));
      darkPalette.setColor(QPalette::ButtonText, QColor(204, 204, 204));
      darkPalette.setColor(QPalette::BrightText, QColor(255, 85, 85));     // Error red
      darkPalette.setColor(QPalette::Link, QColor(76, 175, 180));          // Teal #4cafb4
      darkPalette.setColor(QPalette::Highlight, QColor(76, 175, 180));     // Teal highlight
      darkPalette.setColor(QPalette::HighlightedText, Qt::black);

      QApplication::setPalette(darkPalette);

      setStyleSheet(R"(
        QMainWindow {
          background-color: #1e1e1e;
        }
        QMenuBar {
          background-color: #2d2d30;
          color: #cccccc;
          padding: 6px;
          border-bottom: 1px solid #3f3f46;
        }
        QMenuBar::item {
          padding: 6px 12px;
          border-radius: 4px;
        }
        QMenuBar::item:selected {
          background-color: #3e3e42;
        }
        QMenu {
          background-color: #2d2d30;
          color: #cccccc;
          border: 1px solid #3f3f46;
          padding: 4px;
        }
        QMenu::item {
          padding: 6px 24px 6px 12px;
          border-radius: 3px;
        }
        QMenu::item:selected {
          background-color: #4cafb4;
          color: #1e1e1e;
        }
        QToolBar {
          background-color: #2d2d30;
          border: none;
          border-bottom: 1px solid #3f3f46;
          spacing: 2px;
          padding: 4px 8px;
        }
        QToolButton {
          background-color: transparent;
          border: 1px solid transparent;
          border-radius: 6px;
          padding: 6px;
          margin: 2px;
          color: #cccccc;
        }
        QToolButton:hover {
          background-color: #3e3e42;
          border: 1px solid #4cafb4;
        }
        QToolButton:pressed {
          background-color: #4cafb4;
          border: 1px solid #5fc0c5;
        }
        QToolButton:checked {
          background-color: rgba(76, 175, 180, 0.2);
          border: 1px solid #4cafb4;
        }
        QToolButton::menu-indicator {
          image: none;
          subcontrol-position: right center;
          subcontrol-origin: padding;
          left: -2px;
        }
        QToolButton[popupMode="1"] {
          padding-right: 20px;
        }
        QPushButton {
          background-color: transparent;
          border: none;
          border-radius: 6px;
          padding: 0px;
          color: #cccccc;
        }
        QPushButton:hover {
          background-color: #3e3e42;
        }
        QPushButton:pressed {
          background-color: #2a2a2d;
        }
        QPushButton:checked {
          background-color: rgba(76, 175, 180, 0.15);
          border-left: 3px solid #4cafb4;
          border-radius: 6px;
        }
        QDockWidget {
          border: 1px solid #3f3f46;
          titlebar-close-icon: url(none);
          titlebar-normal-icon: url(none);
        }
        QDockWidget::title {
          background-color: #2d2d30;
          padding: 8px;
          border-bottom: 1px solid #3f3f46;
          color: #cccccc;
          font-weight: bold;
          text-transform: uppercase;
          font-size: 10pt;
        }
        QDockWidget::close-button, QDockWidget::float-button {
          background-color: transparent;
          border: none;
          padding: 4px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
          background-color: #3e3e42;
          border-radius: 3px;
        }
        QTextEdit {
          background-color: #1e1e1e;
          color: #d4d4d4;
          border: 1px solid #3f3f46;
          font-family: 'Monaco', 'Menlo', 'Consolas', monospace;
          selection-background-color: #4cafb4;
          selection-color: #1e1e1e;
        }
        QSplitter::handle {
          background-color: #3f3f46;
          width: 1px;
          height: 1px;
        }
        QSplitter::handle:hover {
          background-color: #4cafb4;
        }
        QLabel {
          color: #cccccc;
        }
        QTabBar::tab {
          background-color: #2d2d30;
          color: #969696;
          padding: 10px 20px;
          border: none;
          border-top: 3px solid transparent;
          font-weight: 500;
        }
        QTabBar::tab:selected {
          background-color: #1e1e1e;
          color: #cccccc;
          border-top: 3px solid #4cafb4;
        }
        QTabBar::tab:hover:!selected {
          background-color: #3e3e42;
          color: #cccccc;
        }
        QStatusBar {
          background-color: #2d2d30;
          color: #888;
          border-top: 1px solid #3f3f46;
        }
        QScrollBar:vertical {
          background-color: #1e1e1e;
          width: 12px;
          border: none;
        }
        QScrollBar::handle:vertical {
          background-color: #3e3e42;
          min-height: 30px;
          border-radius: 6px;
          margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
          background-color: #4cafb4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
          height: 0px;
        }
        QScrollBar:horizontal {
          background-color: #1e1e1e;
          height: 12px;
          border: none;
        }
        QScrollBar::handle:horizontal {
          background-color: #3e3e42;
          min-width: 30px;
          border-radius: 6px;
          margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
          background-color: #4cafb4;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
          width: 0px;
        }
        QMdiArea {
          background-color: #1e1e1e;
        }
        QMdiSubWindow {
          background-color: #252526;
          border: 1px solid #3f3f46;
        }
        QMdiSubWindow::title {
          background-color: #2d2d30;
          padding: 6px;
        }
      )");
    }
    else {
      // Light theme (QISIS-style)
      QPalette lightPalette;
      lightPalette.setColor(QPalette::Window, QColor(200, 200, 200));
      lightPalette.setColor(QPalette::WindowText, Qt::black);
      lightPalette.setColor(QPalette::Base, QColor(240, 240, 240));
      lightPalette.setColor(QPalette::AlternateBase, QColor(233, 233, 233));
      lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
      lightPalette.setColor(QPalette::ToolTipText, Qt::black);
      lightPalette.setColor(QPalette::Text, Qt::black);
      lightPalette.setColor(QPalette::Button, QColor(200, 200, 200));
      lightPalette.setColor(QPalette::ButtonText, Qt::black);
      lightPalette.setColor(QPalette::BrightText, Qt::red);
      lightPalette.setColor(QPalette::Link, QColor(42, 130, 218));
      lightPalette.setColor(QPalette::Highlight, QColor(157, 180, 200));
      lightPalette.setColor(QPalette::HighlightedText, Qt::white);

      QApplication::setPalette(lightPalette);

      setStyleSheet(R"(
        QMainWindow {
          background-color: #c8c8c8;
        }
        QMenuBar {
          background-color: #d0d0d0;
          color: black;
          padding: 4px;
        }
        QMenuBar::item:selected {
          background-color: #9db4c8;
        }
        QMenu {
          background-color: #e8e8e8;
          color: black;
          border: 1px solid #a0a0a0;
        }
        QMenu::item:selected {
          background-color: #9db4c8;
        }
        QToolBar {
          background-color: #d0d0d0;
          border: none;
          spacing: 3px;
          padding: 4px;
        }
        QToolButton {
          background-color: #9db4c8;
          border: 1px solid #7a95a8;
          border-radius: 3px;
          padding: 5px;
          color: black;
        }
        QToolButton:hover {
          background-color: #b5c8d8;
          border-color: #6a85a8;
        }
        QToolButton:pressed {
          background-color: #8da4b8;
        }
        QPushButton {
          background-color: #9db4c8;
          border: 1px solid #7a95a8;
          border-radius: 4px;
          padding: 8px;
          color: black;
          font-weight: bold;
        }
        QPushButton:hover {
          background-color: #b5c8d8;
          border-color: #2a82da;
        }
        QPushButton:pressed {
          background-color: #8da4b8;
        }
        QPushButton:checked {
          background-color: #2a82da;
          color: white;
          border-color: #1a62ba;
        }
        QDockWidget {
          border: 1px solid #a0a0a0;
        }
        QDockWidget::title {
          background-color: #c0c0c0;
          padding: 6px;
          border: 1px solid #a0a0a0;
        }
        QDockWidget::close-button, QDockWidget::float-button {
          background-color: #9db4c8;
          border: 1px solid #7a95a8;
          border-radius: 2px;
          padding: 2px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
          background-color: #b5c8d8;
        }
        QTextEdit {
          background-color: #2d2d2d;
          color: #d4d4d4;
          border: 1px solid #a0a0a0;
          font-family: Courier;
        }
        QSplitter::handle {
          background-color: #a0a0a0;
        }
        QLabel {
          color: black;
        }
      )");
    }
  }


  /**
   * Load application settings
   */
  void IsisWorkspaceMainWindow::loadSettings() {
    QSettings settings("USGS", "ISISWorkspace");

    // Restore geometry and dock state
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Restore theme
    m_isDarkTheme = settings.value("darkTheme", false).toBool();
  }


  /**
   * Save application settings
   */
  void IsisWorkspaceMainWindow::saveSettings() {
    QSettings settings("USGS", "ISISWorkspace");

    // Save geometry and dock state
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("darkTheme", m_isDarkTheme);
  }


  /**
   * Handle close event
   */
  void IsisWorkspaceMainWindow::closeEvent(QCloseEvent *event) {
    if (m_project->isModified()) {
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Save Project?",
                                   "Project has unsaved changes. Save before closing?",
                                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

      if (reply == QMessageBox::Yes) {
        onSaveProject();
        event->accept();
      }
      else if (reply == QMessageBox::No) {
        event->accept();
      }
      else {
        event->ignore();
      }
    }
    else {
      event->accept();
    }
  }

}
