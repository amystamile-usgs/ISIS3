/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FootprintPanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include "Cube.h"
#include "ControlNet.h"
#include "MosaicSceneWidget.h"
#include "WorkspaceProject.h"

namespace Isis {

  /**
   * Constructor
   */
  FootprintPanel::FootprintPanel(WorkspaceProject *project, QWidget *parent)
      : QWidget(parent), m_project(project) {

    setupUi();
  }


  /**
   * Destructor
   */
  FootprintPanel::~FootprintPanel() {
  }


  /**
   * Setup the UI components
   */
  void FootprintPanel::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create the mosaic scene widget for footprint display
    // Parameters: statusBar, showTools, internalizeToolBarsAndProgress, directory, parent
    m_mosaicScene = new MosaicSceneWidget(nullptr, true, false, nullptr, this);

    layout->addWidget(m_mosaicScene);

    setLayout(layout);
  }


  /**
   * Handle cubes changed in project
   */
  void FootprintPanel::onCubesChanged() {
    if (!m_mosaicScene) {
      return;
    }

    // Clear existing footprints
    // Note: MosaicSceneWidget handles footprint management internally

    // Add footprints for all loaded cubes
    QList<Cube *> cubes = m_project->cubes();

    // TODO: Implement proper footprint addition to mosaic scene
    // This requires creating DisplayProperties and Image objects
    // as used in IPCE's Footprint2DView
    // For now, the mosaic scene is displayed but empty
    (void)cubes; // Suppress unused variable warning
  }


  /**
   * Handle control network changed
   */
  void FootprintPanel::onControlNetworkChanged(ControlNet *cnet) {
    if (!m_mosaicScene) {
      return;
    }

    // Display control points on the footprint view
    if (cnet) {
      // TODO: Add control points to the mosaic scene
      // The MosaicSceneWidget has support for displaying control points
      // This would involve setting the control network on the scene
    }
  }

}
