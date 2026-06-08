#include "CreateControlNetWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "Control.h"
#include "ControlList.h"
#include "ControlNet.h"
#include "Directory.h"
#include "IException.h"
#include "Project.h"
#include "ProjectItem.h"
#include "SurfacePoint.h"

namespace Isis {

  /**
   * @brief Constructs a CreateControlNetWorkOrder
   * @param project The project this work order belongs to
   */
  CreateControlNetWorkOrder::CreateControlNetWorkOrder(Project *project) :
      WorkOrder(project) {
    m_newControl = NULL;

    QAction::setText(tr("Create &New Control Network..."));
    QUndoCommand::setText(tr("Create New Control Network"));
    setModifiesDiskState(true);
  }


  /**
   * @brief Copy constructor
   * @param other The work order to copy
   */
  CreateControlNetWorkOrder::CreateControlNetWorkOrder(const CreateControlNetWorkOrder &other) :
      WorkOrder(other) {
    m_newControl = other.m_newControl;
  }


  /**
   * @brief Destructor
   */
  CreateControlNetWorkOrder::~CreateControlNetWorkOrder() {
  }


  /**
   * @brief Creates a copy of this work order
   * @return A pointer to a copy of this work order
   */
  CreateControlNetWorkOrder *CreateControlNetWorkOrder::clone() const {
    return new CreateControlNetWorkOrder(*this);
  }


  /**
   * @brief Determines if this work order can be executed
   *
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus. Only show "Create New Control Network" when right-clicking
   * on the "Control Networks" node in the project tree.
   *
   * @param item The ProjectItem that was clicked
   * @return True if the user clicked on a project tree node named "Control Networks"
   */
  bool CreateControlNetWorkOrder::isExecutable(ProjectItem *item) {
    // Only show in context menu when clicking on "Control Networks" node
    if (item) {
      return (item->text() == "Control Networks");
    }
    // Not executable from context menu if no item
    return false;
  }


  /**
   * @brief Sets up the work order for execution
   *
   * Prompts the user for a name for the new control network.
   *
   * @return True if setup was successful (user didn't cancel)
   */
  bool CreateControlNetWorkOrder::setupExecution() {
    bool ok = false;
    QString defaultName = QString("ControlNet_%1").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString cnetName = QInputDialog::getText(NULL,
        tr("Create New Control Network"),
        tr("Enter a name for the new control network:"),
        QLineEdit::Normal,
        defaultName,
        &ok);

    if (!ok || cnetName.isEmpty()) {
      return false;
    }

    // Store the name for use in execute()
    QStringList internalData;
    internalData.append(cnetName);
    setInternalData(internalData);

    return true;
  }


  /**
   * @brief Executes the work order
   *
   * Creates a new empty control network and adds it to the project.
   */
  void CreateControlNetWorkOrder::execute() {
    QString cnetName = internalData().at(0);
    ControlNet *newCnet = NULL;
    QString cnetFileName;

    try {
      qDebug() << "CreateControlNetWorkOrder::execute() - Creating control network:" << cnetName;

      // Verify project is valid
      if (!project()) {
        throw IException(IException::Programmer,
                        "Project pointer is null",
                        _FILEINFO_);
      }

      qDebug() << "  - Creating ControlNet object";
      // Create a new empty control network with default coordinate type
      newCnet = new ControlNet(SurfacePoint::Latitudinal);

      qDebug() << "  - Setting mutex";
      // Set mutex - this is critical for thread safety
      QMutex *projectMutex = project()->mutex();
      if (projectMutex) {
        newCnet->SetMutex(projectMutex);
      }
      else {
        qWarning() << "  WARNING: Project mutex is null!";
      }

      qDebug() << "  - Setting network metadata";
      newCnet->SetNetworkId(cnetName);

      // Set username safely
      QString userName = QString::fromLocal8Bit(qgetenv("USER"));
      if (userName.isEmpty()) {
        userName = "Unknown";
      }
      newCnet->SetUserName(userName);

      QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
      newCnet->SetCreatedDate(currentDateTime);
      newCnet->SetModifiedDate(currentDateTime);

      qDebug() << "  - Creating control network folder";
      // Determine where to save it in the project using the project's method
      QDir cnetFolder = project()->addCnetFolder("controlNetworks");
      if (!cnetFolder.exists()) {
        throw IException(IException::Io,
                        "Could not create control network folder",
                        _FILEINFO_);
      }

      cnetFileName = cnetFolder.canonicalPath() + "/" + cnetName + ".net";
      qDebug() << "  - Writing to file:" << cnetFileName;

      // Write the control network to disk
      newCnet->Write(cnetFileName);

      qDebug() << "  - Deleting in-memory ControlNet";
      // Delete the ControlNet and create a Control from the file, like ImportControlNetWorkOrder does
      delete newCnet;
      newCnet = NULL;

      qDebug() << "  - Creating Control object from file";
      // Create a Control object from the file and add it to the project
      m_newControl = new Control(project(), cnetFileName);
      if (!m_newControl) {
        throw IException(IException::Programmer,
                        "Failed to create Control object",
                        _FILEINFO_);
      }

      qDebug() << "  - Closing control net file";
      m_newControl->closeControlNet();

      qDebug() << "  - Adding control to project";
      project()->addControl(m_newControl);

      qDebug() << "  - Setting internal data";
      setInternalData(QStringList(m_newControl->id()));

      qDebug() << "CreateControlNetWorkOrder::execute() - SUCCESS";
    }
    catch (IException &e) {
      qDebug() << "CreateControlNetWorkOrder::execute() - ISIS EXCEPTION:" << e.toString();
      // Clean up if we failed
      if (newCnet) {
        delete newCnet;
        newCnet = NULL;
      }

      QString msg = tr("Failed to create control network: %1").arg(e.toString());
      QMessageBox::critical(NULL, tr("Error"), msg);
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    catch (std::exception &e) {
      qDebug() << "CreateControlNetWorkOrder::execute() - STD EXCEPTION:" << e.what();
      if (newCnet) {
        delete newCnet;
        newCnet = NULL;
      }

      QString msg = tr("Failed to create control network: %1").arg(e.what());
      QMessageBox::critical(NULL, tr("Error"), msg);
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    catch (...) {
      qDebug() << "CreateControlNetWorkOrder::execute() - UNKNOWN EXCEPTION";
      // Catch any other exceptions
      if (newCnet) {
        delete newCnet;
        newCnet = NULL;
      }

      QString msg = tr("Failed to create control network: Unknown error");
      QMessageBox::critical(NULL, tr("Error"), msg);
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * @brief Performs any necessary post-execution operations
   *
   * Like ImportControlNetWorkOrder, call activeControl() which will automatically
   * set the newly created control as active if appropriate (only one control exists
   * and there's an active image list).
   */
  void CreateControlNetWorkOrder::postExecution() {
    qDebug() << "CreateControlNetWorkOrder::postExecution() - START";

    try {
      if (!project()) {
        qDebug() << "  - No project, returning";
        return;
      }

      qDebug() << "  - Calling project()->activeControl()";
      // This will automatically set the control as active if it's the only one
      // and there's an active image list. Same pattern as ImportControlNetWorkOrder.
      project()->activeControl();

      qDebug() << "CreateControlNetWorkOrder::postExecution() - END";
    }
    catch (IException &e) {
      qDebug() << "CreateControlNetWorkOrder::postExecution() - ISIS EXCEPTION:" << e.toString();
    }
    catch (std::exception &e) {
      qDebug() << "CreateControlNetWorkOrder::postExecution() - STD EXCEPTION:" << e.what();
    }
    catch (...) {
      qDebug() << "CreateControlNetWorkOrder::postExecution() - UNKNOWN EXCEPTION";
    }
  }
}
