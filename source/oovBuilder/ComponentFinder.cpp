/*
 * projFinder.cpp
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "ComponentFinder.h"
#include "Project.h"
#include "Debug.h"
#include <set>

#define DEBUG_FINDER 0
#if DEBUG_FINDER
static DebugFile sLog("oov-finder-debug.txt");
#endif

void ToolPathFile::getPaths()
    {
    if(mPathCompiler.length() == 0)
	{
	std::string projFileName = Project::getProjectFilePath();
	setFilename(projFileName.c_str());
	readFile();

	std::string optStr = makeBuildConfigArgName(OptToolLibPath, mBuildConfig.c_str());
	mPathLibber = getValue(optStr.c_str());
	optStr = makeBuildConfigArgName(OptToolCompilePath, mBuildConfig.c_str());
	mPathCompiler = getValue(optStr.c_str());
	optStr = makeBuildConfigArgName(OptToolObjSymbolPath, mBuildConfig.c_str());
	mPathObjSymbol = getValue(optStr.c_str());
	}
    }

std::string ToolPathFile::getAnalysisToolPath()
    {
	// ./ is needed in Linux
    return("./oovCppParser");
    }

std::string ToolPathFile::getCompilerPath()
    {
    getPaths();
    return(mPathCompiler);
    }

std::string ToolPathFile::getObjSymbolPath()
    {
    getPaths();
    return(mPathObjSymbol);
    }

std::string ToolPathFile::getLibberPath()
    {
    getPaths();
    return(mPathLibber);
    }

std::string ToolPathFile::getCovInstrToolPath()
    {
	// ./ is needed in Linux
    return("./oovCovInstr");
    }

bool FileStat::isOutputOld(char const * const outputFn,
	char const * const inputFn)
    {
    time_t outTime;
    time_t inTime;
    bool success = getFileTime(outputFn, outTime);
    bool old = !success;
    if(success)
	{
	success = getFileTime(inputFn, inTime);
	if(success)
	    old = inTime > outTime;
	else
	    old = true;
	}
    return old;
    }

bool FileStat::isOutputOld(char const * const outputFn,
	const std::vector<std::string> &inputs, int *oldIndex)
    {
    bool old = false;
    for(size_t i=0; i<inputs.size(); i++)
	{
	if(isOutputOld(outputFn, inputs[i].c_str()))
	    {
	    old = true;
	    if(oldIndex)
		*oldIndex = i;
	    break;
	    }
	}
    return old;
    }

void ScannedComponent::saveComponentSourcesToFile(char const * const compName,
    ComponentTypesFile &compFile) const
    {
    compFile.setComponentSources(compName, mSourceFiles);
    compFile.setComponentIncludes(compName, mIncludeFiles);
    }

//////////////

ScannedComponentsInfo::MapIter ScannedComponentsInfo::addComponents(
    char const * const compName)
    {
    FilePath parent = FilePath(compName, FP_Dir).getParent();
    if(parent.length() > 0)
	{
	removePathSep(parent, parent.length()-1);
        addComponents(parent.c_str());
	}
    MapIter it = mComponents.find(compName);
    if(it == mComponents.end())
	{
	it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
	}
    return it;
    }

void ScannedComponentsInfo::addSourceFile(char const * const compName,
    char const * const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addIncludeFile(char const * const compName,
    char const * const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addIncludeFileName(srcFileName);
    }

static void setFileValues(char const * const tagName,
    const std::vector<std::string> &vals, ComponentsFile &compFile)
    {
    CompoundValue incArgs;
    for(const auto &inc : vals)
	{
	incArgs.addArg(inc.c_str());
	}
    compFile.setNameValue(tagName, incArgs.getAsString().c_str());
    }

void ScannedComponentsInfo::setProjectComponentsFileValues(ComponentsFile &compFile)
    {
    setFileValues("Components-init-proj-incs", mProjectIncludeDirs, compFile);
    }

void ScannedComponentsInfo::initializeComponentTypesFileValues(
    ComponentTypesFile &compFile)
    {
    CompoundValue comps;
    for(const auto &comp : mComponents)
	{
	comps.addArg(comp.first.c_str());
	comp.second.saveComponentSourcesToFile(comp.first.c_str(), compFile);
	}
    compFile.setComponentNames(comps.getAsString().c_str());
    }

//////////////

bool ComponentFinder::readProject(char const * const oovProjectDir,
	char const * const buildConfigName)
    {
    bool success = mProject.readOovProject(oovProjectDir, buildConfigName);
    if(success)
	{
	mComponentTypesFile.read();
	}
    return success;
    }

void ComponentFinder::scanProject()
    {
    mScanningPackage = nullptr;
    mExcludeDirs = mProject.getProjectExcludeDirs();
    recurseDirs(mProject.getSrcRootDirectory().c_str());
    }

void ComponentFinder::scanExternalProject(char const * const externalRootSrch,
	Package const *pkg)
    {
    std::string externalRootDir;
    ComponentsFile::parseProjRefs(externalRootSrch, externalRootDir, mExcludeDirs);

    Package rootPkg;
    if(pkg)
	rootPkg = *pkg;
    rootPkg.setRootDirPackage(externalRootSrch);
    mScanningPackage = &rootPkg;

    recurseDirs(externalRootDir.c_str());

    addBuildPackage(rootPkg);
    }

void ComponentFinder::addBuildPackage(Package const &pkg)
    {
    getProject().getBuildPackages().insertPackage(pkg);
    getProject().getBuildPackages().savePackages();
    }

void ComponentFinder::saveProject(char const * const compFn)
    {
    mComponentsFile.read(compFn);
    mScannedInfo.setProjectComponentsFileValues(mComponentsFile);
    mScannedInfo.initializeComponentTypesFileValues(mComponentTypesFile);
    mComponentTypesFile.writeFile();
    mComponentsFile.writeFile();
    }

std::string ComponentFinder::getComponentName(char const * const filePath) const
    {
    std::string compName;
    FilePath path(filePath, FP_File);
    path.discardFilename();
    Project::getSrcRootDirRelativeSrcFileName(path,
	    mProject.getSrcRootDirectory().c_str(), compName);
    if(compName.length() == 0)
    	{
	path.moveLeftPathSep();
	mRootPathName = path.getPathSegment();
	compName = "<Root>";
    	}
    else
	{
	removePathSep(compName, compName.length()-1);
	}
    return compName;
    }

bool ComponentFinder::processFile(const std::string &filePath)
    {
    /// @todo - find files with no extension? to match things like std::vector include
    if(!ComponentsFile::excludesMatch(filePath, mExcludeDirs))
	{
	bool inc = isHeader(filePath.c_str());
	bool src = isSource(filePath.c_str());
	bool lib = isLibrary(filePath.c_str());
	if(mScanningPackage)
	    {
	    if(inc)
		{
		FilePath path(filePath, FP_File);
		path.discardFilename();

		mScanningPackage->appendAbsoluteIncDir(path.getDrivePath().c_str());

		// Insert parent for things like #include "gtk/gtk.h" or "sys/stat.h"
		FilePath parent(path.getParent());
		if(parent.length() > 0)
		    mScanningPackage->appendAbsoluteIncDir(parent.getDrivePath().c_str());

		// Insert grandparent for things like "llvm/ADT/APInt.h"
		FilePath grandparent(parent.getParent());
		if(grandparent.length() > 0)
		    mScanningPackage->appendAbsoluteIncDir(grandparent.getDrivePath().c_str());
		}
	    else if(lib)
		{
		FilePath path(filePath, FP_File);
		mScanningPackage->appendAbsoluteLibName(filePath.c_str());
		}
	    }
	else
	    {
	    if(src)
		{
		std::string name = getComponentName(filePath.c_str());
		mScannedInfo.addSourceFile(name.c_str(), filePath.c_str());
		}
	    else if(inc)
		{
		FilePath path(filePath, FP_File);
		path.discardFilename();
		mScannedInfo.addProjectIncludePath(path.c_str());
		std::string name = getComponentName(filePath.c_str());
		mScannedInfo.addIncludeFile(name.c_str(), filePath.c_str());
		}
	    }
	}
    return true;
    }

std::vector<std::string> ComponentFinder::getAllIncludeDirs() const
    {
    InsertOrderedSet projIncs = getScannedInfo().getProjectIncludeDirs();
    std::vector<std::string> incs;
    std::copy(projIncs.begin(), projIncs.end(), std::back_inserter(incs));
    for(auto const &pkg : getProject().getBuildPackages().getPackages())
	{
	std::vector<std::string> pkgIncs = pkg.getIncludeDirs();
	std::copy(pkgIncs.begin(), pkgIncs.end(), std::back_inserter(incs));
	}
    return incs;
    }


void CppChildArgs::addCompileArgList(const ComponentFinder &finder,
	const std::vector<std::string> &incDirs)
    {
    // add cpp args
    for(const auto &arg : finder.getProject().getCompileArgs())
	{
	std::string tempArg = arg;
	CompoundValue::quoteCommandLineArg(tempArg);
	addArg(tempArg.c_str());
	}

    for(int i=0; static_cast<size_t>(i)<incDirs.size(); i++)
	{
	std::string incStr = std::string("-I") + incDirs[i];
	CompoundValue::quoteCommandLineArg(incStr);
	addArg(incStr.c_str());
	}
    }

