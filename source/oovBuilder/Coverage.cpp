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
    bool success = file.readFile();
    if(success)
	{
	file.setFilename(dstFn);
	file.setNameValue(OptSourceRootDir, covSrcDir);
	file.writeFile();
	}
    else
	{
	fprintf(stderr, "Unable to make project file %s\n", srcFn.getStr());
	}
    return success;
    }

static bool makeCoverageComponentTypesFile(OovStringRef const srcFn, OovStringRef const dstFn)
    {
    ComponentTypesFile file;
    bool success = file.readTypesOnly(srcFn);
    if(success)
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
	file.writeTypesOnly(dstFn);
	}
    else
	{
	fprintf(stderr, "Unable to make component types file %s\n", srcFn.getStr());
	}
    return success;
    }

static bool copyPackageFile(OovStringRef const srcFn, OovStringRef const dstFn)
    {
    bool success = false;
    File srcFile(srcFn, "r");
    if(FileIsFileOnDisk(srcFn))
	{
	if(srcFile.isOpen())
	    {
	    FileEnsurePathExists(dstFn);
	    File dstFile(dstFn, "w");
	    if(dstFile.isOpen())
		{
		char buf[10000];
		while(fgets(buf, sizeof(buf), srcFile.getFp()))
		    {
		    fputs(buf, dstFile.getFp());
		    success = true;
		    }
		}
	    }
	}
    else
	success = true;
    if(!success)
	{
	fprintf(stderr, "Unable to copy package file %s\n", srcFn.getStr());
	}
    return success;
    }

bool makeCoverageBuildProject()
    {
    std::string origProjFilePath = Project::getProjectFilePath();
    std::string origCompTypesFilePath = Project::getComponentTypesFilePath();
    std::string origPackagesFilePath = Project::getPackagesFilePath();
    std::string covSrcDir = Project::getCoverageSourceDirectory();
    std::string covProjDir = Project::getCoverageProjectDirectory();
    bool success = FileEnsurePathExists(covProjDir);
    if(success)
	{
	Project::setProjectDirectory(covProjDir);
	std::string newProjFilePath = Project::getProjectFilePath();
	success = makeCoverageProjectFile(origProjFilePath,
	    newProjFilePath, covSrcDir);
	}
    if(success)
	{
	success = makeCoverageComponentTypesFile(origCompTypesFilePath,
	    Project::getComponentTypesFilePath());
	}
    if(success)
	{
	success = copyPackageFile(origPackagesFilePath,
	    Project::getPackagesFilePath());
	}
    return success;
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
    File file(fn, "r");
    mInstrCounts.clear();
    if(file.isOpen())
	{
	char buf[100];
	int lineCounter = 0;
	while(fgets(buf, sizeof(buf), file.getFp()))
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
    File statFile(statFn, "w");
    if(statFile.isOpen())
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
    else
	{
	fprintf(stderr, "Unable to open file %s\n", statFn.getStr());
	}
    }

/// Copy a single source file and make a comment that contains the hit count
/// for each instrumented line.
static void updateCovSourceCounts(OovStringRef const relSrcFn,
	std::vector<int> &counts)
    {
    FilePath srcFn(Project::getCoverageSourceDirectory(), FP_Dir);
    srcFn.appendFile(relSrcFn);
    File srcFile(srcFn, "r");
    if(srcFile.isOpen())
	{
	FilePath dstFn(Project::getCoverageProjectDirectory(), FP_Dir);
	dstFn.appendFile(relSrcFn);
	FileEnsurePathExists(dstFn);
	File dstFile(dstFn, "w");
	if(dstFile.isOpen())
	    {
	    char buf[1000];
	    size_t instrCount = 0;
	    while(fgets(buf, sizeof(buf), srcFile.getFp()))
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
		fputs(buf, dstFile.getFp());
		}
	    }
	else
	    {
	    fprintf(stderr, "Unable to write file %s\n", dstFn.getStr());
	    }
	}
    else
	{
	fprintf(stderr, "Unable to read file %s\n", srcFn.getStr());
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
	covHeaderReader.read(covHeaderFile);
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




