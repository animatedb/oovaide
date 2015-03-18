/*
 * OovProcessArgs.cpp
 *
 *  Created on: Feb 21, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovProcessArgs.h"

void OovProcessChildArgs::addArg(OovStringRef const argStr)
    {
    mArgStrings.push_back(argStr);
    }

char const * const *OovProcessChildArgs::getArgv() const
    {
    mArgv.clear();
    for(size_t i=0; i<mArgStrings.size(); i++)
	{
	mArgv.push_back(mArgStrings[i].getStr());
	}
    mArgv.push_back(nullptr);	// Add one for end null
    return const_cast<char const * const *>(&mArgv[0]);
    }

OovString OovProcessChildArgs::getArgsAsStr() const
    {
    OovString argStr;
    for(auto const &arg : mArgStrings)
	{
	argStr += arg;
	argStr += ' ';
	}
    argStr += '\n';
    return argStr;
    }

void OovProcessChildArgs::printArgs(FILE *fh) const
    {
    fprintf(fh, "%s", getArgsAsStr().getStr());
    fflush(fh);
    }

