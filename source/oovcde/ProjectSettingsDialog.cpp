/*
 * ProjectSettingsDialog.cpp
 *
 *  Created on: March 4, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ProjectSettingsDialog.h"
#include "Builder.h"
#include "Options.h"


GtkWindow *sParentWindow;


ProjectSettingsDialog::ProjectSettingsDialog(GtkWindow *parentWindow,
	bool newProject):
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("NewProjectDialog")),
	    parentWindow),
    mNewProject(newProject)
    {
    sParentWindow = parentWindow;
    }

bool ProjectSettingsDialog::runDialog()
    {
    GtkEntry *projDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
	    "OovcdeProjectDirEntry"));
    GtkTextView *excDirsTextView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
	    "ExcludeDirsTextview"));
    GtkEntry *srcDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
    	    "RootSourceDirEntry"));

    Gui::setText(projDirEntry, Project::getProjectDirectory());
    Gui::setText(srcDirEntry, Project::getSourceRootDirectory());
    CompoundValue excDirs(gBuildOptions.getValue(OptProjectExcludeDirs), ';');
    Gui::setText(excDirsTextView, excDirs.getAsString('\n'));

    Gui::setEnabled(srcDirEntry, mNewProject);
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget(
	    "RootSourceDirButton")), mNewProject);
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget(
	    "OovcdeProjectDirButton")), mNewProject);
    Gui::setEnabled(projDirEntry, mNewProject);

    bool ok = run(true);
    if(ok)
	{
	mProjectDir = gtk_entry_get_text(projDirEntry);
	mExcludeDirs.parseString(Gui::getText(excDirsTextView), '\n');
	mExcludeDirs.deleteEmptyStrings();
	}
    return ok;
    }


extern "C" G_MODULE_EXPORT void on_RootSourceDirButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    OovString srcRootDir;
    if(ch.ChoosePath(sParentWindow, "Open Root Source Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, srcRootDir))
	{
	GtkEntry *dirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
		"RootSourceDirEntry"));
	gtk_entry_set_text(dirEntry, srcRootDir.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_OovcdeProjectDirButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    OovString projectDir;
    if(ch.ChoosePath(sParentWindow, "Create OOVCDE Project Directory",
	    GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
	projectDir))
	{
	GtkEntry *dirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
		"OovcdeProjectDirEntry"));
	gtk_entry_set_text(dirEntry, projectDir.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_RootSourceDirEntry_changed(
	GtkWidget *button, gpointer data)
    {
    gBuildOptions.setDefaultOptions();
    gGuiOptions.setDefaultOptions();
    GtkEntry *dirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget("RootSourceDirEntry"));
    FilePath rootSrcText(gtk_entry_get_text(dirEntry), FP_Dir);
    gBuildOptions.setNameValue(OptSourceRootDir, rootSrcText);

    GtkEntry *projDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
	    "OovcdeProjectDirEntry"));

    FilePathRemovePathSep(rootSrcText, rootSrcText.length()-1);
    Project::setSourceRootDirectory(rootSrcText);
    rootSrcText.appendFile("-oovcde");
    gtk_entry_set_text(projDirEntry, rootSrcText.c_str());
    }

extern "C" G_MODULE_EXPORT void on_ExcludeDirsButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    OovString dir;
    if(ch.ChoosePath(sParentWindow, "Add Exclude Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, dir))
	{
	GtkTextView *dirTextView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
		"ExcludeDirsTextview"));
	std::string relDir;
	relDir = Project::getSrcRootDirRelativeSrcFileDir(dir);
	relDir += '\n';
	Gui::appendText(dirTextView, relDir);
	}
    }

