/*
 * Project.cpp
 *
 *  Created on: Jul 16, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Project.h"
#include "Packages.h"

std::string Project::sProjectDirectory;
std::string Project::sSourceRootDirectory;

std::string makeBuildConfigArgName(char const * const baseName,
	char const * const buildConfig)
    {
    std::string name = baseName;
    name += '-';
    name += buildConfig;
    return name;
    }

std::string Project::getComponentTypesFilePath()
    {
    std::string fn = sProjectDirectory;
    ensureLastPathSep(fn);
    fn += "oovcde-comptypes.txt";
    return fn;
    }

std::string &Project::getSrcRootDirectory()
    {
    if(sSourceRootDirectory.length() == 0)
	{
	NameValueFile file(getProjectFilePath().c_str());
	file.readFile();
	sSourceRootDirectory = file.getValue(OptSourceRootDir);
	}
    return sSourceRootDirectory;
    }

std::string Project::getProjectFilePath()
    {
    std::string fn = sProjectDirectory;
    ensureLastPathSep(fn);
    fn += "oovcde.txt";
    return fn;
    }

std::string Project::getGuiOptionsFilePath()
    {
    std::string fn = sProjectDirectory;
    ensureLastPathSep(fn);
    fn += "oovcde-gui.txt";
    return fn;
    }

FilePath Project::getOutputDir(char const * const buildDirClass)
    {
    std::string outClass("out-");
    outClass += buildDirClass;
    FilePath dir(getProjectDirectory(), FP_Dir);
    dir.appendDir(outClass.c_str());
    return dir;
    }

FilePath Project::getIntermediateDir(char const * const buildDirClass)
    {
    std::string outClass("bld-");
    outClass += buildDirClass;
    FilePath dir(getProjectDirectory(), FP_Dir);
    dir.appendDir(outClass.c_str());
    return dir;
    }

void Project::getSrcRootDirRelativeSrcFileName(const std::string &srcFileName,
	const std::string &srcRootDir, std::string &relSrcFileName)
    {
    relSrcFileName = srcFileName;
    size_t pos = relSrcFileName.find(srcRootDir);
    if(pos != std::string::npos)
	{
	relSrcFileName.erase(pos, srcRootDir.length());
	removePathSep(relSrcFileName, 0);
	}
    }

void Project::makeOutBaseFileName(const std::string &srcFileName,
	const std::string &srcRootDir,
	const std::string &outFilePath, std::string &outFileName)
    {
    std::string file;
    getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir, file);
    if(file[0] == '/')
	file.erase(0, 1);
    replaceChars(file, '/', '_');
    replaceChars(file, '.', '_');
    outFileName = outFilePath;
    ensureLastPathSep(outFileName);
    outFileName += file;
    }

void Project::makeTreeOutBaseFileName(const std::string &srcFileName,
	const std::string &srcRootDir,
	const std::string &outFilePath, std::string &outFileName)
    {
    std::string file;
    getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir, file);
    if(file[0] == '/')
	file.erase(0, 1);

    FilePath tmpOutFileName(outFilePath, FP_Dir);
    tmpOutFileName.appendFile(file.c_str());
    tmpOutFileName.discardExtension();
    outFileName = tmpOutFileName;
    }

void Project::replaceChars(std::string &str, char oldC, char newC)
    {
    size_t pos;
    while((pos = str.find(oldC)) != std::string::npos)
	str.replace(pos, 1, 1, newC);
    }

bool ProjectReader::readOovProject(char const * const oovProjectDir,
	char const * const buildConfigName)
    {
    Project::setProjectDirectory(oovProjectDir);
    setFilename(Project::getProjectFilePath().c_str());
    bool success = readFile();
    if(success)
	{
	mProjectPackages.read();
	mBuildPackages.read();
	Project::setSourceRootDirectory(getValue(OptSourceRootDir).c_str());

	StdStringVec args;
	CompoundValue baseArgs;
	baseArgs.parseString(getValue(OptBaseArgs).c_str());
	for(auto const &arg : baseArgs)
	    {
	    args.push_back(arg.c_str());
	    }

	std::string optionExtraArgs = makeBuildConfigArgName(OptExtraBuildArgs,
		buildConfigName);
	CompoundValue extraArgs;
	extraArgs.parseString(getValue(optionExtraArgs.c_str()).c_str());
	extraArgs.quoteAllArgs();
	for(auto const &arg : extraArgs)
	    {
	    args.push_back(arg.c_str());
	    }
	parseArgs(args);
	}
    return success;
    }

void ProjectReader::parseArgs(StdStringVec const &args)
    {
    for(auto const &arg : args)
	{
	if(arg.find("-ER", 0, 3) == 0)
	    {
	    addExternalArg(arg.c_str());
	    }
	else if(arg.find("-EP", 0, 3) == 0)
	    {
	    }
	else if(arg.find("-lnk", 0, 4) == 0)
	    {
	    addLinkArg(arg.substr(4).c_str());
	    }
	else
	    {
	    addCompileArg(arg.c_str());
	    }
	}
    for(auto const &arg : args)
	{
	if(arg.find("-EP", 0, 3) == 0)
	    handleExternalPackage(arg.substr(3).c_str());
	}
    }

CompoundValue ProjectReader::getProjectExcludeDirs() const
    {
    CompoundValue val;
    val.parseString(getValue(OptProjectExcludeDirs).c_str());
    return val;
    }

void ProjectReader::handleExternalPackage(char const * const pkgName)
    {
    addPackageCrcName(pkgName);
    Package pkg = mProjectPackages.getPackage(pkgName);
    std::vector<std::string> cppArgs = pkg.getCompileArgs();
    for(auto const &arg : cppArgs)
	{
	addPackageCrcCompileArg(arg.c_str());
	}
    std::vector<std::string> incDirs = pkg.getIncludeDirs();
    for(auto const &dir : incDirs)
	{
	std::string arg = "-I";
	arg += dir;
	addPackageCrcCompileArg(arg.c_str());
	}
    std::vector<std::string> linkArgs = pkg.getLinkArgs();
    for(auto const &arg : linkArgs)
	{
	addPackageCrcLinkArg(arg.c_str());
	}
    std::vector<std::string> libDirs = pkg.getLibraryDirs();
    for(auto const &dir : libDirs)
	{
	std::string arg = "-L";
	arg += dir;
	addPackageCrcLinkArg(arg.c_str());
	}
    std::vector<std::string> libNames = pkg.getLibraryNames();
    for(auto const &dir : libNames)
	{
	std::string arg = "-l";
	arg += dir;
	addPackageCrcLinkArg(arg.c_str());
	}
    }

const StdStringVec ProjectReader::getAllCrcCompileArgs() const
    {
    StdStringVec vec;
    vec = mCompileArgs;
    std::copy(mPackageCrcCompileArgs.begin(), mPackageCrcCompileArgs.end(),
	    std::back_inserter(vec));
    std::copy(mPackageCrcNames.begin(), mPackageCrcNames.end(),
	    std::back_inserter(vec));
    return vec;
    }

const StdStringVec ProjectReader::getAllCrcLinkArgs() const
    {
    StdStringVec vec;
    vec = mLinkArgs;
    std::copy(mPackageCrcLinkArgs.begin(), mPackageCrcLinkArgs.end(),
	    std::back_inserter(vec));
    return vec;
    }
