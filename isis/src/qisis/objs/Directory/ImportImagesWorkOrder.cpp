/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "ImportImagesWorkOrder.h"

#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QtConcurrentMap>
#include <QVBoxLayout>

#include "Camera.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"
#include "SaveProjectWorkOrder.h"
#include "Target.h"
#include "TextFile.h"

namespace Isis {

  /**
   * @brief Creates an asynchronous WorkOrder for importing images to the project.
   *
   * @param Project *project Project to import images into.
   */
  ImportImagesWorkOrder::ImportImagesWorkOrder(Project *project) :
      WorkOrder(project) {
    // This is an asynchronous work order
    m_isSynchronous = false;
    m_newImages = NULL;
    m_list = NULL;

    QAction::setText(tr("Import &Images..."));
    QUndoCommand::setText(tr("Import Images"));
    setModifiesDiskState(true);
  }


  /**
   * @brief Copies the WorkOrder.
   *
   * @param ImportImagesWorkOrder &other The other work order to copy state from.
   */
  ImportImagesWorkOrder::ImportImagesWorkOrder(const ImportImagesWorkOrder &other) :
      WorkOrder(other) {
    m_newImages = NULL;
    m_list = other.m_list;
  }


  /**
   * @brief Destructor.
   *
   * Releases the memory for the m_newImages member.
   */
  ImportImagesWorkOrder::~ImportImagesWorkOrder() {
    delete m_newImages;
    m_newImages = NULL;

    m_list = NULL;
  }


  /**
   * @brief Creates a clone of this work order.
   *
   * @see WorkOrder::clone()
   *
   * @return ImportImagesWorkOrder* Returns a pointer to the newly cloned work order.
   */
  ImportImagesWorkOrder *ImportImagesWorkOrder::clone() const {
    return new ImportImagesWorkOrder(*this);
  }


  /**
   * This method returns true if the user clicked on a project tree node with the text "Images".
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus.
   *
   * @param item The ProjectItem that was clicked
   *
   * @return bool True if the user clicked on a project tree node named "Shapes"
   */
  bool ImportImagesWorkOrder::isExecutable(ProjectItem *item) {

      if (item) {
        return (item->text() == "Images");
      }

     return false;
  }


  /**
   * @brief Sets up this work order before being executed.
   *
   * First invokes WorkOrder's setupExecution(). Prompts the user for cubes and image
   * list files to import and stores them via a setInternalData() call. If there are more than 100
   * images to import, the user is prompted if they want to save their project before the import
   * occurs. If yes, a SaveProjectWorkOrder will be executed. This setup is considered successful
   * if the user does not hit cancel on a dialog prompt and if there is at least one image has been
   * selected by the user to import. This method was renamed from execute() to setupExecution()
   * according to the WorkOrder redesign.
   *
   * @see WorkOrder::setupExecution()
   *
   * @return bool Returns true if the setup was successful.
   */
  bool ImportImagesWorkOrder::setupExecution() {
    try {
      WorkOrder::setupExecution();

      QStringList fileNames = QFileDialog::getOpenFileNames(
          qobject_cast<QWidget *>(parent()),
          tr("Import Images"), "",
          tr("Isis cubes and list files (*.cub *.lis);;All Files (*)"));

      QStringList* stateToSave = new QStringList();

      qDebug() << "ImportImages: Selected files:" << fileNames.count();

      if (!fileNames.isEmpty()) {
        foreach (FileName fileName, fileNames) {
          qDebug() << "  Processing file:" << fileName.original();
          if (fileName.extension() == "lis") {
            TextFile listFile(fileName.expanded());
            QString path = fileName.path();
            QString lineOfListFile;

            while (listFile.GetLine(lineOfListFile)) {
              FileName relFileName(path + "/" + lineOfListFile);
              if (relFileName.fileExists() ) {
                stateToSave->append(path + "/" + lineOfListFile);
                qDebug() << "    Added from list (relative):" << (path + "/" + lineOfListFile);
              }
              else {
                FileName absFileName(lineOfListFile);
                if ( absFileName.fileExists() && lineOfListFile.startsWith("/") ) {
                  stateToSave->append(lineOfListFile);
                  qDebug() << "    Added from list (absolute):" << lineOfListFile;
                }

                else {
                  project()->warn("File " + lineOfListFile + " not found");
                }
              }
            }
          }
          else {
            stateToSave->append(fileName.original());
            qDebug() << "    Added cube:" << fileName.original();
          }
        }

        qDebug() << "ImportImages: Total images to import:" << stateToSave->count();

        // Show modern options dialog first to know if user wants workspace mode
        bool copyDnData = false;
        bool generateFootprints = false;
        bool createWorkspace = false;

        if (!stateToSave->isEmpty()) {
          // Check if we already have images in the project - if so, enforce the same mode
          bool hasExistingImages = !project()->images().isEmpty();
          bool existingIsLightweight = project()->usesLightweightMode();

          // Create inline dialog
          QDialog optionsDialog(qobject_cast<QWidget *>(parent()));
          optionsDialog.setWindowTitle(tr("Import Images - Options"));
          optionsDialog.setModal(true);
          optionsDialog.resize(550, 400);

          QVBoxLayout *mainLayout = new QVBoxLayout(&optionsDialog);
          mainLayout->setSpacing(20);
          mainLayout->setContentsMargins(20, 20, 20, 20);

          // Header
          QLabel *headerLabel = new QLabel(
              tr("Importing <b>%1 image%2</b>").arg(stateToSave->count())
                  .arg(stateToSave->count() == 1 ? "" : "s"),
              &optionsDialog);
          QFont headerFont = headerLabel->font();
          headerFont.setPointSize(14);
          headerLabel->setFont(headerFont);
          mainLayout->addWidget(headerLabel);

          QLabel *subtitleLabel = new QLabel(
              tr("Configure import options below. These settings affect import speed and functionality."),
              &optionsDialog);
          subtitleLabel->setWordWrap(true);
          mainLayout->addWidget(subtitleLabel);

          // Footprint option
          QGroupBox *footprintGroup = new QGroupBox(tr("Footprint Generation"), &optionsDialog);
          QVBoxLayout *footprintLayout = new QVBoxLayout(footprintGroup);

          QCheckBox *footprintsCheckbox = new QCheckBox(
              tr("Generate image footprints during import"), &optionsDialog);
          footprintsCheckbox->setChecked(false);  // Default: disabled for performance

          QLabel *footprintInfo = new QLabel(
              tr("<b>Recommended: Disabled</b> for faster imports<br><br>"
                 "Image footprints are used by the Footprint2D view to display image coverage. "
                 "Generating footprints requires opening each cube and calculating its ground coverage, "
                 "which can be <b>very slow</b> for large datasets (minutes per thousand images).<br><br>"
                 "<b>Note:</b> You can generate footprints later by running <i>footprintinit</i> "
                 "if you need the Footprint2D view."),
              &optionsDialog);
          footprintInfo->setWordWrap(true);

          footprintLayout->addWidget(footprintsCheckbox);
          footprintLayout->addWidget(footprintInfo);
          mainLayout->addWidget(footprintGroup);

          // Copy images option
          QGroupBox *copyGroup = new QGroupBox(tr("Image Data"), &optionsDialog);
          QVBoxLayout *copyLayout = new QVBoxLayout(copyGroup);

          QCheckBox *copyCheckbox = new QCheckBox(
              tr("Copy image data (DN values) into project"), &optionsDialog);
          copyCheckbox->setChecked(false);  // Default: no copy

          QLabel *copyInfo = new QLabel(
              tr("<b>Recommended: Disabled</b> to save disk space<br><br>"
                 "If disabled, creates lightweight .ecub files that reference the original cubes. "
                 "This is faster and saves disk space but requires original files to remain accessible.<br><br>"
                 "Enable this only if you need a self-contained project or if source images will be moved/deleted."),
              &optionsDialog);
          copyInfo->setWordWrap(true);

          copyLayout->addWidget(copyCheckbox);
          copyLayout->addWidget(copyInfo);
          mainLayout->addWidget(copyGroup);

          // Project structure option
          QGroupBox *structureGroup = new QGroupBox(tr("Project Structure"), &optionsDialog);
          QVBoxLayout *structureLayout = new QVBoxLayout(structureGroup);

          QCheckBox *createWorkspaceCheckbox = new QCheckBox(
              tr("Create project workspace structure"), &optionsDialog);

          QString structureInfoText;
          if (hasExistingImages) {
            // Force same mode as existing images
            createWorkspaceCheckbox->setChecked(existingIsLightweight ? false : true);
            createWorkspaceCheckbox->setEnabled(false);  // Can't change mode
            structureInfoText = existingIsLightweight
              ? tr("<b>Locked to Lightweight Mode</b><br><br>"
                   "This project already has images imported in lightweight mode. "
                   "All subsequent imports must use the same mode for consistency.")
              : tr("<b>Locked to Workspace Mode</b><br><br>"
                   "This project already has images imported in workspace mode. "
                   "All subsequent imports must use the same mode for consistency.");
          }
          else {
            // First import - user can choose
            createWorkspaceCheckbox->setChecked(false);  // Default: lightweight mode
            structureInfoText = tr("<b>Recommended: Disabled</b> for maximum flexibility<br><br>"
                   "If disabled (lightweight mode), images are referenced in place without creating "
                   "project folders or .ecub files. This is much faster and lets you keep your own organization.<br><br>"
                   "If enabled (workspace mode), creates project/images/importN/ folders and .ecub files. "
                   "Only needed if you want a traditional project structure.");
          }

          QLabel *structureInfo = new QLabel(structureInfoText, &optionsDialog);
          structureInfo->setWordWrap(true);

          structureLayout->addWidget(createWorkspaceCheckbox);
          structureLayout->addWidget(structureInfo);
          mainLayout->addWidget(structureGroup);

          // Connect workspace checkbox to auto-enable copy checkbox
          connect(createWorkspaceCheckbox, &QCheckBox::toggled, [copyCheckbox, copyInfo](bool checked) {
            if (checked) {
              // Workspace mode requires copying DN data
              copyCheckbox->setChecked(true);
              copyCheckbox->setEnabled(false);
              copyInfo->setText(tr("<b>Required for Workspace Mode</b><br><br>"
                                   "Workspace mode requires copying DN data into the project structure. "
                                   "This creates a self-contained project that can be saved and moved."));
            }
            else {
              // Lightweight mode - copy is optional
              copyCheckbox->setEnabled(true);
              copyInfo->setText(tr("<b>Recommended: Disabled</b> to save disk space<br><br>"
                                   "If disabled, creates lightweight .ecub files that reference the original cubes. "
                                   "This is faster and saves disk space but requires original files to remain accessible.<br><br>"
                                   "Enable this only if you need a self-contained project or if source images will be moved/deleted."));
            }
          });

          // Trigger the connection to set initial state
          if (createWorkspaceCheckbox->isChecked()) {
            copyCheckbox->setChecked(true);
            copyCheckbox->setEnabled(false);
            copyInfo->setText(tr("<b>Required for Workspace Mode</b><br><br>"
                                 "Workspace mode requires copying DN data into the project structure. "
                                 "This creates a self-contained project that can be saved and moved."));
          }

          mainLayout->addStretch();

          // Buttons
          QDialogButtonBox *buttonBox = new QDialogButtonBox(
              QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
              &optionsDialog);
          connect(buttonBox, &QDialogButtonBox::accepted, &optionsDialog, &QDialog::accept);
          connect(buttonBox, &QDialogButtonBox::rejected, &optionsDialog, &QDialog::reject);
          mainLayout->addWidget(buttonBox);

          // Show dialog and get results
          if (optionsDialog.exec() == QDialog::Accepted) {
            copyDnData = copyCheckbox->isChecked();
            generateFootprints = footprintsCheckbox->isChecked();
            createWorkspace = createWorkspaceCheckbox->isChecked();
          }
          else {
            // User cancelled
            return false;
          }
        }

        // Only prompt to save project if importing many images AND creating a workspace
        // Lightweight mode doesn't need project saving before import
        QMessageBox::StandardButton saveProjectAnswer = QMessageBox::No;
        if (stateToSave->count() >= 100 && project()->isTemporaryProject() && createWorkspace) {
          saveProjectAnswer = QMessageBox::question(qobject_cast<QWidget *>(parent()),
                   tr("Save Project Before Importing Images"),
                   tr("You are importing a large number of images with workspace mode enabled. "
                      "Would you like to save your project <b>before</b> importing? "
                      "This is recommended when creating a workspace for large datasets."),
                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                   QMessageBox::Yes);
        }

        if (saveProjectAnswer == QMessageBox::Yes) {
          SaveProjectWorkOrder saveWorkOrder(project());
          saveWorkOrder.trigger();
        }

        if (saveProjectAnswer == QMessageBox::Cancel) {
          return false;
        }

        // Store options in internal data: [ copy|nocopy, footprints|nofootprints, workspace|lightweight, img1, img2, ... ]
        stateToSave->prepend(createWorkspace ? "workspace" : "lightweight");
        stateToSave->prepend(generateFootprints ? "footprints" : "nofootprints");
        stateToSave->prepend(copyDnData ? "copy" : "nocopy");

        // Calculate actual image count (subtract the 3 option strings we just prepended)
        int imageCount = stateToSave->count() - 3;

        if (fileNames.count() > 1) {
          QUndoCommand::setText(tr("Import %1 Images").arg(imageCount));
        }
        else if (fileNames.count() == 1 && imageCount > 1) {
          QUndoCommand::setText(tr("Import %1 Images from %2").arg(imageCount).arg(fileNames.first()));
        }
        else {
          QUndoCommand::setText(tr("Import %1").arg(fileNames.first()));
        }

        // The internal data will look like: [ copy|nocopy, footprints|nofootprints, workspace|lightweight, img1, img2, ... ]
        setInternalData(*stateToSave);

        bool doImport = stateToSave->count() > 3;

        return doImport;
      }

    }
    catch (IException &e) {
      QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
    return false;

  }


  /**
   * @brief Undoes the work order's execute.
   *
   * After this ImportImagesWorkOrder has executed and finished (all the images have
   * been read), this removes the images from this import from disk in the project's directory.
   * This was renamed from asyncUndo() to undoExecution() according to the WorkOrder redesign.
   *
   * @see WorkOrder::undoExecution()
   */
  void ImportImagesWorkOrder::undoExecution() {
    if (m_list && project()->images().size() > 0 ) {
      project()->waitForImageReaderFinished();
      // Remove the images from disk.
      m_list->deleteFromDisk( project() );
      // Remove the images from the model, which updates the tree view.
      ProjectItem *currentItem =
          project()->directory()->model()->findItemData( QVariant::fromValue(m_list) );
      project()->directory()->model()->removeItem(currentItem);
    }
  }


  /**
   * @brief Cleans up memory (images) after the undo execution occurs.
   *
   * After the undoExecution() occurs, this cleans up memory that was allocated for
   * the images from this import. This was renamed from postSyncUndo() to postUndoExecution()
   * according to the WorkOrder redesign.
   *
   * @see WorkOrder::postUndoExecution()
   */
  void ImportImagesWorkOrder::postUndoExecution() {
    if (m_list && project()->images().size() > 0 ) {
      foreach (Image *image, *m_list) {
        delete image;
      }
      delete m_list;
    }
  }


  /**
   * @brief Executes the work order.
   *
   * This actually "does" the work order task. In this case, this imports the images
   * into memory and copies any necessary data to disk. This was renamed from asyncRedo() to
   * execute() according to the WorkOrder redesign.
   *
   * @see ImportImagesWorkOrder::importConfirmedImages(QStringList confirmedImages, bool copyDnData)
   * @see WorkOrder::execute()
   */
  void ImportImagesWorkOrder::execute() {
    try {
      QObject tmpObj;
      if (internalData().count() > 0) {
        // Recall in setupExecution() that internal data format is:
        // [ copy|nocopy, footprints|nofootprints, workspace|lightweight, img1, img2, ... ]
        bool copyDnData = (internalData()[0] == "copy");
        bool generateFootprints = (internalData()[1] == "footprints");
        bool createWorkspace = (internalData()[2] == "workspace");

        importConfirmedImages(internalData().mid(3), copyDnData, generateFootprints, createWorkspace);
        project()->setClean(false);
      }
    }
    catch (IException &e) {
        QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }


  /**
   * @brief Associates the imported images to the project.
   *
   * After execute finishes, associates the imported images to the project. This will
   * also notify the project if there are any warnings that occurred related to the import. This
   * was renamed from postSyncRedo() to postExecution() according to the WorkOrder redesign.
   *
   * @see Project::addImages(Imagelist newImages)
   * @see WorkOrder::postExecution()
   */
  void ImportImagesWorkOrder::postExecution() {
    try {
      if (!m_newImages->isEmpty()) {
        project()->addImages(*m_newImages);
        m_list = project()->images().last();

        delete m_newImages;
        m_newImages = NULL;
      }
    }
    catch (IException &e) {
      m_status = WorkOrderFinished;
      m_warning.append(e.what());
    }
    if (m_warning != "") {
      project()->warn(m_warning);
    }
  }


  /**
   * @brief Creates the internal functor.
   *
   * This functor is used for copying an image to be imported into the project.
   *
   * @param QThread *guiThread Pointer to the thread that this ImportImagesWorkOrder lives in (was
   * created in).
   * @param QDir destinationFolder Where to copy the image to.
   * @param bool copyDnData Indicates whether or not to copy the image data or just the labels.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      QThread *guiThread, QDir destinationFolder, bool copyDnData) : m_errors(new IException),
      m_numErrors(new int(0)) {
    m_destinationFolder = destinationFolder;
    m_copyDnData = copyDnData;
    m_guiThread = guiThread;
  }


  /**
   * Copies the other functor object (copy constructor).
   *
   * @param OriginalFileToProjectCubeFunctor &other The other functor to copy state from.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      const OriginalFileToProjectCubeFunctor &other) : m_errors(other.m_errors),
      m_numErrors(other.m_numErrors) {
    m_destinationFolder = other.m_destinationFolder;
    m_copyDnData = other.m_copyDnData;
    m_guiThread = other.m_guiThread;
  }


  /**
   * @brief Destructor.
   *
   * Resets the internal members to default values.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::~OriginalFileToProjectCubeFunctor() {
    m_destinationFolder = QDir();
    m_copyDnData = false;
    m_guiThread = NULL;
  }


  /**
   * @brief Overloads the callable operator to invoke this functor.
   *
   * Copies an image to be imported for this ImportImagesWorkOrder into the
   * associated project. If we are not copying the image data, the a .ecub file will be created that
   * points to the original cube. Otherwise, a .cub will be copied into the project and a .ecub
   * will be created in the project that references the copied cube.
   * Note that if too many errors occur, the copying
   * will not proceed for remaining images in the import and a NULL pointer will be returned.
   *
   * @param FileName &original Original file name of the image to be copied.
   *
   * @return Cube* Returns a copy of the original cube.
   */
  Cube *ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::operator()(
      const FileName &original) {
    Cube *result = NULL;

    // As long as we haven't encountered 20 errors related to importing images, we can continue
    // to import images.
    if (*m_numErrors < 20) {
      try {
        QString destination = QFileInfo(m_destinationFolder, original.name())
                                .absoluteFilePath();
        Cube *input = new Cube(original, "r");

        if (m_copyDnData) {
          Cube *copiedCube = input->copy(destination, CubeAttributeOutput());
          delete input;
          input = copiedCube;
        }

        FileName externalLabelFile(destination);
        externalLabelFile = externalLabelFile.setExtension("ecub");

        Cube *projectImage = input->copy(externalLabelFile, CubeAttributeOutput("+External"));

        if (m_copyDnData) {
          // Make sure the external label has a fully relative path to the DN data
          projectImage->relocateDnData(FileName(destination).name());
        }

        //  Set new ecub to readOnly.  When closing cube, the labels were being re-written because
        // the cube was read/write. This caused a segfault when imported large number of images
        // because of a label template file being opened too many times.
        projectImage->reopen();

        delete input;

        result = projectImage;
      }
      // When we encounter an exception, update the m_errors and m_numErrors to with the exception
      // that occurred.
      catch (IException &e) {
        m_errorsLock.lock();

        m_errors->append(e);
        (*m_numErrors)++;

        m_errorsLock.unlock();
      }
    }

    return result;
  }


  /**
   * @brief Indicates if any errors occurred during the import.
   *
   * Returns an IException that details any errors that occurred during the import.
   * Note that if there have been 20 or more errors, the exception returned will indicate that the
   * import was aborted because too many errors have occurred.
   *
   * @return IExecption Returns an IException indicating what errors occured during the import.
   */
  IException ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    if (*m_numErrors >= 20) {
      result.append(
          IException(IException::Unknown,
                     tr("Aborted import images due to a high number of errors"),
                     _FILEINFO_));
    }
    return result;
  }


  /**
   * @brief Imports the images.
   *
   * In workspace mode: Creates a project image folder and copies the cubes into it, creating
   * *.ecub and optionally *.cub files inside the project.
   *
   * In lightweight mode: References cubes in place without creating project folders or .ecub files.
   *
   * This should be called in a non-GUI thread.
   *
   * @param confirmedImages This is a list of cube file names to import
   * @param copyDnData If true, copies *.cub files into project (only in workspace mode)
   * @param generateFootprints If true, footprints will be generated during import
   * @param createWorkspace If true, creates project folders and .ecub files (workspace mode).
   *                        If false, references cubes in place (lightweight mode).
   */
  void ImportImagesWorkOrder::importConfirmedImages(QStringList confirmedImages, bool copyDnData,
                                                     bool generateFootprints, bool createWorkspace) {
    try {
      if (!confirmedImages.isEmpty()) {

        // LIGHTWEIGHT MODE: Just reference cubes in place, no workspace structure
        if (!createWorkspace) {
          // Enable lightweight mode on the project
          project()->setLightweightMode(true);

          setProgressRange(0, confirmedImages.count());

          m_newImages = new ImageList;
          m_newImages->reserve(confirmedImages.count());

          QStringList confirmedImagesFileNames;
          QStringList confirmedImagesIds;

          foreach (QString confirmedImage, confirmedImages) {
            QStringList fileNameAndId = confirmedImage.split(",");
            confirmedImagesFileNames.append(fileNameAndId.first());

            if (fileNameAndId.count() == 2) {
              confirmedImagesIds.append(fileNameAndId.last());
            }
            else {
              confirmedImagesIds.append(QString());
            }
          }

          // Import images directly without copying or creating .ecub files
          for (int i = 0; i < confirmedImagesFileNames.count(); i++) {
            setProgressValue(i);

            try {
              QString originalPath = confirmedImagesFileNames[i];
              Cube *cube = new Cube(originalPath, "r");

              // Add target and camera if needed
              QString instrumentId = cube->label()->findGroup("Instrument",
                                PvlObject::FindOptions::Traverse).findKeyword("InstrumentId")[0];
              QString targetName = cube->label()->findGroup("Instrument",
                                PvlObject::FindOptions::Traverse).findKeyword("TargetName")[0];

              if (!project()->hasTarget(targetName)) {
                Camera *camera = cube->camera();
                Target *target = camera->target();
                project()->addTarget(target);

                if (!project()->hasCamera(instrumentId)) {
                  project()->addCamera(camera);
                }
              }
              else if (!project()->hasCamera(instrumentId)) {
                Camera *camera = cube->camera();
                project()->addCamera(camera);
              }

              // Create Image directly from cube
              Image *newImage = new Image(cube);

              // Generate footprint if requested (before closing cube)
              if (generateFootprints) {
                try {
                  if (!newImage->initFootprint(project()->mutex())) {
                    m_warning.append(tr("Could not generate footprint for %1\n")
                        .arg(newImage->displayProperties()->displayName()));
                  }
                }
                catch (IException &e) {
                  m_warning.append(tr("Footprint error for %1: %2\n")
                      .arg(newImage->displayProperties()->displayName())
                      .arg(e.what()));
                }
              }

              newImage->closeCube();
              cube = NULL;

              // Set or use provided ID
              if (confirmedImagesIds[i].isEmpty()) {
                confirmedImagesIds[i] = newImage->id();
              }
              else {
                newImage->setId(confirmedImagesIds[i]);
              }

              m_newImages->append(newImage);

              // Move to GUI thread
              newImage->moveToThread(thread());
              newImage->displayProperties()->moveToThread(thread());
            }
            catch (IException &e) {
              m_warning.append(tr("Error importing %1: %2\n")
                  .arg(confirmedImagesFileNames[i])
                  .arg(e.what()));
            }
          }

          m_newImages->moveToThread(thread());

          // No internal data update needed for lightweight mode - just use original paths
          return;
        }

        // WORKSPACE MODE: Original behavior - create folders and .ecub files
        QDir folder = project()->addImageFolder("import");

        setProgressRange(0, confirmedImages.count());

        // We are creating a new QObject within an asynchronous execute(), which means that this
        // variable, m_newImages, has thread affinity with a thread in the gloabal thread pool
        // (i.e. m_newImages lives in a thread in the global thread pool).
        // see WorkOrder::redo().
        m_newImages = new ImageList;
        m_newImages->reserve(confirmedImages.count());

        QStringList confirmedImagesFileNames;
        QStringList confirmedImagesIds;

        foreach (QString confirmedImage, confirmedImages) {
          QStringList fileNameAndId = confirmedImage.split(",");
          confirmedImagesFileNames.append(fileNameAndId.first());

          // Determine if there was already a unique id provided for the file.
          if (fileNameAndId.count() == 2) {
            confirmedImagesIds.append(fileNameAndId.last());
          }
          else {
            confirmedImagesIds.append(QString());
          }
        }

        OriginalFileToProjectCubeFunctor functor(thread(), folder, copyDnData);
        // Start concurrently copying the images to import.
        QFuture<Cube *> future = QtConcurrent::mapped(confirmedImagesFileNames, functor);

        // The new internal data will store the copied files as well as their associated unique id's.
        QStringList newInternalData;
        newInternalData.append(internalData().first());

        // By releasing a thread from the global thread pool, we are effectively temporarily
        // increasing the max number of available threads. This is useful when a thread goes to sleep
        // waiting for more work, so we can allow other threads to continue.
        // See Qt's QThreadPool::releaseThread() documentation.
        QThreadPool::globalInstance()->releaseThread();
        for (int i = 0; i < confirmedImages.count(); i++) {
          setProgressValue(i);

          // This will wait for the result at i to finish (the functor invocation finishes) and
          // get the cube.
          Cube *cube = future.resultAt(i);

          if (cube) {

            // Confirm that the target body and the gui camera do not exist before creating and 
            // and adding them for each image. Since a target may be covered by many cameras and a 
            // camera may cover many targets, have to get tricky with the checking.
            QString instrumentId = cube->label()->findGroup("Instrument", 
                              PvlObject::FindOptions::Traverse).findKeyword("InstrumentId")[0];
            QString targetName = cube->label()->findGroup("Instrument", 
                              PvlObject::FindOptions::Traverse).findKeyword("TargetName")[0];
            if (!project()->hasTarget(targetName)) {
              Camera *camera = cube->camera();
              Target *target = camera->target();
              project()->addTarget(target);
              
              if (!project()->hasCamera(instrumentId)) {
                project()->addCamera(camera);
              }
            }
            else if (!project()->hasCamera(instrumentId)) {
              Camera *camera = cube->camera();
              project()->addCamera(camera);
            }

            // Create a new image from the result in the thread spawned in WorkOrder::redo().
            Image *newImage = new Image(cube);

            // Generate footprint if requested (do this before closing cube)
            if (generateFootprints) {
              try {
                if (!newImage->initFootprint(project()->mutex())) {
                  m_warning.append(tr("Could not generate footprint for %1\n")
                      .arg(newImage->displayProperties()->displayName()));
                }
              }
              catch (IException &e) {
                m_warning.append(tr("Footprint error for %1: %2\n")
                    .arg(newImage->displayProperties()->displayName())
                    .arg(e.what()));
              }
            }

            newImage->closeCube();
            // Memory for cube is deleted in Image::closeCube()
            cube = NULL;

            // Either use a unique id that was already provided or create one for the new image.
            if (confirmedImagesIds[i].isEmpty()) {
              confirmedImagesIds[i] = newImage->id();
            }
            else {
              newImage->setId(confirmedImagesIds[i]);
            }

            QStringList imageInternalData;
            imageInternalData.append(confirmedImagesFileNames[i]);
            imageInternalData.append(confirmedImagesIds[i]);

            newInternalData.append(imageInternalData.join(","));

            m_newImages->append(newImage);

            // Move the new image back and its display properities to the GUI thread.
            // Note: thread() returns the GUI thread because this ImportImagesWorkOrder lives
            // (was created) in the GUI thread.
            newImage->moveToThread(thread());
            newImage->displayProperties()->moveToThread(thread());
          }
        }
        // Since we temporarily increased the max thread count (by releasing a thread), make sure
        // to re-reserve the thread for the global thread pool's accounting.
        // See Qt's QThreadPool::reserveThread().
        QThreadPool::globalInstance()->reserveThread();

        m_warning = functor.errors().toString();

        // Recall that m_newImages has thread affinity with a thread in the global thread pool.
        // Move it to the GUI-thread because these threads in the pool do not run in an event loop,
        // so they cannot process events.
        // See https://doc.qt.io/qt-5/threads-qobject.html#per-thread-event-loop
        // See http://doc.qt.io/qt-5/threads-technologies.html#comparison-of-solutions
        m_newImages->moveToThread(thread());

        if (m_newImages->isEmpty()) {
          folder.removeRecursively();
        }

        setInternalData(newInternalData);
      }
    }
    catch (IException &e) {
        QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }
}
