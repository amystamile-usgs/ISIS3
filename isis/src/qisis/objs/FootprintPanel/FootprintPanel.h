#ifndef FootprintPanel_h
#define FootprintPanel_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QWidget>
#include <QPointer>

namespace Isis {
  class ControlNet;
  class Cube;
  class MosaicSceneWidget;
  class WorkspaceProject;

  /**
   * @brief Panel for visualizing image footprints
   *
   * This panel provides 2D footprint visualization of loaded cubes,
   * showing their spatial coverage and overlaps. It also displays
   * control points when a control network is loaded.
   *
   * @author 2026-05-12 Amy Stamile
   *
   * @internal
   */
  class FootprintPanel : public QWidget {
    Q_OBJECT

  public:
    FootprintPanel(WorkspaceProject *project, QWidget *parent = nullptr);
    virtual ~FootprintPanel();

  public slots:
    void onCubesChanged();
    void onControlNetworkChanged(ControlNet *cnet);

  private:
    void setupUi();

    QPointer<WorkspaceProject> m_project;
    QPointer<MosaicSceneWidget> m_mosaicScene;
  };
}

#endif
