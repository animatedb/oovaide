/*
 * IncludeMap.cpp
 *
 *  Created on: Jan 6, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeMap.h"
#include "Components.h" // For isHeader
#include "Debug.h"
#include "OovError.h"
#include <algorithm>

void IncDirDependencyMapReader::read(OovStringRef const fn)
    {
    setFilename(fn);
    // It is ok if the include map is not present the first time.
    OovStatus status(true, SC_File);
    if(isFilePresent(status))
        {
        status = readFile();
        }
    if(status.needReport())
        {
        OovString str = "Unable to read include map: ";
        str += fn;
        status.report(ET_Error, str);
        }
    }

void discardDirs(OovStringVec &paths)
    {
    for(auto &fn : paths)
        {
        FilePath fp(fn, FP_File);
        fn = fp.getNameExt();
        }
    }

OovStringVec IncDirDependencyMapReader::getIncludeFilesDefinedInDirectory(
        OovStringRef const dirName) const
    {
    OovStringVec possibleHeaders = getFilesDefinedInDirectory(dirName);
    OovStringVec headers;
    std::copy_if(possibleHeaders.begin(), possibleHeaders.end(),
            std::back_inserter(headers),
            [](std::string const &header) { return isCppHeader(header); });
    return headers;
    }

OovStringVec IncDirDependencyMapReader::getJavaFilesDefinedInDirectory(
    OovStringRef const dirName) const
    {
    OovStringVec possibleJavaFiles = getFilesDefinedInDirectory(dirName);
    OovStringVec javaFiles;
    std::copy_if(possibleJavaFiles.begin(), possibleJavaFiles.end(),
            std::back_inserter(javaFiles),
            [](std::string const &header) { return isJavaSource(header); });
    return javaFiles;
    }

OovStringVec IncDirDependencyMapReader::getFilesDefinedInDirectory(
        OovStringRef const dirName) const
    {
    OovStringVec incFiles;
    FilePath matchDir(dirName, FP_Dir);
    for(auto const &incDep : getNameValues())
        {
        FilePath incDir(incDep.first, FP_File);
        incDir.discardFilename();
        if(matchDir.compare(incDir) == 0)
            {
            incFiles.push_back(incDep.first);
            }
        }
    return incFiles;
    }

static void dispMapItemErr(OovStringRef fileMapStr)
    {
    fprintf(stderr, "Bad entry in oovaide-incdeps.txt:\n   %s\n",
            fileMapStr.getStr());
    DebugAssert(__FILE__, __LINE__);

    }

template <typename Func> void processIncPath(OovString const fileMapStr,
        std::set<IncludedPath> &incFiles, Func procFunc)
    {
    CompoundValue compVal;
    if(fileMapStr.length() > 0)
        {
        compVal.parseString(fileMapStr);
        // There are two time values followed by directory/filename pairs.
        if(compVal.size() > IncDirMapNumTimeVals &&
                (compVal.size() % IncDirMapNumTimeVals == 0))
            {
            // Skip first two time values.
            for(size_t i=IncDirMapNumTimeVals; i<compVal.size();
                    i+=IncDirMapNumTimeVals)
                {
                IncludedPath incPath(compVal[i], compVal[i+1]);
                const auto &ret = incFiles.insert(incPath);
                // Check if this is the first insertion of this value.
                // Prevents infinite recursion and duplicates.
                if(ret.second == true)
                    {
                    procFunc(incPath);
                    }
                }
            }
        else
            {
            dispMapItemErr(fileMapStr);
            }
        }
    };

std::set<IncludedPath> IncDirDependencyMapReader::getAllIncludeFiles() const
    {
    std::set<IncludedPath> incFiles;
    auto const &nameVals = getNameValues();
    for(auto const &val : nameVals)
        {
        getImmediateIncludeFilesUsedBySourceFile(val.first,
                incFiles);
        }
    return incFiles;
    }

std::set<OovString> IncDirDependencyMapReader::getAllFiles() const
    {
    std::set<IncludedPath> incFiles = getAllIncludeFiles();
    std::set<OovString> files;
    for(auto const &file : incFiles)
        {
        files.insert(file.getFullPath());
        }
    auto const &nameVals = getNameValues();
    for(auto const &val : nameVals)
        {
        files.insert(val.first);
        }
    return files;
    }

void IncDirDependencyMapReader::getImmediateIncludeFilesUsedBySourceFile(
        OovStringRef const srcName, std::set<IncludedPath> &incFiles) const
    {
    FilePath fp(srcName, FP_File);
    OovString val = getValue(fp);
    processIncPath(val, incFiles, [](IncludedPath &){});
    expandJavaFiles(incFiles);
    }

/// This is recursive.
void IncDirDependencyMapReader::getNestedIncludeFilesUsedBySourceFile(
        OovStringRef const srcName, std::set<IncludedPath> &incFiles) const
    {
    FilePath fp(srcName, FP_File);
    OovString val = getValue(fp);
    processIncPath(val, incFiles,
            [this, &incFiles](IncludedPath const &incPath) mutable
        {
        getNestedIncludeFilesUsedBySourceFile(incPath.getFullPath(), incFiles);
        });
    expandJavaFiles(incFiles);
    }

void IncDirDependencyMapReader::expandJavaFiles(std::set<IncludedPath> &incFiles) const
    {
    for(auto it = incFiles.begin(); it != incFiles.end(); )
        {
        if((*it).getFullPath().find('*') != std::string::npos)
            {
            OovStringVec javaFiles = getJavaExpandedFiles((*it).getFullPath());
            if(javaFiles.size() > 0)
                {
                for(auto const &jav : javaFiles)
                    {
                    FilePath dir(jav, FP_File);
                    dir.discardFilename();
                    FilePath file(jav, FP_File);
                    file.discardDirectory();
                    incFiles.insert(IncludedPath(dir, file));
                    }
                it = incFiles.erase(it++);
                }
            else
                {
                ++it;
                }
            }
        else
            {
            ++it;
            }
        }
    }

/// This is recursive
OovStringVec IncDirDependencyMapReader::getNestedIncludeDirsUsedBySourceFile(
        OovStringRef const srcName) const
    {
    std::set<IncludedPath> incFiles;
    getNestedIncludeFilesUsedBySourceFile(srcName, incFiles);
    OovStringSet tempDirs;
    for(const auto &incFile : incFiles)
        {
        tempDirs.insert(incFile.getIncDir());
        }
    OovStringVec incDirs(tempDirs.size());
    std::copy(tempDirs.begin(), tempDirs.end(), incDirs.begin());
    return incDirs;
    }

OovStringVec IncDirDependencyMapReader::getJavaExpandedFiles(
    OovStringRef const incPath) const
    {
    OovString path = incPath;
    size_t wildPos = path.find('*');
    OovStringVec files;
    if(wildPos != std::string::npos)
        {
        files = getJavaFilesDefinedInDirectory(path.substr(0, wildPos));
        }
    return files;
    }

/*
std::set<std::string> IncDirDependencyMapReader::getIncludeDirsUsedByDirectory(
        OovStringRef const compDir)
    {
    std::vector<std::string> sources = getFilesDefinedInDirectory(compDir);
    std::set<std::string> includes;
    for(auto const &src : sources)
        {
        std::vector<std::string> srcIncs =
                getIncludeDirsUsedBySourceFile(src);
        for(auto const &srcInc : srcIncs)
            {
            includes.insert(srcInc);
            }
        }
    return includes;
    }
*/

#define FAST_MATCH 1
#if(FAST_MATCH)
/// This is recursive
bool IncDirDependencyMapReader::anyRootDirsUsedBySourceFile(
        OovStringVec const &incRoots,
        OovStringSet &includesUsedSoFar,
        OovStringRef const srcName
        ) const
    {
    bool match = false;
    std::set<IncludedPath> incFiles;
    getImmediateIncludeFilesUsedBySourceFile(srcName, incFiles);
    for(const auto &incFile : incFiles)
        {
        auto const insertRet = includesUsedSoFar.insert(incFile.getFullPath());
        if(insertRet.second)
            {
            for(auto const &root: incRoots)
                {
                if(root.find(incFile.getIncDir()) != std::string::npos)
                    {
                    match = true;
                    break;
                    }
                }
            if(match)
                break;
            match = anyRootDirsUsedBySourceFile(incRoots, includesUsedSoFar,
                    incFile.getFullPath());
            if(match)
                break;
            }
        }
    return match;
    }
#endif

bool IncDirDependencyMapReader::anyRootDirsMatch(OovStringVec const &incRoots,
        OovStringRef const dirName) const
    {
    bool match = false;
    OovStringSet includesUsedSoFar;
    OovStringVec sources = getFilesDefinedInDirectory(dirName);
    for(auto const &src : sources)
        {
#if(FAST_MATCH)
        match = anyRootDirsUsedBySourceFile(incRoots, includesUsedSoFar, src);
#else
        std::vector<std::string> srcIncDirs =
                getNestedIncludeDirsUsedBySourceFile(src);
        for(auto const &srcIncDir : srcIncDirs)
            {
            auto const insertRet = includesUsedSoFar.insert(srcIncDir);
            if(insertRet.second)
                {
                match = (std::find(incRoots.begin(), incRoots.end(),
                        srcIncDir) != incRoots.end());
                }
            if(match)
                break;
            }
#endif
        if(match)
            break;
        }
    return match;
    }

class DirInfo
    {
    public:
        DirInfo():
            mMatchLen(0), mMatchIndex(NoIndex)
            {}
        static const size_t NoIndex = static_cast<size_t>(-1);
        size_t mMatchLen;
        size_t mMatchIndex;
    };

OovStringVec IncDirDependencyMapReader::getOrderedIncludeDirsForSourceFile(OovStringRef const absSrcName,
        OovStringVec const &orderedIncRoots) const
    {
    OovStringVec incDirs;
    // put directories in search path order.
    std::vector<DirInfo> unorderedDirInfo;
    std::vector<OovString> unorderedIncDirs =
            getNestedIncludeDirsUsedBySourceFile(absSrcName);
    unorderedDirInfo.resize(unorderedIncDirs.size());
    // Go through all directories and for each, find the longest root directory that matches.
    for(size_t incDirI = 0; incDirI < unorderedIncDirs.size(); incDirI++)
        {
        auto &incInfo = unorderedDirInfo[incDirI];
        for(size_t incRootI = 0; incRootI < orderedIncRoots.size(); incRootI++)
            {
            auto const &incRoot = orderedIncRoots[incRootI];
            if(unorderedIncDirs[incDirI].find(incRoot) != std::string::npos)
                {
                if(incRoot.length() > incInfo.mMatchLen)
                    {
                    incInfo.mMatchLen = incRoot.length();
                    incInfo.mMatchIndex = incRootI;
                    }
                }
            }
#if DEBUG_FINDER
        if(incInfo.mMatchLen == 0)
            {
            for(auto const incRoot : orderedIncRoots)
                fprintf(sLog.mFp, "incRoot = %s\n", incRoot.getStr());
            for(auto const incDir : unorderedIncDirs)
                fprintf(sLog.mFp, "unord = %s\n", incDir.getStr());
            }
#endif
        }
    for(size_t incRootI = 0; incRootI < orderedIncRoots.size(); incRootI++)
        {
        for(size_t incDirI = 0; incDirI < unorderedIncDirs.size(); incDirI++)
            {
            auto &incInfo = unorderedDirInfo[incDirI];
            if(incInfo.mMatchIndex == incRootI)
                {
                incDirs.push_back(unorderedIncDirs[incDirI]);
                }
            }
        }
    return incDirs;
    }

