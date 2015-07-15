/*
 * ProjectSettingsDialog.h
 *
 *  Created on: March 4, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PROJECTSETTINGSDIALOG_H_
#define PROJECTSETTINGSDIALOG_H_

#include "Gui.h"
#include "Project.h"
#include "Options.h"


class ProjectSettingsDialog: public Dialog
    {
    public:
        ProjectSettingsDialog(GtkWindow *parentWindow,
                ProjectReader &projectOptions, GuiOptions &guiOptions,
                bool newProject);
        virtual ~ProjectSettingsDialog();
        bool runDialog();
        CompoundValue const &getExcludeDirs() const
            { return mExcludeDirs; }
        OovString const &getProjectDir() const
            { return mProjectDir; }
        ProjectReader &getProjectOptions()
            { return mProjectOptions; }
        GuiOptions &getGuiOptions()
            { return mGuiOptions; }
        GtkWindow *getParentWindow()
            { return mParentWindow; }

    private:
        ProjectReader &mProjectOptions;
        GuiOptions &mGuiOptions;
        GtkWindow *mParentWindow;
        CompoundValue mExcludeDirs;
        OovString mProjectDir;
        bool mNewProject;
    };


#endif /* PROJECTSETTINGSDIALOG_H_ */
