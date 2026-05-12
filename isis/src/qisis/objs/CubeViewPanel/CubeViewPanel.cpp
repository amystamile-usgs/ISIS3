/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubeViewPanel.h"

#include <QVBoxLayout>

#include "Cube.h"
#include "Workspace.h"
#include "WorkspaceProject.h"

namespace Isis {

  /**
   * Constructor
   */
  CubeViewPanel::CubeViewPanel(WorkspaceProject *project, QWidget *parent)
      : QWidget(parent), m_project(project), m_tools(nullptr) {

    // Create the workspace in self-contained mode
    // This means it creates and manages its own tools internally
    m_workspace = new Workspace(true, this);

    // Layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_workspace);

    setLayout(layout);
  }


  /**
   * Destructor
   */
  CubeViewPanel::~CubeViewPanel() {
    // Workspace is a child widget and will be deleted automatically
  }


  /**
   * Get the workspace widget
   */
  Workspace *CubeViewPanel::workspace() const {
    return m_workspace;
  }


  /**
   * Setup tools - not used in self-contained workspace mode
   */
  void CubeViewPanel::setupTools() {
    // Not needed - Workspace(true) creates tools internally
  }


  /**
   * Handle cubes changed in project
   */
  void CubeViewPanel::onCubesChanged() {
    // When cubes are added to the project, add them to the workspace
    QList<Cube *> cubes = m_project->cubes();

    for (Cube *cube : cubes) {
      try {
        m_workspace->addCubeViewport(cube->fileName());
      }
      catch (...) {
        // Cube might already be loaded, ignore error
      }
    }
  }

}
