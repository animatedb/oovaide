/*
 * BuildSettingsDialog.cpp
 *
 *  Created on: Sep 8, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */


#include "BuildSettingsDialog.h"
#include "Gui.h"
#include "Components.h"

static BuildSettingsDialog *gBuildDlg;


BuildSettingsDialog::BuildSettingsDialog()
    {
    gBuildDlg = this;
    mComponentTree.init(*Builder::getBuilder(), "ComponentListTreeview", "Component List");
    }


//////////////


void BuildSettingsDialog::enterScreen()
    {
    if(mComponentFile.read())
	{
	GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
		"ComponentTypeComboboxtext"));
	Gui::clear(typeBox);
	Gui::setSelected(GTK_COMBO_BOX(typeBox), -1);
	Gui::appendText(typeBox, ComponentTypesFile::getLongComponentTypeName(
		ComponentTypesFile::CT_Unknown));
	Gui::appendText(typeBox, ComponentTypesFile::getLongComponentTypeName(
		ComponentTypesFile::CT_StaticLib));
	Gui::appendText(typeBox, ComponentTypesFile::getLongComponentTypeName(
		ComponentTypesFile::CT_SharedLib));
	Gui::appendText(typeBox, ComponentTypesFile::getLongComponentTypeName(
		ComponentTypesFile::CT_Program));

	mComponentTree.clear();
	for(auto const &name : mComponentFile.getComponentNames())
	    {
	    if(mLastCompName.length() == 0)
		{
		mLastCompName = name;
		}
	    mComponentTree.appendText(getParent(name), getChildName(name).c_str());
	    }
	}
    mComponentTree.setSelected(mLastCompName.c_str(), '/');
    }

std::string BuildSettingsDialog::getChildName(std::string const &compName)
    {
    OovString child = compName;
    size_t pos = child.rfind('/');
    if(pos != OovString::npos)
	{
	child.erase(0, pos+1);
	}
    return child;
    }

GuiTreeItem BuildSettingsDialog::getParent(std::string const &compName)
    {
    OovString parent = compName;
    size_t pos = parent.rfind('/');
    if(pos != OovString::npos)
	{
	parent.erase(pos);
	}
    return mComponentTree.getItem(parent, '/');
    }

void BuildSettingsDialog::switchComponent()
    {
    saveFromScreen(mLastCompName);
    mLastCompName = mComponentTree.getSelected('/');
    loadToScreen(mLastCompName);
    }

void BuildSettingsDialog::saveScreen()
    {
    saveFromScreen(mLastCompName);
    mComponentFile.writeFile();
    }

void BuildSettingsDialog::saveFromScreen(std::string const &compName)
    {
    GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
	    "ComponentTypeComboboxtext"));
    GtkTextView *argsView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
	    "ComponentBuildArgsTextview"));

    char const * const boxValue = gtk_combo_box_text_get_active_text(typeBox);
    // If the cursor is changed during construction, boxValue will be NULL.
    if(compName.length())
	{
	if(boxValue)
	    {
            mComponentFile.setComponentType(compName.c_str(), boxValue);
	    }
	std::string str = Gui::getText(argsView);
	CompoundValue buildArgs;
	buildArgs.parseString(str.c_str(), '\n');
	std::string newBuildArgsStr = buildArgs.getAsString();
	mComponentFile.setComponentBuildArgs(compName.c_str(), newBuildArgsStr.c_str());
	}
    }

void BuildSettingsDialog::loadToScreen(std::string const &compName)
    {
    if(compName.length())
	{
	GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
		"ComponentTypeComboboxtext"));
	GtkTextView *argsView = GTK_TEXT_VIEW(Builder::getBuilder()->getWidget(
		"ComponentBuildArgsTextview"));

        int index = mComponentFile.getComponentType(compName.c_str());
	Gui::setSelected(GTK_COMBO_BOX(typeBox), index);

	CompoundValue cppArgs;
	cppArgs.parseString(mComponentFile.getComponentBuildArgs(compName.c_str()).c_str());
	std::string str = cppArgs.getAsString('\n');
	Gui::setText(argsView, str.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_ComponentListTreeview_cursor_changed(GtkTreeView *view,
    gpointer *data)
    {
    gBuildDlg->switchComponent();
    }

extern "C" G_MODULE_EXPORT void on_BuildSettingsOkButton_clicked(
	GtkWidget *button, gpointer data)
    {
    gBuildDlg->saveScreen();
    gtk_widget_hide(Builder::getBuilder()->getWidget("BuildSettingsDialog"));
    }

extern "C" G_MODULE_EXPORT void on_BuildSettingsCancelButton_clicked(
	GtkWidget *button, gpointer data)
    {
    gtk_widget_hide(Builder::getBuilder()->getWidget("BuildSettingsDialog"));
    }

extern "C" G_MODULE_EXPORT void on_BuildSettingsMenuitem_activate()
    {
    gBuildDlg->enterScreen();

    Dialog dlg(GTK_DIALOG(Builder::getBuilder()->getWidget("BuildSettingsDialog")),
	    GTK_WINDOW(Builder::getBuilder()->getWidget("MainWindow")));
    dlg.run();
    }
