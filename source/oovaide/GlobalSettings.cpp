/*
 * GlobalSettings.cpp
 *
 *  Created on: Oct 9, 2015
 */

#include "GlobalSettings.h"
#include "NameValueFile.h"
#include "FilePath.h"
#include "Project.h"
#include "Gui.h"
#include <algorithm>

class SettingsFile:public NameValueFile
    {
    public:
        void save();
        bool open();
        void addProject(OovStringRef projDir);

    private:
        File mFile;

        /// Returned file name is empty if there is no place to store settings.
        static OovString getSettingsFileName();
    };

bool SettingsFile::open()
    {
    bool success = false;
    OovString fn = getSettingsFileName();
    if(fn.length() > 0)
        {
        setFilename(fn);
        OovStatus status = readFile();
        success = status.ok();
        if(status.needReport())
            {
            // Discard errors since this feature is unimportant.
            status.reported();
            }
        }
    return success;
    }

void SettingsFile::save()
    {
    OovString fn = getSettingsFileName();
    if(fn.length() > 0)
        {
        setFilename(fn);
        OovStatus status = writeFile();
        if(status.needReport())
            {
            status.reported();
            }
        }
    }

void SettingsFile::addProject(OovStringRef projDir)
    {
    CompoundValue dirs;
    dirs.parseString(getValue("RecentProjects"));
    dirs.erase(std::remove(dirs.begin(), dirs.end(), OovString(projDir)), dirs.end());
    dirs.insert(dirs.begin(), projDir);
    dirs.resize(8);
    setNameValue("RecentProjects", dirs.getAsString());
    }


void GlobalSettings::saveOpenProject(OovStringRef projDir)
    {
    SettingsFile settings;
    settings.open();    // If the file is present, read it.
    settings.addProject(projDir);
    settings.save();
    }

OovString SettingsFile::getSettingsFileName()
    {
#ifdef __linux__
    FilePath dirStr(getenv("HOME"), FP_Dir);
    dirStr.appendDir(".config/Oovaide");
#else
    OovString dirStr = Project::getBinDirectory();
#endif
    OovString fileName;
    FilePath dir(dirStr, FP_Dir);
    OovStatus status = FileEnsurePathExists(dir);
    if(status.ok())
        {
        FilePath filePath(dir, FP_Dir);
        filePath.appendFile("OovaideSettings.txt");
        fileName = filePath;
        }
    return fileName;
    }


static GlobalSettingsListener *sListener;
GlobalSettingsListener::GlobalSettingsListener()
    {
    sListener = this;
    }

extern "C" G_MODULE_EXPORT gboolean onMenuClicked(GtkMenuItem *menuitem,
    gpointer user_data)
    {
    if(sListener)
        {
        sListener->loadProject(gtk_menu_item_get_label(menuitem));
        }
    return true;
    }


void GlobalSettings::updateRecentFilesMenu()
    {
    SettingsFile settings;
    if(settings.open())
        {
        GtkMenuShell *fileMenu = GTK_MENU_SHELL(Builder::getBuilder()->getWidget("Filemenu"));
        CompoundValue dirs;
        dirs.parseString(settings.getValue("RecentProjects"));
        if(dirs.size() > 0)
            {
            GtkSeparatorMenuItem *menu = GTK_SEPARATOR_MENU_ITEM(gtk_separator_menu_item_new());
            gtk_menu_shell_append(fileMenu, GTK_WIDGET(menu));
            }
        for(auto const &dir : dirs)
            {
            if(dir.length() > 0)
                {
                GtkWidget *menu = gtk_menu_item_new_with_label(dir.getStr());
                gtk_menu_shell_append(fileMenu, menu);
                g_signal_connect(menu, "activate", G_CALLBACK(onMenuClicked), nullptr);
                }
            }
        gtk_widget_show_all(GTK_WIDGET(fileMenu));
        }
    }
