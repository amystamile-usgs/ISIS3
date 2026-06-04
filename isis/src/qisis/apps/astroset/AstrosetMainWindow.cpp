/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AstrosetMainWindow.h"
#include "DashboardWidget.h"
#include "SlidingPanelManager.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QDockWidget>
#include <QLabel>
#include <QTimer>
#include <QMap>
#include <QMapIterator>
#include <QMdiArea>
#include <QObject>
#include <QRect>
#include <QRegExp>
#include <QStringList>
#include <QtWidgets>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QStringList>
#include <QDateTime>
#include <QTreeView>
#include <QVariant>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScrollArea>


#include "AbstractProjectItemView.h"
#include "ControlHealthMonitorView.h"
#include "Cube.h"
#include "CubeDnView.h"
#include "Directory.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Image.h"
#include "ImageList.h"
#include "JigsawRunWidget.h"
#include "MosaicSceneWidget.h"
#include "ProgressWidget.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"
#include "ProjectItemTreeView.h"
#include "FileName.h"
#include "Pvl.h"
#include "OpenProjectWorkOrder.h"
#include "TemplateEditorWidget.h"
#include "ViewSubWindow.h"

namespace Isis {
  /**
   * Construct the main window. This will create a Directory, the menus, and the dock areas.
   *
   * @param parent The Qt-relationship parent widget (usually NULL in this case)
   *
   * @internal
   *   @history 2016-11-09 Tyler Wilson - Moved the if-block which loads a project from the
   *                             command line from the start of the constructor to the end
   *                             because if there were warnings and errors, they were not
   *                             being output to the Warnings widget since the project is loaded
   *                             before the GUI is constructed.  Fixes #4488
   *   @history 2016-11-09 Ian Humphrey - Added default readSettings() call to load initial
   *                           default project window state. References #4358.
   */
  AstrosetMainWindow::AstrosetMainWindow(QWidget *parent) :
      QMainWindow(parent) {
    m_maxThreadCount = -1;

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(24, 32, 40));
    darkPalette.setColor(QPalette::WindowText, QColor(180, 200, 210));
    darkPalette.setColor(QPalette::Base, QColor(28, 38, 45));
    darkPalette.setColor(QPalette::AlternateBase, QColor(32, 42, 50));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(180, 200, 210));
    darkPalette.setColor(QPalette::ToolTipText, QColor(180, 200, 210));
    darkPalette.setColor(QPalette::Text, QColor(180, 200, 210));
    darkPalette.setColor(QPalette::Button, QColor(40, 55, 65));
    darkPalette.setColor(QPalette::ButtonText, QColor(180, 200, 210));
    darkPalette.setColor(QPalette::BrightText, Qt::white);
    darkPalette.setColor(QPalette::Link, QColor(96, 216, 220));
    darkPalette.setColor(QPalette::Highlight, QColor(70, 140, 150));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(darkPalette);

    QWidget *centralWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("<h1>Welcome to Astroset</h1>");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { margin: 20px; }");

    QLabel *instructionsLabel = new QLabel(
      "<div style='font-size: 14px; line-height: 1.6;'>"
      "<h2>Getting Started:</h2>"
      "<ol style='margin-left: 20px;'>"
      "<li style='margin-bottom: 10px;'><b>Import images:</b> File → Import → Import Images</li>"
      "<li style='margin-bottom: 10px;'><b>Explore data:</b> Use the tree on the left to browse imported images</li>"
      "<li style='margin-bottom: 10px;'><b>Open views:</b> Click images in the dashboard to open viewers</li>"
      "<li style='margin-bottom: 10px;'><b>Save your work:</b> File → Save Project (optional, saves state for later)</li>"
      "</ol>"
      "<p style='margin-top: 20px; padding: 15px; background-color: #ffffcc; border-left: 4px solid #ffaa00;'>"
      "<i><b>Tip:</b> Leave \"Generate footprints\" unchecked during import for faster loading. "
      "You can generate footprints later if you need the Footprint2D view.</i>"
      "</p>"
      "</div>"
    );
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setTextFormat(Qt::RichText);
    instructionsLabel->setMaximumWidth(800);
    instructionsLabel->setMargin(20);

    m_dashboard = new DashboardWidget();

    connect(m_dashboard, SIGNAL(imageClicked(QString)),
            this, SLOT(onImageClicked(QString)));

    // Create panel manager
    m_panelManager = new SlidingPanelManager(this);
    connect(m_panelManager, SIGNAL(allPanelsClosed()),
            this, SLOT(onAllPanelsClosed()));

    // Create stacked widget
    m_centralStack = new QStackedWidget(this);

    // Dashboard container
    QWidget *dashboardContainer = new QWidget();
    dashboardContainer->setStyleSheet(
      "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
      "            stop:0 #1a252e, stop:0.5 #1e2a35, stop:1 #1a252e);"
    );

    QVBoxLayout *dashboardLayout = new QVBoxLayout(dashboardContainer);
    dashboardLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    scrollArea->viewport()->setStyleSheet("background: transparent;");
    scrollArea->setWidget(m_dashboard);

    dashboardLayout->addWidget(scrollArea);

    // Add both to stack
    m_centralStack->addWidget(dashboardContainer);  // Index 0
    m_centralStack->addWidget(m_panelManager);       // Index 1

    // Create a container with stack + tab bar overlay
    QWidget *mainContainer = new QWidget();
    mainContainer->setStyleSheet("QWidget { background: #1c2630; }");
    QHBoxLayout *mainContainerLayout = new QHBoxLayout(mainContainer);
    mainContainerLayout->setContentsMargins(0, 0, 0, 0);
    mainContainerLayout->setSpacing(0);

    mainContainerLayout->addWidget(m_centralStack);

    // Tab bar will be added by panel manager as needed
    // For now, just use the stack

    setCentralWidget(mainContainer);

    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::South);

    // This was causing some buggy behavior, but this is what we would ultimately like.
    // Allows a user to undock a group of tabs.
    //setDockOptions(GroupedDragging | AllowTabbedDocks);

    setDockNestingEnabled(true);

    setStyleSheet(
      "QMainWindow { background: #1c2630; } "
      "QWidget { background: #1c2630; } "
      "QMainWindow::separator { background: rgba(70, 140, 150, 0.4); width: 3; height: 3px; } "
      "QDockWidget { color: #b4c8d2; background: #1c2630; border: none; } "
      "QDockWidget::title { background: #28363f; padding: 8px; color: #b4c8d2; font-weight: 600; } "
      "QTreeView { background: #1c2630; color: #b4c8d2; border: 1px solid #3a4e58; selection-background-color: #468c96; } "
      "QTreeView::item:selected { background: #468c96; color: white; } "
      "QTreeView::item:hover { background: rgba(70, 140, 150, 0.3); } "
      "QTreeView::branch { background: #1c2630; } "
      "QMenuBar { background: #1e2a35; color: #b4c8d2; border: none; padding: 2px; } "
      "QMenuBar::item { padding: 8px 14px; border-radius: 0; background: transparent; } "
      "QMenuBar::item:selected { background: #468c96; color: white; } "
      "QMenuBar::item:pressed { background: #3a7888; } "
      "QMenu { background: #28363f; color: #b4c8d2; border: 1px solid #468c96; padding: 4px; } "
      "QMenu::item { padding: 8px 30px; } "
      "QMenu::item:selected { background: #468c96; color: white; } "
      "QMenu::separator { height: 1px; background: #3a4e58; margin: 4px 10px; } "
      "QToolBar { background: #1e2a35; border: none; spacing: 4px; padding: 6px; } "
      "QToolButton { background: transparent; color: #60d8dc; border: none; padding: 10px; border-radius: 6px; } "
      "QToolButton:hover { background: rgba(70, 140, 150, 0.5); } "
      "QToolButton:pressed { background: rgba(70, 140, 150, 0.7); } "
      "QToolButton:checked { background: #468c96; color: white; } "
      "QStatusBar { background: #1c2630; color: #b4c8d2; } "
      "QScrollBar:vertical { background: #1c2630; width: 12px; } "
      "QScrollBar::handle:vertical { background: #3a4e58; border-radius: 6px; min-height: 20px; } "
      "QScrollBar::handle:vertical:hover { background: #468c96; } "
      "QScrollBar:horizontal { background: #1c2630; height: 12px; } "
      "QScrollBar::handle:horizontal { background: #3a4e58; border-radius: 6px; min-width: 20px; } "
      "QScrollBar::handle:horizontal:hover { background: #468c96; } "
    );

    try {
      m_directory = new Directory(this);
      connect(m_directory, SIGNAL( newWidgetAvailable(QWidget *) ),
              this, SLOT( addView(QWidget *) ) );

      connect(m_directory, SIGNAL(closeView(QWidget *)),
              this, SLOT(removeView(QWidget *)));

      connect(m_directory, SIGNAL( directoryCleaned() ),
              this, SLOT( removeAllViews() ) );
      connect(m_directory->project(), SIGNAL(projectLoaded(Project *)),
              this, SLOT(readSettings(Project *)));
      connect(m_directory->project(), SIGNAL(projectSaved(Project *)),
              this, SLOT(writeSettings(Project *)));

      connect(m_directory->project(), SIGNAL(imagesAdded(ImageList *)),
              this, SLOT(onImagesAdded(ImageList *)));

      QTimer::singleShot(1000, this, SLOT(applyCustomIcons()));
      QTimer::singleShot(2000, this, SLOT(applyCustomIcons()));

      connect(m_directory, SIGNAL( newWarning() ),
              this, SLOT( raiseWarningTab() ) );
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer,
          "Could not create Directory.", _FILEINFO_);
    }

    m_projectDock = new QDockWidget("Project", this, Qt::SubWindow);
    m_projectDock->setObjectName("projectDock");
    m_projectDock->setFeatures(QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_projectDock->setStyleSheet(
      "QDockWidget { background: #1c2630; border: none; }"
      "QDockWidget::title { background: #28363f; color: #90b0c0; padding: 8px; font-weight: 600; }"
    );

    ProjectItemTreeView *projectTreeView = m_directory->addProjectItemTreeView();
    projectTreeView->setInternalModel( m_directory->model() );
    projectTreeView->treeView()->expandAll();
    projectTreeView->installEventFilter(this);
    projectTreeView->setAutoFillBackground(true);
    QPalette projectPalette = projectTreeView->palette();
    projectPalette.setColor(QPalette::Window, QColor(28, 38, 48));
    projectPalette.setColor(QPalette::Base, QColor(28, 38, 48));
    projectPalette.setColor(QPalette::Text, QColor(180, 200, 210));
    projectPalette.setColor(QPalette::WindowText, QColor(180, 200, 210));
    projectTreeView->setPalette(projectPalette);

    QTreeView *treeView = projectTreeView->treeView();
    if (treeView) {
      treeView->setAutoFillBackground(true);
      QPalette treePalette = treeView->palette();
      treePalette.setColor(QPalette::Base, QColor(28, 38, 48));
      treePalette.setColor(QPalette::Text, QColor(180, 200, 210));
      treePalette.setColor(QPalette::Window, QColor(28, 38, 48));
      treePalette.setColor(QPalette::WindowText, QColor(180, 200, 210));
      treePalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
      treePalette.setColor(QPalette::BrightText, QColor(255, 255, 255));
      treeView->setPalette(treePalette);
      treeView->setStyleSheet(
        "QTreeView { "
        "  background: #1c2630; "
        "  color: #b4c8d2; "
        "  border: none; "
        "}"
        "QTreeView::item { "
        "  color: #b4c8d2; "
        "  background: transparent; "
        "  padding: 4px; "
        "}"
        "QTreeView::item:selected { "
        "  background: #468c96; "
        "  color: #ffffff; "
        "}"
        "QTreeView::item:hover { "
        "  background: rgba(70, 140, 150, 0.3); "
        "  color: #ffffff; "
        "}"
        "QTreeView::branch { "
        "  background: #1c2630; "
        "}"
      );
    }

    m_projectDock->setWidget(projectTreeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock, Qt::Horizontal);

    m_warningsDock = new QDockWidget("Warnings", this, Qt::SubWindow);
    m_warningsDock->setObjectName("m_warningsDock");
    m_warningsDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    m_warningsDock->setWhatsThis(tr("This shows notices and warnings from all operations "
                          "on the current project."));
    m_warningsDock->setAllowedAreas(Qt::BottomDockWidgetArea);

    m_warningsDock->setStyleSheet(
      "QDockWidget { background: rgba(28, 38, 48, 0.95); border: 1px solid rgba(70, 140, 150, 0.4); }"
      "QDockWidget::title { background: rgba(40, 55, 65, 0.95); color: #90b0c0; padding: 10px; font-weight: 600; }"
    );

    m_directory->setWarningContainer(m_warningsDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_warningsDock);
    m_warningsDock->setMaximumHeight(150);

    QDockWidget *historyDock = new QDockWidget("History", this, Qt::SubWindow);
    historyDock->setObjectName("historyDock");
    historyDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    historyDock->setWhatsThis(tr("This shows all operations performed on the current project."));
    historyDock->setAllowedAreas(Qt::BottomDockWidgetArea);

    historyDock->setStyleSheet(
      "QDockWidget { background: rgba(28, 38, 48, 0.95); border: 1px solid rgba(70, 140, 150, 0.4); }"
      "QDockWidget::title { background: rgba(40, 55, 65, 0.95); color: #90b0c0; padding: 10px; font-weight: 600; }"
    );

    m_directory->setHistoryContainer(historyDock);
    tabifyDockWidget(m_warningsDock, historyDock);

    QTabBar *tabBar = nullptr;
    QList<QTabBar *> tabBars = findChildren<QTabBar *>();
    for (QTabBar *tb : tabBars) {
      if (tb->count() > 0) {
        tabBar = tb;
        break;
      }
    }
    if (tabBar) {
      tabBar->setStyleSheet(
        "QTabBar::tab { "
        "  background: rgba(40, 55, 65, 0.9); "
        "  color: #8099aa; "
        "  padding: 8px 16px; "
        "  border: none; "
        "  margin-right: 2px; "
        "  border-radius: 0; "
        "}"
        "QTabBar::tab:selected { "
        "  background: rgba(70, 140, 150, 0.7); "
        "  color: #ffffff; "
        "  border-bottom: 2px solid #60d8dc; "
        "}"
        "QTabBar::tab:hover { "
        "  background: rgba(60, 100, 110, 0.6); "
        "  color: #c8dce6; "
        "}"
      );
    }

    historyDock->raise();

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    statusBar()->setStyleSheet(
      "QStatusBar { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(15, 12, 30, 0.95), stop:1 rgba(10, 10, 25, 0.95)); "
      "  border-top: 1px solid rgba(100, 80, 180, 0.4); "
      "  color: #7c8ba0; "
      "}"
    );
    statusBar()->showMessage("Ready");

    QProgressBar *projectProgress = m_directory->project()->progress();
    projectProgress->setStyleSheet(
      "QProgressBar { "
      "  background: rgba(30, 25, 50, 0.6); "
      "  border: none; "
      "  border-radius: 6px; "
      "  text-align: center; "
      "  color: #00ff88; "
      "  height: 8px; "
      "}"
      "QProgressBar::chunk { "
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00ff88, stop:1 #00ccaa); "
      "  border-radius: 6px; "
      "}"
    );
    statusBar()->addWidget(projectProgress);

    foreach (QProgressBar *progressBar, m_directory->progressBars()) {
      progressBar->setStyleSheet(
        "QProgressBar { "
        "  background: rgba(30, 25, 50, 0.6); "
        "  border: none; "
        "  border-radius: 6px; "
        "  text-align: center; "
        "  color: #00ff88; "
        "}"
        "QProgressBar::chunk { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00ff88, stop:1 #00ccaa); "
        "  border-radius: 6px; "
        "}"
      );
      statusBar()->addWidget(progressBar);
    }

    readSettings(m_directory->project() );

    initializeActions();
    createMenus();
    createToolBars();

    menuBar()->setStyleSheet(
      "QMenuBar { "
      "  background: rgba(12, 12, 28, 0.95); "
      "  border-bottom: 1px solid rgba(80, 70, 140, 0.4); "
      "  color: #c0cce0; "
      "}"
      "QMenuBar::item { "
      "  padding: 8px 16px; "
      "  background: transparent; "
      "  color: #c0cce0; "
      "}"
      "QMenuBar::item:selected { "
      "  background: rgba(80, 60, 140, 0.5); "
      "  color: #ffffff; "
      "}"
    );

    QCoreApplication::setApplicationName("ipce");
    QStringList args = QCoreApplication::arguments();

    if (args.count() == 2) {
      OpenProjectWorkOrder *workorder = new OpenProjectWorkOrder(m_directory->project());
      workorder->execute();
    }
  }


  /**
   * This is connected from Directory's newWidgetAvailable signal
   *
   * @param[in] newWidget (QWidget *)
   */
  void AstrosetMainWindow::addView(QWidget *newWidget, Qt::DockWidgetArea area,
                               Qt::Orientation orientation) {

    // JigsawRunWidget is already a QDockWidget, and no modifications need to be made to it
    if (qobject_cast<JigsawRunWidget *>(newWidget)) {
      splitDockWidget(m_projectDock, (QDockWidget*)newWidget, Qt::Vertical);

      // Save view docks for cleanup during a project close
      m_specialDocks.append((QDockWidget *)newWidget);
      return;
    }

    QDockWidget *dock = new QDockWidget(newWidget->windowTitle(), this);
    dock->setWidget(newWidget);
    dock->setObjectName(newWidget->objectName());
    dock->setAttribute(Qt::WA_DeleteOnClose);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);

    // Apply dark styling to all view docks
    dock->setStyleSheet(
      "QDockWidget { "
      "  background: rgba(18, 18, 35, 0.95); "
      "  border: 1px solid rgba(100, 80, 180, 0.4); "
      "  color: #e8eef7; "
      "}"
      "QDockWidget::title { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(25, 20, 45, 0.95), stop:1 rgba(20, 18, 38, 0.95)); "
      "  color: #e8eef7; "
      "  padding: 10px; "
      "  border-bottom: 1px solid rgba(100, 80, 180, 0.4); "
      "}"
    );

    // Apply dark styling to the widget itself
    newWidget->setStyleSheet(
      "QWidget { "
      "  background: rgba(15, 15, 30, 0.95); "
      "  color: #e8eef7; "
      "}"
      "QToolBar { "
      "  background: rgba(20, 18, 38, 0.9); "
      "  border: none; "
      "  spacing: 4px; "
      "}"
      "QToolButton { "
      "  background: transparent; "
      "  border: 1px solid transparent; "
      "  border-radius: 6px; "
      "  padding: 6px; "
      "}"
      "QToolButton:hover { "
      "  background: rgba(60, 50, 120, 0.4); "
      "  border: 1px solid rgba(120, 100, 220, 0.5); "
      "}"
      "QMenuBar { "
      "  background: rgba(20, 18, 38, 0.9); "
      "  color: #e8eef7; "
      "}"
      "QMenuBar::item { "
      "  background: transparent; "
      "  color: #e8eef7; "
      "  padding: 6px 12px; "
      "}"
      "QMenuBar::item:selected { "
      "  background: rgba(80, 60, 140, 0.5); "
      "}"
    );

    if ( qobject_cast<ControlHealthMonitorView *>(newWidget) ||
              qobject_cast<TemplateEditorWidget *>(newWidget)) {
      dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
      splitDockWidget(m_projectDock, dock, Qt::Vertical);
      m_specialDocks.append(dock);
    }
    else {
      // Put cube viewers and other views in sliding panels
      if (m_panelManager) {
        // Get panel label from widget title or object name
        QString panelLabel = newWidget->windowTitle();
        if (panelLabel.isEmpty()) {
          panelLabel = newWidget->objectName();
        }
        if (panelLabel.isEmpty()) {
          panelLabel = "View";
        }

        // Extract a simpler ID from the title if it contains a filename
        QString panelId = QString("view_%1").arg((qulonglong)newWidget, 0, 16);
        if (panelLabel.contains(".cub")) {
          // Use filename as ID for consistency
          QFileInfo fi(panelLabel.split("@").first().trimmed());
          panelId = QString("img_%1").arg(fi.absoluteFilePath());
        }

        // Add to panel manager
        m_panelManager->addPanel(panelId, newWidget, panelLabel);
        m_panelManager->showPanel(panelId);

        // Switch to panel view
        m_centralStack->setCurrentIndex(1);

        // Don't create dock, view is now in panel
        delete dock;
        return;
      }
      // Desired behavior of docking views:
      // First regular view (footprint,cubeDisplay) is added to the right of the "special" views
      // (project, sensor, jigsaw, controlHealth).  Adding additional "regular" views are tabbed
      // with the last "regular" view which was added.
      // The following logic not guaranteed to work as intended. If user moves one of the "special"
      // views such as sensor from below the project dock to the right of the project, the following
      // will put the new dock to the right of the moved "special" dock instead of tabbing.  The only
      // way to possibly ensure the intended functionality would be to check for the position of the
      // last added dock and either add or tabify depending on location.  This might also allow the
      // docks to be kept in a single list instead of m_specialDocks & m_viewDocks.
      if (m_viewDocks.count() == 0) {
        addDockWidget(Qt::RightDockWidgetArea, dock, Qt::Horizontal);
      }
      else {
        tabifyDockWidget(m_viewDocks.last(), dock);
        dock->show();
        dock->raise();
      }

      // Save view docks for cleanup during a project close
      m_viewDocks.append(dock);
    }

    // When dock widget is destroyed, make sure the view it holds is also destroyed
    connect(dock, SIGNAL(destroyed(QObject *)), newWidget, SLOT(deleteLater()));
    // The list of dock widgets needs cleanup as each view is destroyed
    connect(dock, SIGNAL(destroyed(QObject *)), this, SLOT(cleanupViewDockList(QObject *)));

    // Only emit the signal when one view is added just for simplicity; behavior would not change
    // if this was emitted after every addition.
    if (m_viewDocks.size() == 1) {
      emit enableViewActions(true);
    }
  }


  /**
   * Cleans up m_viewDocks when a view is closed (object is destroyed).
   *
   * @param view QObject* The dock widget to remove from the m_viewDocks
   */
  void AstrosetMainWindow::cleanupViewDockList(QObject *obj) {

    QDockWidget *dock = static_cast<QDockWidget *>(obj);
    if (dock) {
      m_viewDocks.removeAll(dock);
      m_specialDocks.removeAll(dock);
    }

    if (m_viewDocks.size() == 0) {
      emit enableViewActions(false);
    }
  }


  /**
   * This slot is connected from Directory::closeView(QWidget *) signal.  It will close the given
   * view and delete the view.
   *
   * @param view QWidget* The view to close.
   */
  void AstrosetMainWindow::removeView(QWidget *view) {

    QDockWidget *parentDock = qobject_cast<QDockWidget *>(view->parent());
    removeDockWidget(parentDock);
    delete parentDock;
  }


  /**
   * Removes All Views in main window, connected to directory signal directoryCleaned()
   */
  void AstrosetMainWindow::removeAllViews() {
    setWindowTitle("ipce");
    foreach (QDockWidget *dock, m_viewDocks) {
      if (dock) {
        removeDockWidget(dock);
        delete dock;
      }
    }

    foreach (QDockWidget *dock, m_specialDocks) {
      if (dock) {
        removeDockWidget(dock);
        m_specialDocks.removeAll(dock);
        delete dock;
      }
    }
    m_viewDocks.clear();
    m_specialDocks.clear();
    emit enableViewActions(false);
}


  /**
   * Cleans up the directory.
   */
  AstrosetMainWindow::~AstrosetMainWindow() {
    m_directory->deleteLater();
  }


  /**
   * This is needed so that the project clean flag is not set to false when move and resize events
   * are emitted from ipce.cpp when AstrosetMainWindow::show() is called.
   * The non-spontaneous or internal QShowEvent is only emitted once from ipce.cpp, so the project
   * clean flag can be reset.
   *
   * @param event QShowEvent*
   *
   */
  void AstrosetMainWindow::showEvent(QShowEvent *event) {
    if (!event->spontaneous()) {
      m_directory->project()->setClean(true);
    }
  }


  /**
   * Filters out events from views so they can be handled by the main
   * window. Filters out DragEnter Drop and ContextMenu events from
   * views.
   *
   * @param[in] watched (QObject *) The object being filtered.
   * @param[in] event (QEvent *) The event that may be filtered.
   */
  bool AstrosetMainWindow::eventFilter(QObject *watched, QEvent *event) {
    if ( AbstractProjectItemView *view = qobject_cast<AbstractProjectItemView *>(watched) ) {
      if (event->type() == QEvent::DragEnter) {
        return true;
      }
      else if (event->type() == QEvent::Drop) {
        return true;
      }
      else if (event->type() == QEvent::ContextMenu) {
        QMenu contextMenu;

        QList<QAction *> viewActions = view->contextMenuActions();

        if ( !viewActions.isEmpty() ) {
          foreach (QAction *action, viewActions) {
            if (action) {
              contextMenu.addAction(action);
            }
            else {
              contextMenu.addSeparator();
            }
          }
          contextMenu.addSeparator();
        }

        QList<QAction *> workOrders = m_directory->supportedActions( view->currentItem() );

        if ( !workOrders.isEmpty() ) {
          foreach (QAction *action, workOrders) {
            contextMenu.addAction(action);
          }
          contextMenu.addSeparator();
        }

        contextMenu.exec( static_cast<QContextMenuEvent *>(event)->globalPos() );

        return true;
      }
    }

    return QMainWindow::eventFilter(watched, event);
  }


  /**
   * This method takes the max thread count setting and asks
   * QtConcurrent to respect it.
   */
  void AstrosetMainWindow::applyMaxThreadCount() {
    if (m_maxThreadCount <= 1) {
      // Allow QtConcurrent to use every core and starve the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }
    else {
      // subtract 1 to account for the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(m_maxThreadCount - 1);
    }
  }


  /**
   * Initializes the internal lists of actions of the main window for
   * use in the menus and toolbars.
   */
  void AstrosetMainWindow::initializeActions() {
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setIcon( QIcon::fromTheme("window-close") );
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    m_fileMenuActions.append(exitAction);
    m_permToolBarActions.append(exitAction);

    QAction *tabViewsAction = new QAction("Tab Views", this);
    connect( tabViewsAction, SIGNAL(triggered()), this, SLOT(tabViews()) );
    connect( this, SIGNAL(enableViewActions(bool)), tabViewsAction, SLOT(setEnabled(bool)) );
    m_viewMenuActions.append(tabViewsAction);
    tabViewsAction->setDisabled(true);  // Disabled on default, until a view is added

    QAction *tileViewsAction = new QAction("Tile Views", this);
    connect( tileViewsAction, SIGNAL(triggered()), this, SLOT(tileViews()) );
    connect( this, SIGNAL(enableViewActions(bool)), tileViewsAction, SLOT(setEnabled(bool)) );
    m_viewMenuActions.append(tileViewsAction);
    tileViewsAction->setDisabled(true); // Disabled on default, until a view is added

    QAction *undoAction = m_directory->undoAction();
    undoAction->setShortcut(Qt::Key_Z | Qt::CTRL);

    QAction *redoAction = m_directory->redoAction();
    redoAction->setShortcut(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);

    m_editMenuActions.append(undoAction);
    m_editMenuActions.append(redoAction);

    QAction *threadLimitAction = new QAction("Set Thread &Limit", this);
    connect(threadLimitAction, SIGNAL(triggered()),
            this, SLOT(configureThreadLimit()));

    m_settingsMenuActions.append(m_directory->project()->userPreferenceActions());
    m_settingsMenuActions.append(threadLimitAction);

    QAction *activateWhatsThisAct = new QAction("&What's This", this);
    activateWhatsThisAct->setShortcut(Qt::SHIFT | Qt::Key_F1);
    activateWhatsThisAct->setIcon(
        QPixmap(FileName("$ISISROOT/appdata/images/icons/contexthelp.png").expanded()));
    activateWhatsThisAct->setToolTip("Activate What's This and click on parts "
        "this program to see more information about them");
    connect(activateWhatsThisAct, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

    m_helpMenuActions.append(activateWhatsThisAct);
  }


  /**
   * Creates and fills the application menus of the menu bar.
   */
  void AstrosetMainWindow::createMenus() {

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->setObjectName("fileMenu");
    // Get Directory FileMenu actions
    foreach ( QAction *action, m_directory->fileMenuActions() ) {
      m_fileMenu->addAction(action);
    }
    m_fileMenu->addSeparator();
    // Get FileMenu actions from the ipceMainWindow, Exit is the only action
    foreach ( QAction *action, m_fileMenuActions ) {
      m_fileMenu->addAction(action);
    }

    m_projectMenu = menuBar()->addMenu(tr("&Project"));
    m_projectMenu->setObjectName("projectMenu");
    //  Get Project menu actions from Directory
    foreach ( QAction *action, m_directory->projectMenuActions() ) {
      m_projectMenu->addAction(action);
    }
    // Allow tool tips to be displayed for the project menu's actions (e.g. "Bundle Adjustment")
    // This is a work around for Qt's what this text not working on disabled actions
    // (even though the Qt documentation says it should work on disabled QAction's).
    m_projectMenu->setToolTipsVisible(true);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->setObjectName("editMenu");
    m_editMenu->addSeparator();
    // Get Edit menu actions from Directory
    foreach ( QAction *action, m_directory->editMenuActions() ) {
      m_editMenu->addAction(action);
    }
    // Get Edit menu actions from AstrosetMainWindow
    foreach ( QAction *action, m_editMenuActions ) {
      m_editMenu->addAction(action);
    }

    m_viewMenu = menuBar()->addMenu("&View");
    m_viewMenu->setObjectName("viewMenu");
    // Get View menu actions from Directory
    foreach ( QAction *action, m_directory->viewMenuActions() ) {
      m_viewMenu->addAction(action);
    }
    m_viewMenu->addSeparator();
    // Get View menu actions from AstrosetMainWindow
    foreach ( QAction *action, m_viewMenuActions ) {
      m_viewMenu->addAction(action);
    }

    m_settingsMenu = menuBar()->addMenu("&Settings");
    m_settingsMenu->setObjectName("settingsMenu");
    // Get Settings menu actions from Directory
    foreach ( QAction *action, m_directory->settingsMenuActions() ) {
      m_settingsMenu->addAction(action);
    }
    m_settingsMenu->addSeparator();
    // Get Settings menu actions from AstrosetMainWindow
    foreach ( QAction *action, m_settingsMenuActions ) {
      m_settingsMenu->addAction(action);
    }

    m_helpMenu = menuBar()->addMenu("&Help");
    m_helpMenu->setObjectName("helpMenu");
    // Get Help menu actions from Directory
    foreach ( QAction *action, m_directory->helpMenuActions() ) {
      m_helpMenu->addAction(action);
    }
    m_helpMenu->addSeparator();
    // Get Help menu actions from AstrosetMainWindow
    foreach ( QAction *action, m_helpMenuActions ) {
      m_helpMenu->addAction(action);
    }
  }


  /**
   * Create the tool bars and populates them with QActions from several sources. Actions are taken
   * from an internal list of QActions and the Directory.
   */
  void AstrosetMainWindow::createToolBars() {
    m_permToolBar = new QToolBar(this);
    QSize iconSize(25, 45);
    m_permToolBar->setIconSize(iconSize);
    m_permToolBar->setObjectName("PermanentToolBar");

    // Style the toolbar with teal theme
    m_permToolBar->setStyleSheet(
      "QToolBar { "
      "  background: #1e2a35; "
      "  border-bottom: 1px solid rgba(70, 140, 150, 0.4); "
      "  spacing: 6px; "
      "  padding: 10px; "
      "}"
      "QToolButton { "
      "  background: transparent; "
      "  border: 1px solid transparent; "
      "  border-radius: 8px; "
      "  padding: 8px; "
      "  color: #60d8dc; "
      "}"
      "QToolButton:hover { "
      "  background: rgba(70, 140, 150, 0.5); "
      "  border: 1px solid rgba(96, 216, 220, 0.5); "
      "}"
      "QToolButton:pressed { "
      "  background: rgba(70, 140, 150, 0.7); "
      "  border: 1px solid rgba(96, 216, 220, 0.8); "
      "}"
    );

    addToolBar(m_permToolBar);

    foreach ( QAction *action, m_directory->permToolBarActions() ) {
      QString actionText = action->text().toLower();
      QString iconPath;

      if (actionText.contains("open")) {
        iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/open.svg";
      }
      else if (actionText.contains("save as")) {
        iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/save-as.svg";
      }
      else if (actionText.contains("save")) {
        iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/save.svg";
      }

      if (!iconPath.isEmpty()) {
        FileName iconFile(iconPath);
        QIcon icon(iconFile.expanded());
        if (!icon.isNull()) {
          action->setIcon(icon);
        }
      }

      m_permToolBar->addAction(action);
    }

    foreach (QAction *action, m_permToolBarActions) {
      m_permToolBar->addAction(action);
    }
  }


  /**
   * Handles when images are added to the project - updates dashboard
   */
  void AstrosetMainWindow::onImagesAdded(ImageList *images) {
    if (!m_dashboard || !images) return;

    // Add each image to the dashboard card
    foreach (Image *image, *images) {
      QString imageName = image->displayProperties()->displayName();
      QString imagePath = image->fileName();
      m_dashboard->addImageToCard(imageName, imagePath);
    }

    // Apply icons multiple times to ensure they stick
    QTimer::singleShot(200, this, SLOT(applyCustomIcons()));
    QTimer::singleShot(500, this, SLOT(applyCustomIcons()));
    QTimer::singleShot(1000, this, SLOT(applyCustomIcons()));

    // Update target body info from the first image if available
    if (!images->isEmpty()) {
      Image *firstImage = images->first();
      try {
        Cube *cube = firstImage->cube();
        if (cube && cube->label()) {
          PvlGroup &instGroup = cube->label()->findGroup("Instrument", Pvl::Traverse);

          // Extract system name
          QString systemName = "";
          if (instGroup.hasKeyword("TargetName")) {
            QString targetName = QString::fromStdString(instGroup["TargetName"][0].toStdString());

            // Determine system name from target name
            if (targetName.compare("MOON", Qt::CaseInsensitive) == 0) {
              systemName = "EARTH";
            }
            else if (targetName.compare("Titan", Qt::CaseInsensitive) == 0 ||
                     targetName.compare("Enceladus", Qt::CaseInsensitive) == 0) {
              systemName = "SATURN";
            }
            else if (targetName.compare("Europa", Qt::CaseInsensitive) == 0) {
              systemName = "JUPITER";
            }
            else if (targetName.compare("Mars", Qt::CaseInsensitive) == 0) {
              systemName = "MARS";
            }

            // Load target image based on target name
            QPixmap targetImage;
            if (targetName.compare("MOON", Qt::CaseInsensitive) == 0) {
              targetImage.load(FileName("$ISISROOT/appdata/images/targets/nasa_moon_large.png").expanded());
            }
            else if (targetName.compare("Enceladus", Qt::CaseInsensitive) == 0) {
              targetImage.load(FileName("$ISISROOT/appdata/images/targets/nasa_enceladus_saturn.png").expanded());
            }
            else if (targetName.compare("Europa", Qt::CaseInsensitive) == 0) {
              targetImage.load(FileName("$ISISROOT/appdata/images/targets/nasa_europa_large.png").expanded());
            }
            else if (targetName.compare("Mars", Qt::CaseInsensitive) == 0) {
              targetImage.load(FileName("$ISISROOT/appdata/images/targets/nasa_mars_large.png").expanded());
            }
            else if (targetName.compare("Titan", Qt::CaseInsensitive) == 0) {
              targetImage.load(FileName("$ISISROOT/appdata/images/targets/nasa_titan_large.png").expanded());
            }

            // Get coordinate info if available
            QString centerLon = "";
            QString centerLat = "";
            try {
              if (cube->label()->hasGroup("Mapping")) {
                PvlGroup &mappingGroup = cube->label()->findGroup("Mapping", Pvl::Traverse);
                if (mappingGroup.hasKeyword("CenterLongitude")) {
                  double lon = mappingGroup["CenterLongitude"];
                  centerLon = QString::number(lon, 'f', 2) + "°";
                }
                if (mappingGroup.hasKeyword("CenterLatitude")) {
                  double lat = mappingGroup["CenterLatitude"];
                  centerLat = QString::number(lat, 'f', 2) + "°";
                }
              }
            } catch (...) {}

            m_dashboard->setTargetBodyInfo(targetName, targetImage, systemName, centerLon, centerLat);
          }

          QString spacecraftName = "";
          QString instrumentName = "";
          QString startTime = "";
          QString exposureDuration = "";
          QString filter = "";

          if (instGroup.hasKeyword("SpacecraftName")) {
            spacecraftName = QString::fromStdString(instGroup["SpacecraftName"][0].toStdString());
          }

          if (instGroup.hasKeyword("InstrumentId")) {
            instrumentName = QString::fromStdString(instGroup["InstrumentId"][0].toStdString());
          }

          if (instGroup.hasKeyword("StartTime")) {
            startTime = QString::fromStdString(instGroup["StartTime"][0].toStdString());
          }

          if (instGroup.hasKeyword("ExposureDuration")) {
            exposureDuration = QString::fromStdString(instGroup["ExposureDuration"][0].toStdString());
          }

          if (instGroup.hasKeyword("FilterName")) {
            filter = QString::fromStdString(instGroup["FilterName"][0].toStdString());
          }

          if (!spacecraftName.isEmpty()) {
            m_dashboard->setSpacecraftInfo(spacecraftName, instrumentName, startTime, exposureDuration, filter);
          }
        }
      }
      catch (...) {
        // If we can't get target info, just skip it
      }
    }
  }

  /**
   * Handles when all panels are closed - return to dashboard
   */
  void AstrosetMainWindow::onAllPanelsClosed() {
    // Switch back to dashboard
    if (m_centralStack) {
      m_centralStack->setCurrentIndex(0);
    }
  }

  /**
   * Handles when an image is clicked in the dashboard - opens the cube viewer
   */
  void AstrosetMainWindow::onImageClicked(const QString &imagePath) {
    // Find the Image object from the project and its corresponding ProjectItem
    ProjectItemModel *model = m_directory->model();
    if (!model) return;

    foreach (ImageList *imageList, m_directory->project()->images()) {
      foreach (Image *image, *imageList) {
        if (image->fileName() == imagePath) {
          // Find the ProjectItem for this image
          ProjectItem *imageItem = model->findItemData(QVariant::fromValue(image));
          if (imageItem) {
            // Check if a panel already exists for this image
            QString panelId = QString("img_%1").arg(imagePath);

            if (m_panelManager && m_panelManager->hasPanel(panelId)) {
              // Panel exists, just show it
              m_panelManager->showPanel(panelId);
              m_centralStack->setCurrentIndex(1);
            } else {
              // Create new view
              CubeDnView *view = m_directory->addCubeDnView();

              // Add the image item to the view
              if (view) {
                QList<ProjectItem *> itemList;
                itemList.append(imageItem);
                view->addItems(itemList);
              }
            }
          }
          return;
        }
      }
    }
  }

  /**
   * Writes the global settings like recent projects and thread count.
   */
  void AstrosetMainWindow::writeGlobalSettings(Project *project) {

    QString appName = QApplication::applicationName();

    QSettings globalSettings(FileName("$HOME/.Isis/" + appName + "/ipce.config").expanded(),
        QSettings::NativeFormat);

    // If no config file exists and a user immediately opens a project,
    // the project's geometry will be saved as a default for when ipce is
    // opened again. Previously, the ipce's default size was small,
    // until a user opened ipce (but not a project) and resized to how they
    // wanted it to be sized, then closed ipce.
    if (project->isTemporaryProject() || !globalSettings.contains("geometry")) {
      globalSettings.setValue("geometry", QVariant(geometry()));
    }

    globalSettings.setValue("maxThreadCount", m_maxThreadCount);
    globalSettings.setValue("maxRecentProjects",m_maxRecentProjects);

    globalSettings.beginGroup("recent_projects");
    QStringList keys = globalSettings.allKeys();
    QMap<QString,QString> recentProjects;

    foreach (QString key,keys) {
      recentProjects[key]=globalSettings.value(key).toString();
    }

    QList<QString> projectPaths = recentProjects.values();

    if (keys.count() >= m_maxRecentProjects) {

      //Clear out the recent projects before repopulating this group
      globalSettings.remove("");

      //If the currently open project is a project that has been saved and is not within the current
      //list of recently open projects, then remove the oldest project from the list.
      if (!project->projectRoot().contains("tmpProject") &&
          !projectPaths.contains(project->projectRoot()) ) {
        QString s=keys.first();
        recentProjects.remove( s );
      }

      //If the currently open project is already contained within the list,
      //then remove the earlier reference.
      if (projectPaths.contains(project->projectRoot())) {
        QString key = recentProjects.key(project->projectRoot());
        recentProjects.remove(key);

      }

      QMap<QString,QString>::iterator i;

      //Iterate through the recentProjects QMap and set the <key,val> pairs.
      for (i=recentProjects.begin();i!=recentProjects.end();i++) {
          globalSettings.setValue(i.key(),i.value());
      }

      //Get a unique time value for generating a key
      long t0 = QDateTime::currentMSecsSinceEpoch();

      QString projName = project->name();
      QString t0String=QString::number(t0);

      //Save the project location
      if (!project->projectRoot().contains("tmpProject") ) {
              globalSettings.setValue(t0String+"%%%%%"+projName,project->projectRoot());
      }
    }

    //The numer of recent open projects is less than m_maxRecentProjects
    else {

      long t0 = QDateTime::currentMSecsSinceEpoch();
      QString projName = project->name();
      QString t0String=QString::number(t0);

      if (!project->isTemporaryProject() &&
          !projectPaths.contains( project->projectRoot())) {
        globalSettings.setValue(t0String+"%%%%%"+projName,project->projectRoot());
      }
    }
    globalSettings.endGroup();
    globalSettings.sync();
  }


  /**
   * Write the window positioning and state information out to a
   * config file. This allows us to restore the settings when we
   * create another main window (the next time this program is run).
   *
   * The state will be saved according to the currently loaded project and its name.
   *
   * When no project is loaded (i.e. the default "Project" is open), the config file used is
   * $HOME/.Isis/$APPNAME/ipce.config.
   * When a project, ProjectName, is loaded, the config file used is
   * project->projectRoot()/ipce.config.
   *
   * @param[in] project Pointer to the project that is currently loaded (default is "Project")
   *
   * @internal
   *   @history 2016-11-09 Ian Humphrey - Settings are now written according to the loaded project.
   *                           References #4358.
   *   @history 2017-10-17 Tyler Wilson Added a [recent projects] group for the saving and
   *                           restoring of recently opened projects.  References #4492.
   *   @history Kaitlyn Lee 2018-07-09 - Added the value "maximized" in the project settings
   *                           so that a project remembers if it was in fullscreen when saved.
   *                           Fixes #5175.
   */
  void AstrosetMainWindow::writeSettings(Project *project) {

    // Ensure that we are not using a NULL pointer
    if (!project) {
      QString msg = "Cannot write settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    QSettings projectSettings(FileName(project->newProjectRoot() + "/ipce.config").expanded(),
        QSettings::NativeFormat);

    projectSettings.setValue("geometry", QVariant(geometry()));
    projectSettings.setValue("windowState", saveState());
    projectSettings.setValue("maximized", isMaximized());
    projectSettings.sync();
  }


  /**
   * Read the window positioning and state information from the config file.
   *
   * When running ipce without opening a project, the config file read is
   * $HOME/.Isis/$APPNAME/ipce.config
   * When running ipce and opening a project (ProjectName), the config file read is
   * project->projectRoot()/ipce.config
   *
   * @param[in] project (Project *) The project that was loaded.
   *
   * @internal
   *   @history Ian Humphrey - Settings are now read on a project name basis. References #4358.
   *   @history Tyler Wilson 2017-11-02 - Settings now read recent projects.  References #4492.
   *   @history Tyler Wilson 2017-11-13 - Commented out a resize call near the end because it
   *                was messing with the positions of widgets after a project was loaded.
   *                Fixes #5075.
   *   @history Makayla Shepherd 2018-06-10 - Settings are read from the project root ipce.config.
   *                If that does not exist then we read from .Isis/ipce/ipce.config.
   *   @history Kaitlyn Lee 2018-07-09 - Added the call showNormal() so when a project is
   *                not saved in fullscreen, the window will resize to the project's
   *                window size. This also fixes the history/warning tabs being misplaced
   *                when opening a project. Fixes #5175.
   */
  void AstrosetMainWindow::readSettings(Project *project) {
    // Ensure that the Project pointer is not NULL
    if (!project) {
      QString msg = "Cannot read settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Set the path of the settings file
    // The default is to assume that the project has an ipce.config in it
    // If the file does not exist then we read settings from .Isis/ipce/ipce.config
    QString appName = QApplication::applicationName();
    QString filePath = project->projectRoot() + "/ipce.config";
    bool isFullScreen = false;
    if (!FileName(filePath).fileExists()) {
      filePath = "$HOME/.Isis/" + appName + "/ipce.config";
      // If the $HOME/.Isis/ipce/ipce.config does not exist then we want ipce to show up in
      // in full screen. In other words the default geometry is full screen
      if (!FileName(filePath).fileExists()) {
        isFullScreen = true;
      }
    }

    if (project->name() == "Project") {
      setWindowTitle("ipce");
    }
    else {
      setWindowTitle( project->name() );
    }

    QSettings projectSettings(FileName(filePath).expanded(), QSettings::NativeFormat);

    if (!isFullScreen) {
      // If a project was not in fullscreen when saved, restore the project's window size.
      if (!projectSettings.value("maximized").toBool()) {
        showNormal();
      }
      setGeometry(projectSettings.value("geometry").value<QRect>());

      if (!project->isTemporaryProject()) {
        restoreState(projectSettings.value("windowState").toByteArray());
      }
    }
    else {
      this->showMaximized();
    }

    if (project->name() == "Project") {
      QSettings globalSettings(FileName("$HOME/.Isis/" + appName + "/ipce.config").expanded(),
                              QSettings::NativeFormat);

      QStringList projectNameList;
      QStringList projectPathList;
      globalSettings.beginGroup("recent_projects");
      QStringList keys = globalSettings.allKeys();
      QRegExp underscore("%%%%%");

      foreach (QString key, keys) {
        QString childKey = "recent_projects/"+key;
        QString projectPath = globalSettings.value(key).toString();
        QString projectName = projectPath.split("/").last();
        projectPathList.append(projectPath) ;
        projectNameList.append(projectName);
      }

      globalSettings.endGroup();

      QStringList projectPathReverseList;
      for (int i = projectPathList.count() - 1; i >= 0; i--) {
        projectPathReverseList.append(projectPathList[i]);
      }

      QStringList projectPathListTruncated;

      int i =0;

      foreach (QString proj,projectPathReverseList) {
        if (i <= m_maxRecentProjects) {
          projectPathListTruncated.append(proj);
          i++;
        }
        else
          break;
        }

      m_directory->setRecentProjectsList(projectPathListTruncated);
      m_directory->updateRecentProjects();
      m_maxThreadCount = globalSettings.value("maxThreadCount", m_maxThreadCount).toInt();
      applyMaxThreadCount();
    }

    m_directory->project()->setClean(true);
  }


  /**
   * Handle the close event by writing the window positioning and
   * state information before forwarding the event to the QMainWindow.
   */
  void AstrosetMainWindow::closeEvent(QCloseEvent *event) {

    foreach(TemplateEditorWidget *templateEditor, m_directory->templateEditorViews()) {
      templateEditor->saveOption();
    }
    // The active control is checked here for modification because this was the simplest solution
    // vs changing the project clean state every time the control is modified or saved.
    if (!m_directory->project()->isClean() || (m_directory->project()->activeControl() &&
                                               m_directory->project()->activeControl()->isModified())) {
      QMessageBox *box = new QMessageBox(QMessageBox::NoIcon, QString("Current Project Has Unsaved Changes"),
                             QString("Would you like to save your current project?"),
                             QMessageBox::NoButton, qobject_cast<QWidget *>(parent()), Qt::Dialog);
      QPushButton *save = box->addButton("Save", QMessageBox::AcceptRole);
      box->addButton("Don't Save", QMessageBox::RejectRole);
      QPushButton *cancel = box->addButton("Cancel", QMessageBox::NoRole);
      box->exec();

      if (box->clickedButton() == (QAbstractButton*)cancel) {
        event->ignore();
        return;
      }
      else if (box->clickedButton() == (QAbstractButton*)save) {
        m_directory->project()->save();
      }
    }
    //  Write global settings, for now this is for the project "Project"
    writeGlobalSettings(m_directory->project());
    m_directory->project()->clear();

    QMainWindow::closeEvent(event);
  }


  /**
   * Ask the user how many threads to use in this program. This
   * includes the GUI thread.
   */
  void AstrosetMainWindow::configureThreadLimit() {
    bool ok = false;

    QStringList options;

    int current = 0;
    options << tr("Use all available");

    for(int i = 1; i < 24; i++) {
      QString option = tr("Use %1 threads").arg(i + 1);

      options << option;
      if(m_maxThreadCount == i + 1)
        current = i;
    }

    QString res = QInputDialog::getItem(NULL, tr("Concurrency"),
        tr("Set the number of threads to use"),
        options, current, false, &ok);

    if (ok) {
      m_maxThreadCount = options.indexOf(res) + 1;

      if (m_maxThreadCount <= 1)
        m_maxThreadCount = -1;

      applyMaxThreadCount();
    }
  }


  /**
   * Activate the What's This? cursor. This is useful for he What's
   * This? action in the help menu.
   */
  void AstrosetMainWindow::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


  /**
   * Tabs all open attached/detached views
   */
  void AstrosetMainWindow::tabViews() {
    // tabifyDockWidget() takes two widgets and tabs them, so an easy way to do
    // this is to grab the first view and tab the rest with the first.
    QDockWidget *firstView = m_viewDocks.first();

    foreach (QDockWidget *currentView, m_viewDocks) {
      // We have to reattach a view before it can be tabbed. If it is attached,
      // this will have no affect.
      currentView->setFloating(false);

      if (currentView == firstView) {
        continue;
      }
      tabifyDockWidget(firstView, currentView);
    }
  }


  /**
   * Tile all open attached/detached views
   */
  void AstrosetMainWindow::tileViews() {
    // splitDockWidget() takes two widgets and tiles them, so an easy way to do
    // this is to grab the first view and tile the rest with the first.
    QDockWidget *firstView = m_viewDocks.first();

    foreach (QDockWidget *currentView, m_viewDocks) {
      // We have to reattach a view before it can be tiled. If it is attached,
      // this will have no affect. We have to call addDockWidget() to untab any views.
      currentView->setFloating(false);
      addDockWidget(Qt::RightDockWidgetArea, currentView, Qt::Horizontal);

      if (currentView == firstView) {
        continue;
      }
      splitDockWidget(firstView, currentView, Qt::Horizontal);
    }
  }


/**
 * Raises the warningWidget to the front of the tabs. Connected to warning signal from directory.
 */
  void AstrosetMainWindow::raiseWarningTab() {
    m_warningsDock->raise();
  }

  /**
   * Apply custom astroset-themed icons to the project tree
   */
  void AstrosetMainWindow::applyCustomIcons() {
    if (!m_directory || !m_directory->model()) {
      return;
    }

    // Get the invisible root item (which is a QStandardItem, not ProjectItem)
    QStandardItem *rootItem = m_directory->model()->invisibleRootItem();
    if (!rootItem) {
      return;
    }

    // Iterate through the root's children, which ARE ProjectItems
    for (int i = 0; i < rootItem->rowCount(); i++) {
      QStandardItem *child = rootItem->child(i);
      ProjectItem *projectChild = dynamic_cast<ProjectItem*>(child);
      if (projectChild) {
        updateItemIcons(projectChild);
      }
    }
  }

  /**
   * Recursively update icons for project items
   */
  void AstrosetMainWindow::updateItemIcons(ProjectItem *item) {
    if (!item) return;

    QString iconPath;
    QString text = item->text().toLower();

    if (text.contains("image") || text.endsWith(".cub")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/image.svg";
    }
    else if (text.contains("control")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/control.svg";
    }
    else if (text.contains("map")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/map.svg";
    }
    else if (text.contains("registration")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/registration.svg";
    }
    else if (text.contains("shape")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/shape.svg";
    }
    else if (text.contains("target")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/target.svg";
    }
    else if (text.contains("sensor")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/sensor.svg";
    }
    else if (text.contains("spacecraft") || text.contains("space")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/spacecraft.svg";
    }
    else if (text.contains("result")) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/results.svg";
    }
    else if (text == "project") {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/project.svg";
    }
    else if (item->hasChildren()) {
      iconPath = "$ISISROOT/../isis/src/qisis/apps/astroset/icons/folder.svg";
    }

    if (!iconPath.isEmpty()) {
      FileName iconFile(iconPath);
      QIcon icon(iconFile.expanded());
      if (!icon.isNull()) {
        item->setIcon(icon);
      }
    }

    // Set text color to light for dark theme
    item->setForeground(QBrush(QColor(232, 238, 247)));

    // Recursively update children
    for (int i = 0; i < item->rowCount(); i++) {
      ProjectItem *child = static_cast<ProjectItem*>(item->child(i));
      if (child) {
        updateItemIcons(child);
      }
    }
  }
}
