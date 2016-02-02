/*
 * NewModule.cpp
 *
 *  Created on: Oct 9, 2015
 */

#include "NewModule.h"
#include "Gui.h"
#include "Builder.h"
#include "Project.h"
#include "Components.h"

////// New Module Dialog ////////////

extern "C" G_MODULE_EXPORT void on_NewModule_ModuleEntry_changed(
        GtkWidget * widget, gpointer /*data*/)
    {
    OovString module = Gui::getText(GTK_ENTRY(widget));
    OovString interfaceName = module + ".h";
    OovString implementationName = module + ".cpp";
    Gui::setText(GTK_ENTRY(Builder::getBuilder()->getWidget(
            "NewModule_InterfaceEntry")), interfaceName);
    Gui::setText(GTK_ENTRY(Builder::getBuilder()->getWidget(
            "NewModule_ImplementationEntry")), implementationName);
    }

// In glade, set the callback for the dialog's GtkWidget "delete-event" to
// gtk_widget_hide_on_delete for the title bar close button to work.
/*
extern "C" G_MODULE_EXPORT void on_NewModuleCancelButton_clicked(
        GtkWidget * widget, gpointer data)
    {
    gtk_widget_hide(Builder::getBuilder()->getWidget("NewModuleDialog"));
    }
*/

static OovString makeHeaderComment(OovStringRef fileName)
    {
    OovString str = "// File: ";
    str += fileName;
    char buf[50];
    time_t curTime = time(NULL);
    tm *locTime = localtime(&curTime);
// Not available on windows on 10-8-2015
//    strftime(buf, sizeof(buf), "\n// Created: %F\n", locTime);
    strftime(buf, sizeof(buf), "\n// Created: %Y-%m-%d\n", locTime);
    str += buf;
    return str;
    }

bool NewModule::createModuleFiles()
    {
    gtk_widget_hide(Builder::getBuilder()->getWidget("NewModuleDialog"));
    OovString basePath = Project::getSrcRootDirectory();
    OovString interfaceName = Gui::getText(GTK_ENTRY(Builder::getBuilder()->getWidget(
            "NewModule_InterfaceEntry")));
    OovString implementationName = Gui::getText(GTK_ENTRY(Builder::getBuilder()->getWidget(
            "NewModule_ImplementationEntry")));
    OovString compName = Gui::getText(GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
            "NewModule_ComponentComboboxtext")));

    FilePath compDir(basePath, FP_Dir);
    if(compName != Project::getRootComponentName())
        compDir.appendDir(compName);
    // Create the new component directory if it doesn't exist.
    OovStatus status = FileEnsurePathExists(compDir);
    if(!status.ok())
        {
        status.reported();
        OovString err;
        err = "Unable to create component directory ";
        err += compDir;
        Gui::messageBox(err, GTK_MESSAGE_INFO);
        }

    if(status.ok() && interfaceName.length() > 0)
        {
        if(!FileIsFileOnDisk(interfaceName, status))
            {
            FilePath tempInt(compDir, FP_Dir);
            tempInt.appendFile(interfaceName);
            File intFile;
            status = intFile.open(tempInt, "w");
            if(status.ok())
                {
                OovString str = makeHeaderComment(interfaceName);
                status = intFile.putString(str);
                }
            }
        if(!status.ok())
            {
            status.reported();
            OovString err;
            err = "Unable to create interface ";
            err += interfaceName;
            Gui::messageBox(err, GTK_MESSAGE_INFO);
            }
        }

    if(status.ok() && implementationName.length() > 0)
        {
        if(!FileIsFileOnDisk(implementationName, status))
            {
            FilePath tempImp(compDir, FP_Dir);
            tempImp.appendFile(implementationName);
            File impFile;
            status = impFile.open(tempImp, "w");
            if(status.ok())
                {
                mFileName = tempImp;
                OovString str = makeHeaderComment(implementationName);
                status = impFile.putString(str);
                impFile.close();
                }
            }
        if(!status.ok())
            {
            status.reported();
            Gui::messageBox("Unable to create implementation", GTK_MESSAGE_INFO);
            }
        }
    return status.ok();
    }

bool NewModule::runDialog()
    {
    Dialog dlg(GTK_DIALOG(Builder::getBuilder()->getWidget("NewModuleDialog")));
    ComponentTypesFile componentsFile(mProject);
    Gui::clear(GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
            "NewModule_ComponentComboboxtext")));
    ScannedComponentInfo scannedCompInfo;
    OovStatus status = scannedCompInfo.readScannedInfo();
    if(status.ok())
        {
        OovStringVec names = scannedCompInfo.getComponentNames();
        if(names.size() == 0)
            {
            Gui::appendText(GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
                "NewModule_ComponentComboboxtext")), Project::getRootComponentName());
            }
        for(auto const &name : names)
            {
            Gui::appendText(GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
                "NewModule_ComponentComboboxtext")), name);
            }
        Gui::setSelected(GTK_COMBO_BOX(Builder::getBuilder()->getWidget(
            "NewModule_ComponentComboboxtext")), 0);
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read component file for new module");
        }
    bool success = false;
    if(dlg.run(true))
        {
        success = createModuleFiles();
        }
    return success;
    }
