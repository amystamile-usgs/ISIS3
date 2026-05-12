/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QApplication>
#include <QCoreApplication>
#include <iostream>

#include "Application.h"
#include "FileName.h"
#include "Gui.h"
#include "IException.h"
#include "IsisWorkspaceMainWindow.h"
#include "Preference.h"
#include "QIsisApplication.h"

using namespace Isis;

int main(int argc, char *argv[]) {
  if (getenv("ISISROOT") == NULL || QString(getenv("ISISROOT")) == "") {
    std::cerr << "Please set ISISROOT before running any Isis applications" << std::endl;
    exit(1);
  }
  Isis::Gui::checkX11();

  try {
    // Add the Qt plugin directory to the library path
    FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
    QCoreApplication::addLibraryPath(qtpluginpath.expanded());

    QIsisApplication *app = new QIsisApplication(argc, argv);
    QApplication::setApplicationName("ISIS Workspace");
    QApplication::setOrganizationName("USGS Astrogeology");
    QApplication::setOrganizationDomain("astrogeology.usgs.gov");

    // Set modern Fusion style as base
    QApplication::setStyle("Fusion");

    // Check for GUI style preference
    PvlGroup &uiPref = Preference::Preferences().findGroup("UserInterface");
    if (uiPref.hasKeyword("GuiStyle")) {
      QString style = uiPref["GuiStyle"];
      QApplication::setStyle(style);
    }

    // Create and show the main window
    IsisWorkspaceMainWindow *mainWindow = new IsisWorkspaceMainWindow();
    mainWindow->show();

    // Load any cubes passed as command line arguments
    for (int i = 1; i < argc; i++) {
      try {
        QString arg(argv[i]);
        if (!arg.startsWith("-")) {
          mainWindow->loadCube(arg);
        }
      }
      catch (IException &e) {
        e.print();
      }
    }

    int status = app->exec();

    delete mainWindow;
    delete app;

    return status;
  }
  catch (IException &e) {
    e.print();
    return 1;
  }
}
