/*
 * EditOptions.cpp
 *
 *  Created on: Feb 21, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "EditOptions.h"
#include "FilePath.h"
#include "OovString.h"

void EditOptions::setProjectDir(std::string projDir)
    {
    if(projDir.length() > 0)
	{
	FilePath projName(projDir, FP_Dir);
	projName.appendFile("oovEditConfig.txt");
	setFilename(projName.c_str());
	}
    }

void EditOptions::setScreenCoord(char const * const tag, int val)
    {
    OovString str;
    str.appendInt(val);
    setNameValue(tag, str.c_str());
    }

void EditOptions::saveScreenSize(int width, int height)
    {
    if(getFilename().length() > 0)
	{
	setScreenCoord("ScreenWidth", width);
	setScreenCoord("ScreenHeight", height);
	writeFile();
	}
    }

bool EditOptions::getScreenCoord(char const * const tag, int &val)
    {
    OovString str = getValue(tag);
    bool got = false;
    if(str.length() > 0)
	{
	if(str.getInt(0, 100000, val))
	    {
	    got = true;
	    }
	}
    return got;
    }

bool EditOptions::getScreenSize(int &width, int &height)
    {
    bool gotPos = false;
    if(readFile())
	{
	int tempWidth;
	if(getScreenCoord("ScreenWidth", tempWidth))
	    {
            int tempHeight;
	    if(getScreenCoord("ScreenHeight", tempHeight))
		{
		width = tempWidth;
		height = tempHeight;
		gotPos = true;
		}
	    }
	}
    return gotPos;
    }

