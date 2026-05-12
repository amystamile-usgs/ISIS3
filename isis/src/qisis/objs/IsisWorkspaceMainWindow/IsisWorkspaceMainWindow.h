#ifndef IsisWorkspaceMainWindow_h
#define IsisWorkspaceMainWindow_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QMainWindow>
#include <QMap>
#include <QPointer>

class QAction;
class QDockWidget;
class QMenu;
class QPushButton;
class QSplitter;
class QToolBar;
class QVBoxLayout;
class QWidget;

namespace Isis {
  class ControlNet;
  class CubeViewPanel;
  class ControlNetPanel;
  class FootprintPanel;
  class TerminalWidget;
  class WorkspaceProject;

  /**
   * @brief Modern integrated ISIS workspace with dockable panels
   *
   * This application provides a unified interface for ISIS cube viewing,
   * control network editing, and footprint visualization. It combines
   * functionality from qview, qnet, cneteditor, and qmos into a single
   * modern GUI with dockable panels.
   *
   * Features:
   * - Dockable panels for cube viewing, control network editing, and footprints
   * - Synchronized views - all panels work with the same cubes
   * - Modern Qt6 styling with dark/light theme support
   * - Project-based workflow with save/restore
   *
   * @author 2026-05-12 Amy Stamile
   *
   * @internal
   */
  class IsisWorkspaceMainWindow : public QMainWindow {
    Q_OBJECT

  public:
    IsisWorkspaceMainWindow(QWidget *parent = nullptr);
    virtual ~IsisWorkspaceMainWindow();

    void loadCube(const QString &cubePath);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void onOpenCubes();
    void onSaveProject();
    void onOpenProject();
    void onToggleTheme();
    void onAbout();
    void onTerminalCommand(const QString &command);
    void onToggleQView();
    void onToggleQNet();
    void onToggleQMos();

  private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createMainLayout();
    void createLauncherPanel();
    void loadSettings();
    void saveSettings();
    void applyTheme(const QString &themeName);
    void setupConnections();
    void updateCentralWidget();
    class QWidget* createAppPanel(const QString &appName);

    // Project management
    QPointer<WorkspaceProject> m_project;

    // Main layout widgets
    TerminalWidget *m_terminal;
    QDockWidget *m_terminalDock;
    QWidget *m_launcherPanel;
    QDockWidget *m_launcherDock;
    QWidget *m_centralWidget;

    // Launcher buttons
    QPushButton *m_qviewButton;
    QPushButton *m_qnetButton;
    QPushButton *m_qmosButton;

    // Track open dock widgets
    QMap<QString, QDockWidget*> m_openPanels;

    // Menus
    QPointer<QMenu> m_fileMenu;
    QPointer<QMenu> m_viewMenu;
    QPointer<QMenu> m_toolsMenu;
    QPointer<QMenu> m_helpMenu;

    // Toolbars
    QPointer<QToolBar> m_mainToolBar;

    // Actions
    QPointer<QAction> m_openCubesAction;
    QPointer<QAction> m_saveProjectAction;
    QPointer<QAction> m_openProjectAction;
    QPointer<QAction> m_exitAction;
    QPointer<QAction> m_toggleThemeAction;
    QPointer<QAction> m_aboutAction;

    // Settings
    QString m_currentTheme;
    bool m_isDarkTheme;
  };
}

#endif
