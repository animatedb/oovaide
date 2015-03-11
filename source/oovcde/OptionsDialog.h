/*
 * OptionsDialog.h
 *
 *  Created on: Jul 11, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPTIONSDIALOG_H_
#define OPTIONSDIALOG_H_

#include "Builder.h"
#include "ClassGraph.h"
#include "Gui.h"
#include "PackagesDialogs.h"

/// Provides an options/preferences/settings dialog. This supports a tabbed
/// list of dialogs to display as options.
class OptionsDialog
    {
    public:
	OptionsDialog();
	virtual ~OptionsDialog()
	    {}
	void updateBuildConfig();
	void showScreen();
	void saveScreen();
	void setBuildConfig();
	void newConfig();
	void addConfig();
	void runPackagesDialog();
	// This class calls this function after the options file is written.
	virtual void updateOptions() = 0;
	virtual void buildConfig(OovStringRef const name) = 0;

    private:
	GuiList mBuildConfigList;
	std::string mCurrentBuildConfig;
	int mDialogRunning;
	std::vector<GtkWidget *> mMenuItems;
	void updateBuildMenu(OovStringVec &names);
    };

class ClassPreferencesDialog
    {
    public:
	bool run(Builder &builder, ClassNodeDrawOptions &options);
    };

#endif /* OPTIONSDIALOG_H_ */
