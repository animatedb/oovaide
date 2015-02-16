/*
 * BuildSettingsDialog.h
 *
 *  Created on: Sep 8, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef BUILDSETTINGSDIALOG_H_
#define BUILDSETTINGSDIALOG_H_

#include "Builder.h"
#include "ClassGraph.h"
#include "Project.h"
#include "Components.h"
#include "Gui.h"


// The build settings dialog allows the user to define the type of components
// and define some build arguments for each component.
class BuildSettingsDialog
    {
    public:
	BuildSettingsDialog();
	virtual ~BuildSettingsDialog()
	    {}
	// Reads from the component file and displays in the tree view
	void enterScreen();
	// Writes the screen selections to the component file.
	void saveScreen();
	// Meant to be called when a different component is selected in the
	// tree view.
	void switchComponent();

    private:
	GuiTree mComponentTree;
	ComponentTypesFile mComponentFile;
	std::string mLastCompName;
	GuiTreeItem getParent(std::string const &compName);
	void saveFromScreen(std::string const &compName);
	void loadToScreen(std::string const &compName);
    };


#endif /* BUILDSETTINGSDIALOG_H_ */
