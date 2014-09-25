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

static bool makeCoverageProjectFile(char const * const srcFn, char const * const dstFn,
	char const * const covSrcDir)
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
	fprintf(stderr, "Unable to make project file %s\n", srcFn);
	}
    return success;
    }

static bool makeCoverageComponentTypesFile(char const * const srcFn, char const * const dstFn)
    {
    ComponentTypesFile file;
    bool success = file.readTypesOnly(srcFn);
    if(success)
	{
	// Define a static library that contains the code that stores
	// the coverage counts that is compiled into exectuables.
	static char const * const covLibName = Project::getCovLibName();
	CompoundValue names = file.getComponentNames();
	if(names.find(covLibName) == CompoundValue::npos)
	    {
	    names.push_back(covLibName);
	    file.setComponentNames(names.getAsString().c_str());
	    file.setComponentType(covLibName,
		    ComponentTypesFile::getShortComponentTypeName(
			    ComponentTypesFile::CT_StaticLib));
	    }
	file.writeTypesOnly(dstFn);
	}
    else
	{
	fprintf(stderr, "Unable to make component types file %s\n", srcFn);
	}
    return success;
    }

bool copyPackageFile(char const * const srcFn, char const * const dstFn)
    {
    bool success = false;
    File srcFile(srcFn, "r");
    if(fileExists(srcFn))
	{
	if(srcFile.isOpen())
	    {
	    ensurePathExists(dstFn);
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
	fprintf(stderr, "Unable to copy package file %s\n", srcFn);
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
    bool success = ensurePathExists(covProjDir.c_str());
    if(success)
	{
	Project::setProjectDirectory(covProjDir.c_str());
	std::string newProjFilePath = Project::getProjectFilePath();
	success = makeCoverageProjectFile(origProjFilePath.c_str(),
	    newProjFilePath.c_str(), covSrcDir.c_str());
	}
    if(success)
	{
	success = makeCoverageComponentTypesFile(origCompTypesFilePath.c_str(),
	    Project::getComponentTypesFilePath().c_str());
	}
    if(success)
	{
	success = copyPackageFile(origPackagesFilePath.c_str(),
	    Project::getPackagesFilePath().c_str());
	}
    return success;
    }

class CoverageCountsReader
    {
    public:
    CoverageCountsReader():
	    mNumInstrumentedLines(0)
	    {}
	void read(char const * const fn);
	int getNumInstrumentedLines() const
	    { return mNumInstrumentedLines; }
	std::vector<int> const &getCounts() const
	    { return mInstrCounts; }

    private:
	int mNumInstrumentedLines;
	std::vector<int> mInstrCounts;
    };

void CoverageCountsReader::read(char const * const fn)
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
static std::string makeOrigCovFn(char const * const fn)
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
    FilePath statFn(Project::getCoverageProjectDirectory().c_str(), FP_Dir);
    statFn.appendFile("oovCovStats.txt");
    File statFile(statFn.c_str(), "w");
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
	    std::string covFn = makeOrigCovFn(mapItem.first.c_str());
	    fprintf(statFile.getFp(), "%s %d\n", covFn.c_str(), percent);
	    }
	}
    else
	{
	fprintf(stderr, "Unable to open file %s\n", statFn.c_str());
	}
    }

/// Copy a single source file and make a comment that contains the hit count
/// for each instrumented line.
static void updateCovSourceCounts(char const * const relSrcFn,
	std::vector<int> &counts)
    {
    FilePath srcFn(Project::getCoverageSourceDirectory().c_str(), FP_Dir);
    srcFn.appendFile(relSrcFn);
    File srcFile(srcFn.c_str(), "r");
    if(srcFile.isOpen())
	{
	FilePath dstFn(Project::getCoverageProjectDirectory().c_str(), FP_Dir);
	dstFn.appendFile(relSrcFn);
	ensurePathExists(dstFn.c_str());
	File dstFile(dstFn.c_str(), "w");
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
			std::string newStr = buf;
			size_t pos = newStr.find('\n');
			newStr.insert(pos, countStr);
			if(newStr.length() < sizeof(buf)-1)
			    {
			    strcpy(buf, newStr.c_str());
			    }
			}
		    instrCount++;
		    }
		fputs(buf, dstFile.getFp());
		}
	    }
	else
	    {
	    fprintf(stderr, "Unable to write file %s\n", dstFn.c_str());
	    }
	}
    else
	{
	fprintf(stderr, "Unable to read file %s\n", srcFn.c_str());
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
	std::string covFn = makeOrigCovFn(mapItem.first.c_str());
	updateCovSourceCounts(covFn.c_str(), fileCounts);
	}
    }

/// This uses the coverage header file that was generated by oovCovInstr to
/// get all of the source file names number of instrumented lines in each file.
/// Then it gets the OovCoverageCounts.txt file that matches the total
/// number of instrumented lines to get coverage information.
bool makeCoverageStats()
    {
    bool success = false;
    SharedFile covHeaderFile;
    std::string covHeaderFn = CoverageHeaderReader::getFn(
	    Project::getCoverageSourceDirectory().c_str());
    eOpenStatus stat = covHeaderFile.open(covHeaderFn.c_str(), M_ReadShared, OE_Binary);
    if(stat == OS_Opened)
	{
	CoverageHeaderReader covHeaderReader;
	covHeaderReader.read(covHeaderFile);
	int headerInstrLines = covHeaderReader.getNumInstrumentedLines();
	if(headerInstrLines > 0)
	    {
	    success = true;
	    FilePath covCountsFn(Project::getCoverageProjectDirectory().c_str(), FP_Dir);
	    covCountsFn.appendDir("out-Debug");
	    static char const covCountsFnStr[] = "OovCoverageCounts.txt";
	    covCountsFn.appendFile(covCountsFnStr);
	    CoverageCountsReader covCounts;
	    covCounts.read(covCountsFn.c_str());
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
	    fprintf(stderr, "No lines are instrumented in %s\n", covHeaderFn.c_str());
	    }
	}
    else
	{
	fprintf(stderr, "Unable to open file %s\n", covHeaderFn.c_str());
	}
    return success;
    }




