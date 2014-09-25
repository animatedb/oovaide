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

static size_t findEarlierBreakPos(size_t pos, std::string const &str,
	char const *breakStr, size_t startPos)
    {
    size_t spacePos = str.find(breakStr, startPos);
    if(spacePos != std::string::npos)
	{
	spacePos += std::string(breakStr).length();
	if(pos == std::string::npos || spacePos < pos)
	    {
	    pos = spacePos;
	    }
	}
    return pos;
    }

void splitStrings(std::vector<std::string> &strs, size_t desiredLength)
    {
    for(size_t i=0; i<strs.size(); i++)
	{
	if(strs[i].length() > desiredLength)
	    {
	    size_t pos = strs[i].find(',', desiredLength);
	    pos = findEarlierBreakPos(pos, strs[i], "::", desiredLength);
	    pos = findEarlierBreakPos(pos, strs[i], " ", desiredLength);
	    pos = findEarlierBreakPos(pos, strs[i], "<", desiredLength);
	    pos = findEarlierBreakPos(pos, strs[i], ">", desiredLength);
	    if(pos == std::string::npos)
		pos = desiredLength;
	    std::string temp = "   " + strs[i].substr(pos);
	    strs[i].resize(pos);
	    strs.insert(strs.begin()+i+1, temp);
	    }
	}
    }

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
	Gui::messageBox("Use Analysis/Settings to set up an editor to view source");
	}
    }

