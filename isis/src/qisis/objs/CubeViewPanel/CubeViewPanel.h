#ifndef CubeViewPanel_h
#define CubeViewPanel_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QWidget>
#include <QPointer>

namespace Isis {
  class Cube;
  class Workspace;
  class WorkspaceProject;
  class ToolList;
  class Tool;

  /**
   * @brief Panel for viewing ISIS cubes with analysis tools
   *
   * This panel wraps qview's Workspace and provides cube viewing
   * capabilities with all the standard ISIS tools (zoom, pan, stretch,
   * statistics, plotting, etc.).
   *
   * @author 2026-05-12 Amy Stamile
   *
   * @internal
   */
  class CubeViewPanel : public QWidget {
    Q_OBJECT

  public:
    CubeViewPanel(WorkspaceProject *project, QWidget *parent = nullptr);
    virtual ~CubeViewPanel();

    Workspace *workspace() const;

  public slots:
    void onCubesChanged();

  private:
    void setupTools();

    QPointer<WorkspaceProject> m_project;
    QPointer<Workspace> m_workspace;
    ToolList *m_tools;
  };
}

#endif
