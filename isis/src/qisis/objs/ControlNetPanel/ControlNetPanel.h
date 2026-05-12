#ifndef ControlNetPanel_h
#define ControlNetPanel_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QWidget>
#include <QPointer>

namespace Isis {
  class ControlNet;
  class CnetEditorWidget;
  class WorkspaceProject;

  /**
   * @brief Panel for editing control networks
   *
   * This panel provides control network editing capabilities,
   * wrapping the CnetEditorWidget to provide table-based editing
   * of control points and measures.
   *
   * @author 2026-05-12 Amy Stamile
   *
   * @internal
   */
  class ControlNetPanel : public QWidget {
    Q_OBJECT

  public:
    ControlNetPanel(WorkspaceProject *project, QWidget *parent = nullptr);
    virtual ~ControlNetPanel();

  public slots:
    void onControlNetworkChanged(ControlNet *cnet);
    void onCreateControlNetwork();
    void onLoadControlNetwork();
    void onSaveControlNetwork();

  private:
    void setupUi();

    QPointer<WorkspaceProject> m_project;
    QPointer<CnetEditorWidget> m_cnetEditor;
  };
}

#endif
