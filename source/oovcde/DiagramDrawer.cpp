/*
 * DiagramDrawer.cpp
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramDrawer.h"
#include "Options.h"
#include "OovProcess.h"
#include "Gui.h"
#include "Project.h"
#include <stdio.h>

int DiagramDrawer::getPad(int div)
    {
    int pad = getTextExtentHeight("W");
    pad /= div;
    if(pad < 1)
	pad = 1;
    return pad;
    }

void viewSource(char const * const module, int lineNum)
    {
    std::string proc = gGuiOptions.getValue(OptGuiEditorPath);
    if(proc.length() > 0)
	{
	OovProcessChildArgs args;
	args.addArg(proc.c_str());
	OovString lineArg = gGuiOptions.getValue(OptGuiEditorLineArg);
	if(lineArg.length() != 0)
	    {
	    lineArg.appendInt(lineNum);
	    args.addArg(lineArg.c_str());
	    }
	args.addArg(module);

	std::string projArg;
	if(proc.find("oovEdit") != std::string::npos)
	    {
	    projArg += "-p";
	    projArg += Project::getProjectDirectory();
	    args.addArg(projArg.c_str());
	    }
	spawnNoWait(proc.c_str(), args.getArgv());
	}
    else
	{
	Gui::messageBox("Use Edit/Preferences to set up an editor to view source");
	}
    }

