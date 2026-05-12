#ifndef WorkspaceProject_h
#define WorkspaceProject_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QObject>
#include <QList>
#include <QPointer>
#include <QString>

namespace Isis {
  class Cube;
  class ControlNet;

  /**
   * @brief Manages the workspace project data
   *
   * This class handles the data model for the ISIS workspace,
   * managing cubes, control networks, and their relationships.
   * It provides signals to notify UI components when data changes.
   *
   * @author 2026-05-12 Amy Stamile
   *
   * @internal
   */
  class WorkspaceProject : public QObject {
    Q_OBJECT

  public:
    WorkspaceProject(QObject *parent = nullptr);
    virtual ~WorkspaceProject();

    // Cube management
    void addCube(const QString &cubePath);
    void removeCube(Cube *cube);
    void clearCubes();
    QList<Cube *> cubes() const;
    int cubeCount() const;

    // Control network management
    void setControlNetwork(ControlNet *cnet);
    ControlNet *controlNetwork() const;
    bool hasControlNetwork() const;

    // Project I/O
    void save(const QString &filePath);
    void load(const QString &filePath);
    void clear();

    QString projectPath() const;
    bool isModified() const;

  signals:
    void cubesChanged();
    void cubeAdded(Cube *cube);
    void cubeRemoved(Cube *cube);
    void controlNetworkChanged(ControlNet *cnet);
    void projectModified();

  private:
    QList<Cube *> m_cubes;
    ControlNet *m_controlNet;
    QString m_projectPath;
    bool m_isModified;
  };
}

#endif
