/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetPanel.h"

#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

#include "CnetEditorWidget.h"
#include "Control.h"
#include "ControlNet.h"
#include "WorkspaceProject.h"

namespace Isis {

  /**
   * Constructor
   */
  ControlNetPanel::ControlNetPanel(WorkspaceProject *project, QWidget *parent)
      : QWidget(parent), m_project(project) {

    setupUi();
  }


  /**
   * Destructor
   */
  ControlNetPanel::~ControlNetPanel() {
  }


  /**
   * Setup the UI components
   */
  void ControlNetPanel::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Toolbar for control network actions
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setObjectName("ControlNetToolBar");

    QPushButton *createBtn = new QPushButton("Create Network", this);
    connect(createBtn, &QPushButton::clicked, this, &ControlNetPanel::onCreateControlNetwork);
    toolBar->addWidget(createBtn);

    QPushButton *loadBtn = new QPushButton("Load Network", this);
    connect(loadBtn, &QPushButton::clicked, this, &ControlNetPanel::onLoadControlNetwork);
    toolBar->addWidget(loadBtn);

    QPushButton *saveBtn = new QPushButton("Save Network", this);
    connect(saveBtn, &QPushButton::clicked, this, &ControlNetPanel::onSaveControlNetwork);
    toolBar->addWidget(saveBtn);

    layout->addWidget(toolBar);

    // Placeholder for control network editor
    // The CnetEditorWidget will be created when a control network is loaded
    QLabel *placeholder = new QLabel(
        "No control network loaded.\n\n"
        "Create a new network or load an existing one to begin editing.",
        this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("QLabel { color: #888; font-size: 12pt; }");
    layout->addWidget(placeholder);

    setLayout(layout);
  }


  /**
   * Handle control network changed
   */
  void ControlNetPanel::onControlNetworkChanged(ControlNet *cnet) {
    if (cnet) {
      // Remove old editor if it exists
      if (m_cnetEditor) {
        layout()->removeWidget(m_cnetEditor);
        m_cnetEditor->deleteLater();
      }

      // Create new editor widget for this control network
      Control *control = new Control(cnet, "control");
      m_cnetEditor = new CnetEditorWidget(control, "");
      m_cnetEditor->setParent(this);

      // Add to layout (remove placeholder first)
      QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
      if (mainLayout && mainLayout->count() > 1) {
        QLayoutItem *item = mainLayout->takeAt(1);
        if (item->widget()) {
          item->widget()->deleteLater();
        }
        delete item;
      }

      layout()->addWidget(m_cnetEditor);
    }
  }


  /**
   * Create a new control network
   */
  void ControlNetPanel::onCreateControlNetwork() {
    if (!m_project->cubes().isEmpty()) {
      // Create a new empty control network
      ControlNet *cnet = new ControlNet();
      cnet->SetNetworkId("New Network");
      cnet->SetUserName("ISIS Workspace User");
      cnet->SetDescription("Control network created in ISIS Workspace");

      m_project->setControlNetwork(cnet);

      QMessageBox::information(this, "Control Network Created",
                              "A new control network has been created.");
    }
    else {
      QMessageBox::warning(this, "No Cubes Loaded",
                          "Please load cubes before creating a control network.");
    }
  }


  /**
   * Load an existing control network
   */
  void ControlNetPanel::onLoadControlNetwork() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Load Control Network"),
        QString(),
        tr("ISIS Control Networks (*.net *.cnet);;All Files (*)"));

    if (!fileName.isEmpty()) {
      try {
        ControlNet *cnet = new ControlNet(fileName);
        m_project->setControlNetwork(cnet);

        QMessageBox::information(this, "Control Network Loaded",
                                tr("Loaded control network: %1").arg(fileName));
      }
      catch (...) {
        QMessageBox::critical(this, "Error Loading Control Network",
                            tr("Failed to load control network: %1").arg(fileName));
      }
    }
  }


  /**
   * Save the current control network
   */
  void ControlNetPanel::onSaveControlNetwork() {
    if (!m_project->hasControlNetwork()) {
      QMessageBox::warning(this, "No Control Network",
                          "No control network to save.");
      return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Control Network"),
        QString(),
        tr("ISIS Control Networks (*.net);;All Files (*)"));

    if (!fileName.isEmpty()) {
      try {
        m_project->controlNetwork()->Write(fileName);

        QMessageBox::information(this, "Control Network Saved",
                                tr("Saved control network: %1").arg(fileName));
      }
      catch (...) {
        QMessageBox::critical(this, "Error Saving Control Network",
                            tr("Failed to save control network: %1").arg(fileName));
      }
    }
  }

}
