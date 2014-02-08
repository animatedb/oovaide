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
    mComponentList.init(*Builder::getBuilder(), "ComponentListTreeview", "Component List");
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

	mComponentList.clear();
	for(auto const &name : mComponentFile.getComponentNames())
	    {
	    if(mLastCompName.length() == 0)
		{
		mLastCompName = name;
		}
	    mComponentList.appendText(name.c_str());
	    }
	}
    mComponentList.setSelected(mLastCompName.c_str());
    }

void BuildSettingsDialog::switchComponent()
    {
    saveFromScreen(mLastCompName.c_str());
    mLastCompName = mComponentList.getSelected();
    loadToScreen(mLastCompName);
    }

void BuildSettingsDialog::saveScreen()
    {
    saveFromScreen(mLastCompName.c_str());
    mComponentFile.writeFile();
    }

void BuildSettingsDialog::saveFromScreen(std::string const &compName)
    {
    GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
	    "ComponentTypeComboboxtext"));
    char const * const boxValue = gtk_combo_box_text_get_active_text(typeBox);
    // If the cursor is changed during construction, boxValue will be NULL.
    if(compName.length() && boxValue)
	{
	std::string tag = ComponentTypesFile::getCompTagName(compName, "type");
	char const * value = ComponentTypesFile::getComponentTypeAsFileValue(
		ComponentTypesFile::CompTypes::CT_Unknown);
	if(boxValue)
	    {
	    value = ComponentTypesFile::getComponentTypeAsFileValue(
		    ComponentTypesFile::getComponentTypeFromTypeName(boxValue));
	    }
	mComponentFile.setNameValue(tag.c_str(), value);
	}
    }

void BuildSettingsDialog::loadToScreen(std::string const &compName)
    {
    if(compName.length())
	{
	GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(Builder::getBuilder()->getWidget(
		"ComponentTypeComboboxtext"));
	std::string tag = ComponentTypesFile::getCompTagName(compName, "type");
	std::string val = mComponentFile.getValue(tag.c_str());
	int index = ComponentTypesFile::getComponentTypeFromTypeName(val.c_str());
	Gui::setSelected(GTK_COMBO_BOX(typeBox), index);
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

    Dialog dlg(GTK_DIALOG(Builder::getBuilder()->getWidget("BuildSettingsDialog")));
    dlg.run();
    }
