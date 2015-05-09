/*
 * IncDirMap.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "IncDirMap.h"
#include <algorithm>		// For find
#include <time.h>
#include <limits.h>	// For UINT_MAX


#define SHARED_FILE 1

void IncDirDependencyMap::read(char const * const outDir, char const * const incFn)
    {
    FilePath outIncFileName(outDir, FP_Dir);
    outIncFileName.appendFile(incFn);
    setFilename(outIncFileName);
#if(SHARED_FILE)
    if(!readFileShared())
	{
	fprintf(stderr, "\nOovCppParser - Read file sharing error\n");
	}
#else
    readFile();
#endif
    }

/// For every includer file that is run across during parsing, this means that
/// the includer file was fully parsed, and that no old included information
/// needs to be kept, except to check if the old included information has not
/// changed.  This implies that tricky ifdefs must be the same for all
/// included files.
void IncDirDependencyMap::write()
    {
    time_t curTime;
    time_t changedTime = 0;
    time(&curTime);

#if(SHARED_FILE)
    SharedFile file;
    if(!writeFileExclusiveReadUpdate(file))
	{
	fprintf(stderr, "\nOovCppParser - Write file sharing error\n");
	}
#endif
    // Append new values from parsed includes to the NameValueFile.
    // Update existing values times and make new dependencies.
    for(const auto &mapItem : mParsedIncludeDependencies)
	{
	CompoundValue compVal;
	// First check if the includer filename exists in the dependency file.
	OovString val = getValue(mapItem.first);
	if(val.size() > 0)
	    {
	    // get the changed time, which is the first value and discard
	    // everything else.
	    compVal.parseString(val);
	    bool changed = false;
	    if(compVal.size()-2 != mapItem.second.size())
		{
		changed = true;
		}
	    else
		{
		for(const auto &included : mapItem.second)
		    {
		    if(std::find(compVal.begin(), compVal.end(), included) ==
			    compVal.end())
			{
			changed = true;
			}
		    }
		}
	    if(changed)
		changedTime = curTime;
	    else
		{
		OovString timeStr(compVal[0]);
		unsigned int timeVal;
		timeStr.getUnsignedInt(0, UINT_MAX, timeVal);
		changedTime = static_cast<time_t>(timeVal);
		}
	    compVal.clear();
	    }

	OovString changeStr;
	changeStr.appendInt(changedTime);
	compVal.addArg(changeStr);

	OovString checkedStr;
	checkedStr.appendInt(curTime);
	compVal.addArg(checkedStr);

	for(const auto &str : mapItem.second)
	    {
	    size_t pos = compVal.find(str);
	    if(pos == CompoundValue::npos)
		{
		compVal.addArg(str);
		}
	    }
	setNameValue(mapItem.first, compVal.getAsString());
	}
    if(file.isOpen())
	{
#if(SHARED_FILE)
	writeFileExclusive(file);
#else
	writeFile();
#endif
	}
    }

void IncDirDependencyMap::insert(const std::string &includerFn,
	const FilePath &includedFn)
    {
#ifdef __linux__

#else
    if(includedFn.getPosPathSegment("mingw") == std::string::npos)
#endif
	{
	mParsedIncludeDependencies[includerFn].insert(includedFn);
	}
    }

