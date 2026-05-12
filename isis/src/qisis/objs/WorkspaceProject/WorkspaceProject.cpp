/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "WorkspaceProject.h"

#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Cube.h"
#include "ControlNet.h"
#include "IException.h"

namespace Isis {

  /**
   * Constructor
   */
  WorkspaceProject::WorkspaceProject(QObject *parent)
      : QObject(parent), m_controlNet(nullptr), m_isModified(false) {
  }


  /**
   * Destructor
   */
  WorkspaceProject::~WorkspaceProject() {
    clear();
  }


  /**
   * Add a cube to the project
   *
   * @param cubePath Path to the ISIS cube file
   */
  void WorkspaceProject::addCube(const QString &cubePath) {
    try {
      Cube *cube = new Cube();
      cube->open(cubePath);

      m_cubes.append(cube);
      m_isModified = true;

      emit cubeAdded(cube);
      emit cubesChanged();
      emit projectModified();
    }
    catch (IException &e) {
      throw;
    }
  }


  /**
   * Remove a cube from the project
   *
   * @param cube Pointer to the cube to remove
   */
  void WorkspaceProject::removeCube(Cube *cube) {
    if (m_cubes.removeOne(cube)) {
      emit cubeRemoved(cube);
      emit cubesChanged();
      emit projectModified();

      delete cube;
      m_isModified = true;
    }
  }


  /**
   * Clear all cubes from the project
   */
  void WorkspaceProject::clearCubes() {
    qDeleteAll(m_cubes);
    m_cubes.clear();

    emit cubesChanged();
    emit projectModified();
    m_isModified = true;
  }


  /**
   * Get list of all cubes in the project
   *
   * @return List of cube pointers
   */
  QList<Cube *> WorkspaceProject::cubes() const {
    return m_cubes;
  }


  /**
   * Get the number of cubes in the project
   *
   * @return Number of cubes
   */
  int WorkspaceProject::cubeCount() const {
    return m_cubes.size();
  }


  /**
   * Set the control network for the project
   *
   * @param cnet Pointer to the control network
   */
  void WorkspaceProject::setControlNetwork(ControlNet *cnet) {
    m_controlNet = cnet;
    emit controlNetworkChanged(cnet);
    emit projectModified();
    m_isModified = true;
  }


  /**
   * Get the current control network
   *
   * @return Pointer to the control network, or nullptr if none exists
   */
  ControlNet *WorkspaceProject::controlNetwork() const {
    return m_controlNet;
  }


  /**
   * Check if project has a control network
   *
   * @return True if a control network exists
   */
  bool WorkspaceProject::hasControlNetwork() const {
    return m_controlNet != nullptr;
  }


  /**
   * Save the project to a file
   *
   * @param filePath Path where to save the project
   */
  void WorkspaceProject::save(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
      throw IException(IException::Io,
                      QString("Cannot write to file: %1").arg(filePath),
                      _FILEINFO_);
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("ISISWorkspaceProject");
    stream.writeAttribute("version", "1.0");

    // Write cubes
    stream.writeStartElement("cubes");
    for (Cube *cube : cubes()) {
      stream.writeStartElement("cube");
      stream.writeAttribute("path", cube->fileName());
      stream.writeEndElement();
    }
    stream.writeEndElement(); // cubes

    // Write control network if present
    if (hasControlNetwork()) {
      stream.writeStartElement("controlNetwork");
      // TODO: Add control network path/data
      stream.writeEndElement();
    }

    stream.writeEndElement(); // ISISWorkspaceProject
    stream.writeEndDocument();

    file.close();

    m_projectPath = filePath;
    m_isModified = false;
  }


  /**
   * Load a project from a file
   *
   * @param filePath Path to the project file
   */
  void WorkspaceProject::load(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
      throw IException(IException::Io,
                      QString("Cannot read file: %1").arg(filePath),
                      _FILEINFO_);
    }

    clear();

    QXmlStreamReader stream(&file);

    while (!stream.atEnd()) {
      stream.readNext();

      if (stream.isStartElement()) {
        if (stream.name() == QString("cube")) {
          QString cubePath = stream.attributes().value("path").toString();
          if (!cubePath.isEmpty()) {
            try {
              addCube(cubePath);
            }
            catch (IException &e) {
              // Log error but continue loading other cubes
              e.print();
            }
          }
        }
      }
    }

    if (stream.hasError()) {
      throw IException(IException::Io,
                      QString("Error parsing project file: %1").arg(filePath),
                      _FILEINFO_);
    }

    file.close();

    m_projectPath = filePath;
    m_isModified = false;
  }


  /**
   * Clear the entire project
   */
  void WorkspaceProject::clear() {
    clearCubes();
    if (m_controlNet) {
      delete m_controlNet;
      m_controlNet = nullptr;
    }
    m_projectPath.clear();
    m_isModified = false;
  }


  /**
   * Get the current project file path
   *
   * @return Project file path, or empty string if not saved
   */
  QString WorkspaceProject::projectPath() const {
    return m_projectPath;
  }


  /**
   * Check if the project has been modified since last save
   *
   * @return True if modified
   */
  bool WorkspaceProject::isModified() const {
    return m_isModified;
  }

}
