/*
 * IncDirMap.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "IncDirMap.h"
#include "IncludeMap.h"
#include "OovError.h"
#include <algorithm>            // For find
#include <time.h>
#include <limits.h>     // For UINT_MAX


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
    if(!readFile())
        {
        }
#endif
    }

/// For every includer file that is run across during parsing, this means that
/// the includer file was fully parsed, and that no old included information
/// needs to be kept, except to check if the old included information has not
/// changed.
///
///  This code assumes that no tricks are played with ifdef values, and
/// ifdef values must be the same every time a the same file is included.
void IncDirDependencyMap::write()
    {
    bool anyChanges = false;
    time_t curTime;
    time_t changedTime = 0;
    time(&curTime);

#if(SHARED_FILE)
    SharedFile file;
    if(!writeFileExclusiveReadUpdate(file))
        {
        fprintf(stderr, "\nOovCppParser - Write file sharing error %s\n", getFilename().c_str());
        }
#endif
    // Append new values from parsed includes into the original NameValueFile.
    // Update existing values times and make new dependencies.
    for(const auto &newMapItem : mParsedIncludeDependencies)
        {
        bool changed = includedPathsChanged(newMapItem.first, newMapItem.second);
        if(changed)
            {
            // Cheat and say updated time and checked time are the same.
            changedTime = curTime;

            CompoundValue newIncludedInfoCompVal;
            OovString changeStr;
            changeStr.appendInt(changedTime);
            newIncludedInfoCompVal.addArg(changeStr);

            OovString checkedStr;
            checkedStr.appendInt(curTime);
            newIncludedInfoCompVal.addArg(checkedStr);

            for(const auto &str : newMapItem.second)
                {
                size_t pos = newIncludedInfoCompVal.find(str);
                if(pos == CompoundValue::npos)
                    {
                    newIncludedInfoCompVal.addArg(str);
                    }
                }
            setNameValue(newMapItem.first, newIncludedInfoCompVal.getAsString());
            anyChanges = true;
            }
        }
    if(file.isOpen() && anyChanges)
        {
#if(SHARED_FILE)
        if(!writeFileExclusive(file))
            {
            OovString str = "Unable to write include map file";
            OovError::report(ET_Error, str);
            }
#else
        writeFile();
#endif
        }
    }

bool IncDirDependencyMap::includedPathsChanged(OovStringRef includerFn,
        std::set<std::string> const &includedInfoStr) const
    {
    bool changed = false;

    // First check if the includer filename exists in the dependency file.
    OovString origIncludedInfoStr = getValue(includerFn);
    if(origIncludedInfoStr.size() > 0)
        {
        CompoundValue origIncludedInfoCompVal;
        // get the changed time, which is the first value and discard
        // everything else.
        origIncludedInfoCompVal.parseString(origIncludedInfoStr);
        // Check that counts of the number of includes is the same in
        // the new and original map.  This will detect deleted includes.
        if(origIncludedInfoCompVal.size()-IncDirMapNumTimeVals !=
                (includedInfoStr.size() * IncDirMapNumIncPathParts))
            {
            changed = true;
            }
        else
            {
            // Every included file in the new map must exist in the
            // original map.
            for(const auto &included : includedInfoStr)
                {
                CompoundValue incPathParts;
                incPathParts.parseString(included);
                if(incPathParts.size() == 2)
                    {
                    if(std::find(origIncludedInfoCompVal.begin(),
                            origIncludedInfoCompVal.end(), incPathParts[1]) ==
                            origIncludedInfoCompVal.end())
                        {
                        changed = true;
                        }
                    }
                else
                    {
                    changed = true;
                    }
                }
            }
        }
    else
        {
        changed = true;
        }
    return changed;
    }

void IncDirDependencyMap::insert(const std::string &includerFn,
        const FilePath &includedFn)
    {
#ifdef __linux__

#else
// This is an optimization, but is not standard for all compilers.
//    if(includedFn.getPosPathSegment("mingw") == std::string::npos)
#endif
        {
        mParsedIncludeDependencies[includerFn].insert(includedFn);
        }
    }

