/*
 * coverage.cpp
 *
 *  Created on: Sep 17, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Coverage.h"
#include "CoverageHeaderReader.h"
#include "Components.h"
#include "Project.h"
#include <string.h>

static bool makeCoverageProjectFile(OovStringRef const srcFn, OovStringRef const dstFn,
        OovStringRef const covSrcDir)
    {
    NameValueFile file(srcFn);
    OovStatus status = file.readFile();
    if(status.ok())
        {
        file.setFilename(dstFn);
        file.setNameValue(OptSourceRootDir, covSrcDir);
        status = file.writeFile();
        }
    if(status.needReport())
        {
        OovString err = "Unable to make project file ";
        err += srcFn;
        status.report(ET_Error, err);
        }
    return status.ok();
    }

static bool makeCoverageComponentTypesFile(OovStringRef const srcFn, OovStringRef const dstFn)
    {
    ComponentTypesFile file;
    OovStatus status = file.readTypesOnly(srcFn);
    if(status.ok())
        {
        // Define a static library that contains the code that stores
        // the coverage counts that is compiled into exectuables.
        static OovStringRef const covLibName = Project::getCovLibName();
        CompoundValue names = file.getComponentNames();
        if(names.find(covLibName) == CompoundValue::npos)
            {
            names.push_back(covLibName);
            file.setComponentNames(names.getAsString());
            file.setComponentType(covLibName,
                    ComponentTypesFile::getShortComponentTypeName(
                            ComponentTypesFile::CT_StaticLib));
            }
        status = file.writeTypesOnly(dstFn);
        }
    if(status.needReport())
        {
        OovString err = "Unable to make component types file ";
        err += srcFn;
        status.report(ET_Error, err);
        }
    return status.ok();
    }


/// The package file is only copied if it doesn't exist or is old.  If it was
/// always copied, then the date would be updated which would cause a full
/// rebuild of all analysis and build files.
static bool copyPackageFileIfNeeded(OovStringRef const srcFn, OovStringRef const dstFn)
    {
    OovStatus status(true, SC_File);
    if(FileStat::isOutputOld(dstFn, srcFn, status))
        {
        File srcFile;
        status = srcFile.open(srcFn, "r");
        if(status.ok())
            {
            if(FileIsFileOnDisk(srcFn, status))
                {
                if(srcFile.isOpen())
                    {
                    status = FileEnsurePathExists(dstFn);
                    if(status.ok())
                        {
                        File dstFile;
                        status = dstFile.open(dstFn, "w");
                        if(status.ok())
                            {
                            char buf[10000];
                            while(srcFile.getString(buf, sizeof(buf), status))
                                {
                                status = dstFile.putString(buf);
                                if(!status.ok())
                                    {
                                    break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        if(status.needReport())
            {
            OovString err = "Unable to copy package file ";
            err += srcFn;
            status.report(ET_Error, err);
            }
        }
    return status.ok();
    }

bool makeCoverageBuildProject()
    {
    std::string origProjFilePath = Project::getProjectFilePath();
    std::string origCompTypesFilePath = Project::getComponentTypesFilePath();
    std::string origPackagesFilePath = Project::getPackagesFilePath();
    std::string covSrcDir = Project::getCoverageSourceDirectory();
    std::string covProjDir = Project::getCoverageProjectDirectory();
    bool success = true;
    OovStatus status = FileEnsurePathExists(covProjDir);
    if(status.ok())
        {
        Project::setProjectDirectory(covProjDir);
        std::string newProjFilePath = Project::getProjectFilePath();
        success = makeCoverageProjectFile(origProjFilePath,
            newProjFilePath, covSrcDir);
        if(success)
            {
            success = makeCoverageComponentTypesFile(origCompTypesFilePath,
                Project::getComponentTypesFilePath());
            }
        if(success)
            {
            success = copyPackageFileIfNeeded(origPackagesFilePath,
                Project::getPackagesFilePath());
            }
        }
    return status.ok() && success;
    }

class CoverageCountsReader
    {
    public:
    CoverageCountsReader():
            mNumInstrumentedLines(0)
            {}
        void read(OovStringRef const fn);
        int getNumInstrumentedLines() const
            { return mNumInstrumentedLines; }
        std::vector<int> const &getCounts() const
            { return mInstrCounts; }

    private:
        int mNumInstrumentedLines;
        std::vector<int> mInstrCounts;
    };

void CoverageCountsReader::read(OovStringRef const fn)
    {
    File file;
    OovStatus status = file.open(fn, "r");
    mInstrCounts.clear();
    if(status.ok())
        {
        char buf[100];
        int lineCounter = 0;
        while(file.getString(buf, sizeof(buf), status))
            {
            lineCounter++;
            int val;
            if(sscanf(buf, "%d", &val) == 1)
                {
                if(lineCounter == 1)
                    {
                    mNumInstrumentedLines = val;
                    }
                else
                    {
                    mInstrCounts.push_back(val);
                    }
                }
            }
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read coverage counts");
        }
    }

/// Use the filename to make an identifier.
static std::string makeOrigCovFn(OovStringRef const fn)
    {
    OovString covFn = fn;
    if(covFn.find("COV_") != std::string::npos)
        {
        covFn.erase(0, 4);
        }
    covFn.replaceStrs("_", "/");
    size_t pos = covFn.rfind('/');
    if(pos != std::string::npos)
        {
        covFn.replace(pos, 1, ".");
        }
    return covFn;
    }

/// Make a stats file that contains the percentage of instrumented
/// lines that have been executed for each source file.
static void makeCoverageStats(CoverageHeaderReader const &covHeader,
        CoverageCountsReader const &covCounts)
    {
    FilePath statFn(Project::getCoverageProjectDirectory(), FP_Dir);
    statFn.appendFile("oovCovStats.txt");
    File statFile;
    OovStatus status = statFile.open(statFn, "w");
    if(status.ok())
        {
        size_t countIndex = 0;
        std::vector<int> const &counts = covCounts.getCounts();
        for(auto const &mapItem : covHeader.getMap())
            {
            int count = mapItem.second;
            int hits = 0;
            for(int i=0; i<count; i++)
                {
                if(countIndex < counts.size())
                    {
                    if(counts[countIndex++])
                        {
                        hits++;
                        }
                    }
                }
            int percent = 0;
            if(count > 0)
                {
                percent = (hits * 100) / count;
                }
            else
                {
                percent = 100;
                }
            OovString covFn = makeOrigCovFn(mapItem.first);
            fprintf(statFile.getFp(), "%s %d\n", covFn.getStr(), percent);
            }
        }
    if(status.needReport())
        {
        OovString err = "Unable to open file ";
        err += statFn;
        status.report(ET_Error, err);
        }
    }

/// Copy a single source file and make a comment that contains the hit count
/// for each instrumented line.
static void updateCovSourceCounts(OovStringRef const relSrcFn,
        std::vector<int> &counts)
    {
    FilePath srcFn(Project::getCoverageSourceDirectory(), FP_Dir);
    srcFn.appendFile(relSrcFn);
    FilePath dstFn(Project::getCoverageProjectDirectory(), FP_Dir);
    dstFn.appendFile(relSrcFn);
    File srcFile;
    OovStatus status = srcFile.open(srcFn, "r");
    if(status.ok())
        {
        status = FileEnsurePathExists(dstFn);
        if(status.ok())
            {
            File dstFile;
            status = dstFile.open(dstFn, "w");
            if(status.ok())
                {
                char buf[1000];
                size_t instrCount = 0;
                while(srcFile.getString(buf, sizeof(buf), status))
                    {
                    if(strstr(buf, "COV_IN("))
                        {
                        if(instrCount < counts.size())
                            {
                            OovString countStr = "    // ";
                            countStr.appendInt(counts[instrCount]);
                            OovString newStr = buf;
                            size_t pos = newStr.find('\n');
                            newStr.insert(pos, countStr);
                            if(newStr.length() < sizeof(buf)-1)
                                {
                                strcpy(buf, newStr.getStr());
                                }
                            }
                        instrCount++;
                        }
                    status = dstFile.putString(buf);
                    if(!status.ok())
                        {
                        break;
                        }
                    }
                }
            }
        }
    if(status.needReport())
        {
        OovString err = "Unable to transfer coverage ";
        err += srcFn;
        err += " ";
        err += dstFn;
        status.report(ET_Error, err);
        }
    }

/// Copy each source file and make a comment that contains the hit count
/// for each instrumented line.
static void updateCovSourceCounts(CoverageHeaderReader const &covHeader,
        CoverageCountsReader const &covCounts)
    {
    std::vector<int> const &counts = covCounts.getCounts();
    size_t countIndex = 0;
    for(auto const &mapItem : covHeader.getMap())
        {
        int count = mapItem.second;
        std::vector<int> fileCounts;
        for(int i=0; i<count; i++)
            {
            if(countIndex < counts.size())
                {
                fileCounts.push_back(counts[countIndex++]);
                }
            }
        std::string covFn = makeOrigCovFn(mapItem.first);
        updateCovSourceCounts(covFn, fileCounts);
        }
    }

bool makeCoverageStats()
    {
    bool success = false;
    SharedFile covHeaderFile;
    OovString covHeaderFn = CoverageHeaderReader::getFn(
            Project::getCoverageSourceDirectory());
    eOpenStatus stat = covHeaderFile.open(covHeaderFn, M_ReadShared, OE_Binary);
    if(stat == OS_Opened)
        {
        CoverageHeaderReader covHeaderReader;
        OovStatus status = covHeaderReader.read(covHeaderFile);
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to read coverage header");
            }
        int headerInstrLines = covHeaderReader.getNumInstrumentedLines();
        if(headerInstrLines > 0)
            {
            success = true;
            FilePath covCountsFn(Project::getCoverageProjectDirectory(), FP_Dir);
            covCountsFn.appendDir("out-Debug");
            static char const covCountsFnStr[] = "OovCoverageCounts.txt";
            covCountsFn.appendFile(covCountsFnStr);
            CoverageCountsReader covCounts;
            covCounts.read(covCountsFn);
            int covInstrLines = covCounts.getNumInstrumentedLines();
            if(headerInstrLines == covInstrLines)
                {
                makeCoverageStats(covHeaderReader, covCounts);
                updateCovSourceCounts(covHeaderReader, covCounts);
                }
            else
                {
                fprintf(stderr, "Number of OovCoverage.h lines %d don't match %s lines %d\n",
                        headerInstrLines, covCountsFnStr, covInstrLines);
                }
            }
        else
            {
            fprintf(stderr, "No lines are instrumented in %s\n", covHeaderFn.getStr());
            }
        }
    else
        {
        fprintf(stderr, "Unable to open file %s\n", covHeaderFn.getStr());
        }
    return success;
    }




