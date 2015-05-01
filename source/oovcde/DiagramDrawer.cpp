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
#include <math.h>	// for atan

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

void splitStrings(OovStringVec &strs, size_t desiredLength)
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

DiagramArrow::DiagramArrow(GraphPoint producer, GraphPoint consumer, int arrowSize)
    {
    int xdist = producer.x-consumer.x;
    int ydist = producer.y-consumer.y;
    double lineAngleRadians;

    arrowSize = -arrowSize;
    if(ydist != 0)
	lineAngleRadians = atan2(xdist, ydist);
    else
	{
	if(producer.x > consumer.x)
	    lineAngleRadians = M_PI/2;
	else
	    lineAngleRadians = -M_PI/2;
	}
    const double triangleAngle = (2 * M_PI) / arrowSize;
    // calc left point of symbol
    mLeftPos.set(sin(lineAngleRadians-triangleAngle) * arrowSize,
		cos(lineAngleRadians-triangleAngle) * arrowSize);
    // calc right point of symbol
    mRightPos.set(sin(lineAngleRadians+triangleAngle) * arrowSize,
	    cos(lineAngleRadians+triangleAngle) * arrowSize);
    }

int DiagramDrawer::getPad(int div)
    {
    int pad = getTextExtentHeight("W");
    pad /= div;
    if(pad < 1)
	pad = 1;
    return pad;
    }

// Just use Keneth Kelly's colors
// http://stackoverflow.com/questions/470690/how-to-automatically-generate-n-distinct-colors
// @param index 0 : MaxColors
size_t DistinctColors::getNumColors()
    {
    return 19;
    }

Color DistinctColors::getColor(size_t index)
    {
    static int colors[] =
	{
	0xFFB300, // Vivid Yellow
	0x803E75, // Strong Purple
	0xFF6800, // Vivid Orange
	0xA6BDD7, // Very Light Blue
// RESERVED	0xC10020, // Vivid Red
	0xCEA262, // Grayish Yellow
	0x817066, // Medium Gray

	// Not good for color blind people
	0x007D34, // Vivid Green
	0xF6768E, // Strong Purplish Pink
	0x00538A, // Strong Blue
	0xFF7A5C, // Strong Yellowish Pink
	0x53377A, // Strong Violet
	0xFF8E00, // Vivid Orange Yellow
	0xB32851, // Strong Purplish Red
	0xF4C800, // Vivid Greenish Yellow
	0x7F180D, // Strong Reddish Brown
	0x93AA00, // Vivid Yellowish Green
	0x593315, // Deep Yellowish Brown
	0xF13A13, // Vivid Reddish Orange
	0x232C16, // Dark Olive Green
	};
    return(Color(colors[index]>>16, (colors[index]>>8) & 0xFF, (colors[index]) & 0xFF));
    }

void viewSource(OovStringRef const module, int lineNum)
    {
    std::string proc = gGuiOptions.getValue(OptGuiEditorPath);
    if(proc.length() > 0)
	{
	OovProcessChildArgs args;
	args.addArg(proc);
	OovString lineArg = gGuiOptions.getValue(OptGuiEditorLineArg);
	if(lineArg.length() != 0)
	    {
	    lineArg.appendInt(lineNum);
	    args.addArg(lineArg);
	    }
	FilePath file(module, FP_File);
	FilePathQuoteCommandLinePath(file);
	args.addArg(file);

	std::string projArg;
	if(proc.find("oovEdit") != std::string::npos)
	    {
	    projArg += "-p";
	    projArg += Project::getProjectDirectory();
	    args.addArg(projArg);
	    }
	spawnNoWait(proc, args.getArgv());
	}
    else
	{
	Gui::messageBox("Use Analysis/Settings to set up an editor to view source");
	}
    }

