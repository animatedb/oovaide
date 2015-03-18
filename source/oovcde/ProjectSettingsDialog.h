/*
 * ProjectSettingsDialog.h
 *
 *  Created on: March 4, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PROJECTSETTINGSDIALOG_H_
#define PROJECTSETTINGSDIALOG_H_

#include "Gui.h"
#include "NameValueFile.h"


class ProjectSettingsDialog: public Dialog
    {
    public:
	ProjectSettingsDialog(GtkWindow *parentWindow, bool newProject);
	bool runDialog();
	CompoundValue const &getExcludeDirs() const
	    { return mExcludeDirs; }
	OovString const &getProjectDir() const
	    { return mProjectDir; }

    private:
	CompoundValue mExcludeDirs;
	OovString mProjectDir;
	bool mNewProject;
    };


#endif /* PROJECTSETTINGSDIALOG_H_ */
