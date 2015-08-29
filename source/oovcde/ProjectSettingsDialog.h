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
        OovString const getProjectDir() const;
        CompoundValue const &getExcludeDirs() const
            { return mExcludeDirs; }

        // Called by extern functions
        void rootSourceDirButtonClicked();
        void oovcdeProjectDirButtonClicked();
        void excludeDirsButtonClicked();
        void rootSourceDirEntryChanged();

    private:
        ProjectReader &mProjectOptions;
        GuiOptions &mGuiOptions;
        GtkWindow *mParentWindow;
        CompoundValue mExcludeDirs;
        bool mNewProject;

        OovString getRootSrcDir() const;
        ProjectReader &getProjectOptions()
            { return mProjectOptions; }
        GuiOptions &getGuiOptions()
            { return mGuiOptions; }
        GtkWindow *getParentWindow()
            { return mParentWindow; }
    };


#endif /* PROJECTSETTINGSDIALOG_H_ */
