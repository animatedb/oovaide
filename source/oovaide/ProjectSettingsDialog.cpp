/*
 * ProjectSettingsDialog.cpp
 *
 *  Created on: March 4, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ProjectSettingsDialog.h"
#include "Builder.h"


ProjectSettingsDialog *sProjectSettingsDialog;

ProjectSettingsDialog::ProjectSettingsDialog(GtkWindow *parentWindow,
        ProjectReader &projectOptions, GuiOptions &guiOptions,
        EditStyles editStyle):
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("NewProjectDialog")),
            parentWindow),
    mProjectOptions(projectOptions), mGuiOptions(guiOptions),
    mParentWindow(parentWindow), mEditStyle(editStyle)
    {
    sProjectSettingsDialog = this;
    }

ProjectSettingsDialog::~ProjectSettingsDialog()
    {
    }

OovString const ProjectSettingsDialog::getProjectDir() const
    {
    GtkEntry *projDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
            "OovaideProjectDirEntry"));
    return gtk_entry_get_text(projDirEntry);
    }

OovString ProjectSettingsDialog::getRootSrcDir() const
    {
    GtkEntry *srcDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
            "RootSourceDirEntry"));
    return gtk_entry_get_text(srcDirEntry);
    }

bool ProjectSettingsDialog::runDialog()
    {
    GtkEntry *projDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
            "OovaideProjectDirEntry"));
    GtkTextView *excDirsTextView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
            "ExcludeDirsTextview"));
    GtkEntry *srcDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
            "RootSourceDirEntry"));

    OovString origSourceDir = Project::getSourceRootDirectory();
    if(mEditStyle == PS_NewProject)
        {
        Gui::clear(projDirEntry);
        Gui::clear(srcDirEntry);
        Gui::clear(excDirsTextView);
        }
    else
        {
        Gui::setText(projDirEntry, Project::getProjectDirectory());
        Gui::setText(srcDirEntry, Project::getSourceRootDirectory());
        CompoundValue excDirs(mProjectOptions.getValue(OptProjectExcludeDirs), ';');
        Gui::setText(excDirsTextView, excDirs.getAsString('\n'));
        }

    bool editSrc = (mEditStyle == PS_NewProject ||
        mEditStyle == PS_OpenProjectEditSource);
    Gui::setEnabled(srcDirEntry, editSrc);
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget(
            "RootSourceDirButton")), editSrc);
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget(
            "OovaideProjectDirButton")), mEditStyle == PS_NewProject);
    Gui::setEnabled(projDirEntry, mEditStyle == PS_NewProject);

    bool ok = run(true);
    if(ok)
        {
        OovStatus status(true, SC_File);
        if(!FileIsDirOnDisk(getRootSrcDir(), status))
            {
            ok = Gui::messageBox("The source directory does not exist. "
                "Do you want to create it?", GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO);
            if(ok)
                {
                FilePath dir(getRootSrcDir(), FP_Dir);
                status = FileEnsurePathExists(dir);
                }
            }
        if(status.needReport())
            {
            ok = false;
            status.report(ET_Error, "Unable to access directory");
            }
        }
    if(ok)
        {
        mExcludeDirs.parseString(Gui::getText(excDirsTextView), '\n');
        mExcludeDirs.deleteEmptyStrings();
        Project::setSourceRootDirectory(getRootSrcDir());

        // Update the project options.
        if(mEditStyle == PS_NewProject)
            {
            OptionsDefaults optionDefaults(getProjectOptions());
            optionDefaults.setDefaultOptions();
            sProjectSettingsDialog->getGuiOptions().setDefaultOptions();
            }
        FilePath rootSrcText(getRootSrcDir(), FP_Dir);
        getProjectOptions().setNameValue(OptSourceRootDir, rootSrcText);
        }
    return ok;
    }

void ProjectSettingsDialog::rootSourceDirButtonClicked()
    {
    PathChooser ch;
    OovString srcRootDir;
    if(ch.ChoosePath(sProjectSettingsDialog->getParentWindow(),
            "Open Root Source Directory",
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, srcRootDir))
        {
        GtkEntry *dirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
                "RootSourceDirEntry"));
        gtk_entry_set_text(dirEntry, srcRootDir.c_str());
        }
    }

void ProjectSettingsDialog::oovaideProjectDirButtonClicked()
    {
    PathChooser ch;
    OovString projectDir;
    if(ch.ChoosePath(sProjectSettingsDialog->getParentWindow(),
            "Create OOVAIDE Project Directory",
            GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
        projectDir))
        {
        GtkEntry *dirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
                "OovaideProjectDirEntry"));
        gtk_entry_set_text(dirEntry, projectDir.c_str());
        }
    }

void ProjectSettingsDialog::excludeDirsButtonClicked()
    {
    PathChooser ch;
    OovString dir;
    if(ch.ChoosePath(sProjectSettingsDialog->getParentWindow(),
            "Add Exclude Directory",
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, dir))
        {
        GtkTextView *dirTextView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
                "ExcludeDirsTextview"));
        std::string relDir;
        relDir = Project::getSrcRootDirRelativeSrcFileDir(getRootSrcDir(), dir);
        relDir += '\n';
        Gui::appendText(dirTextView, relDir);
        }
    }

void ProjectSettingsDialog::rootSourceDirEntryChanged()
    {
    if(mEditStyle != PS_OpenProjectEditSource)
        {
        GtkEntry *projDirEntry = GTK_ENTRY(Builder::getBuilder()->getWidget(
                "OovaideProjectDirEntry"));

        FilePath rootSrcText(getRootSrcDir(), FP_Dir);
        FilePathRemovePathSep(rootSrcText, rootSrcText.length()-1);
        rootSrcText.appendFile("-oovaide");
        gtk_entry_set_text(projDirEntry, rootSrcText.c_str());
        }
    }

extern "C" G_MODULE_EXPORT void on_RootSourceDirButton_clicked(
        GtkWidget *button, gpointer data)
    {
    sProjectSettingsDialog->rootSourceDirButtonClicked();
    }

extern "C" G_MODULE_EXPORT void on_OovaideProjectDirButton_clicked(
        GtkWidget *button, gpointer data)
    {
    sProjectSettingsDialog->oovaideProjectDirButtonClicked();
    }

extern "C" G_MODULE_EXPORT void on_RootSourceDirEntry_changed(
        GtkWidget *button, gpointer data)
    {
    sProjectSettingsDialog->rootSourceDirEntryChanged();
    }

extern "C" G_MODULE_EXPORT void on_ExcludeDirsButton_clicked(
        GtkWidget *button, gpointer data)
    {
    sProjectSettingsDialog->excludeDirsButtonClicked();
    }

