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


/// This does not read or write the project file. It only reads and writes
/// values in Project.
class ProjectSettingsDialog: public Dialog
    {
    public:
        enum EditStyles
            {
            PS_NewProject,      // Clears the entry fields, and allows editing
            PS_OpenProject,     // Fills entry fields and does not allow editing
            PS_OpenProjectEditSource    // Fills entry fields and allows editing source and excludes
            };
        ProjectSettingsDialog(GtkWindow *parentWindow,
                ProjectReader &projectOptions, GuiOptions &guiOptions,
                EditStyles editStyle);
        virtual ~ProjectSettingsDialog();
        bool runDialog();
        OovString const getProjectDir() const;
        CompoundValue const &getExcludeDirs() const
            { return mExcludeDirs; }

        // Called by extern functions
        void rootSourceDirButtonClicked();
        void oovaideProjectDirButtonClicked();
        void excludeDirsButtonClicked();
        void rootSourceDirEntryChanged();

    private:
        ProjectReader &mProjectOptions;
        GuiOptions &mGuiOptions;
        GtkWindow *mParentWindow;
        CompoundValue mExcludeDirs;
        EditStyles mEditStyle;

        OovString getRootSrcDir() const;
        ProjectReader &getProjectOptions()
            { return mProjectOptions; }
        GuiOptions &getGuiOptions()
            { return mGuiOptions; }
        GtkWindow *getParentWindow()
            { return mParentWindow; }
    };


#endif /* PROJECTSETTINGSDIALOG_H_ */
