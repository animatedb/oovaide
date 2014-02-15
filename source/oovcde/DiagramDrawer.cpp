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
	char const * argv[4];
	int argi=0;
	argv[argi++] = proc.c_str();
	std::string lineArg = gGuiOptions.getValue(OptGuiEditorLineArg);
	if(lineArg.length() != 0)
	    {
	    char buf[10];
	    snprintf(buf, sizeof(buf), "%d", lineNum);
	    lineArg += buf;
	    argv[argi++] = lineArg.c_str();
	    }
	argv[argi++] = module;
	argv[argi++] = NULL;
	spawnNoWait(proc.c_str(), argv);
	}
    else
	Gui::messageBox("Use Edit/Preferences to set up an editor to view source");
    }

