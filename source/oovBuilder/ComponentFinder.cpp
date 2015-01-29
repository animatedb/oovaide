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
	setFilename(projFileName);
	readFile();

	std::string optStr = makeBuildConfigArgName(OptToolLibPath, mBuildConfig);
	mPathLibber = getValue(optStr);
	optStr = makeBuildConfigArgName(OptToolCompilePath, mBuildConfig);
	mPathCompiler = getValue(optStr);
	optStr = makeBuildConfigArgName(OptToolObjSymbolPath, mBuildConfig);
	mPathObjSymbol = getValue(optStr);
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

bool FileStat::isOutputOld(OovStringRef const outputFn,
	OovStringRef const inputFn)
    {
    time_t outTime;
    time_t inTime;
    bool success = FileGetFileTime(outputFn, outTime);
    bool old = !success;
    if(success)
	{
	success = FileGetFileTime(inputFn, inTime);
	if(success)
	    old = inTime > outTime;
	else
	    old = true;
	}
    return old;
    }

bool FileStat::isOutputOld(OovStringRef const outputFn,
	OovStringVec const &inputs, int *oldIndex)
    {
    bool old = false;
    for(size_t i=0; i<inputs.size(); i++)
	{
	if(isOutputOld(outputFn, inputs[i]))
	    {
	    old = true;
	    if(oldIndex)
		*oldIndex = i;
	    break;
	    }
	}
    return old;
    }

void ScannedComponent::saveComponentSourcesToFile(OovStringRef const compName,
    ComponentTypesFile &compFile) const
    {
    compFile.setComponentSources(compName, mSourceFiles);
    compFile.setComponentIncludes(compName, mIncludeFiles);
    }

//////////////

ScannedComponentsInfo::MapIter ScannedComponentsInfo::addComponents(
    OovStringRef const compName)
    {
    FilePath parent = FilePath(compName, FP_Dir).getParent();
    if(parent.length() > 0)
	{
	FilePathRemovePathSep(parent, parent.length()-1);
        addComponents(parent);
	}
    MapIter it = mComponents.find(compName);
    if(it == mComponents.end())
	{
	it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
	}
    return it;
    }

void ScannedComponentsInfo::addSourceFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addIncludeFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addIncludeFileName(srcFileName);
    }

static void setFileValues(OovStringRef const tagName,
    OovStringVec const &vals, ComponentsFile &compFile)
    {
    CompoundValue incArgs;
    for(const auto &inc : vals)
	{
	incArgs.addArg(inc);
	}
    compFile.setNameValue(tagName, incArgs.getAsString());
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
	comps.addArg(comp.first);
	comp.second.saveComponentSourcesToFile(comp.first, compFile);
	}
    compFile.setComponentNames(comps.getAsString());
    }

//////////////

bool ComponentFinder::readProject(OovStringRef const oovProjectDir,
	OovStringRef const buildConfigName)
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
    recurseDirs(mProject.getSrcRootDirectory().getStr());
    }

void ComponentFinder::scanExternalProject(OovStringRef const externalRootSrch,
	Package const *pkg)
    {
    OovString externalRootDir;
    ComponentsFile::parseProjRefs(externalRootSrch, externalRootDir, mExcludeDirs);

    Package rootPkg;
    if(pkg)
	rootPkg = *pkg;
    rootPkg.setRootDirPackage(externalRootSrch);
    mScanningPackage = &rootPkg;

    recurseDirs(externalRootDir.getStr());

    addBuildPackage(rootPkg);
    }

void ComponentFinder::addBuildPackage(Package const &pkg)
    {
    getProject().getBuildPackages().insertPackage(pkg);
    getProject().getBuildPackages().savePackages();
    }

void ComponentFinder::saveProject(OovStringRef const compFn)
    {
    mComponentsFile.read(compFn);
    mScannedInfo.setProjectComponentsFileValues(mComponentsFile);
    mScannedInfo.initializeComponentTypesFileValues(mComponentTypesFile);
    mComponentTypesFile.writeFile();
    mComponentsFile.writeFile();
    }

OovString ComponentFinder::getComponentName(OovStringRef const filePath) const
    {
    FilePath path(filePath, FP_File);
    path.discardFilename();
    OovString compName = Project::getSrcRootDirRelativeSrcFileName(path,
	    mProject.getSrcRootDirectory());
    if(compName.length() == 0)
    	{
	mRootPathName = path.getPathSegment(path.getPosEndDir());
	compName = "<Root>";
    	}
    else
	{
	FilePathRemovePathSep(compName, compName.length()-1);
	}
    return compName;
    }

bool ComponentFinder::processFile(OovStringRef const filePath)
    {
    /// @todo - find files with no extension? to match things like std::vector include
    if(!ComponentsFile::excludesMatch(filePath, mExcludeDirs))
	{
	bool inc = isHeader(filePath);
	bool src = isSource(filePath);
	bool lib = isLibrary(filePath);
	if(mScanningPackage)
	    {
	    if(inc)
		{
		FilePath path(filePath, FP_File);
		path.discardFilename();

		mScanningPackage->appendAbsoluteIncDir(path.getDrivePath());

		// Insert parent for things like #include "gtk/gtk.h" or "sys/stat.h"
		FilePath parent(path.getParent());
		if(parent.length() > 0)
		    mScanningPackage->appendAbsoluteIncDir(parent.getDrivePath());

		// Insert grandparent for things like "llvm/ADT/APInt.h"
		FilePath grandparent(parent.getParent());
		if(grandparent.length() > 0)
		    mScanningPackage->appendAbsoluteIncDir(grandparent.getDrivePath());
		}
	    else if(lib)
		{
		FilePath path(filePath, FP_File);
		mScanningPackage->appendAbsoluteLibName(filePath);
		}
	    }
	else
	    {
	    if(src)
		{
		std::string name = getComponentName(filePath);
		mScannedInfo.addSourceFile(name, filePath);
		}
	    else if(inc)
		{
		FilePath path(filePath, FP_File);
		path.discardFilename();
		mScannedInfo.addProjectIncludePath(path);
		std::string name = getComponentName(filePath);
		mScannedInfo.addIncludeFile(name, filePath);
		}
	    }
	}
    return true;
    }

OovStringVec ComponentFinder::getAllIncludeDirs() const
    {
    InsertOrderedSet projIncs = getScannedInfo().getProjectIncludeDirs();
    OovStringVec incs;
    std::copy(projIncs.begin(), projIncs.end(), std::back_inserter(incs));
    for(auto const &pkg : getProject().getBuildPackages().getPackages())
	{
	OovStringVec pkgIncs = pkg.getIncludeDirs();
	std::copy(pkgIncs.begin(), pkgIncs.end(), std::back_inserter(incs));
	}
    return incs;
    }


void CppChildArgs::addCompileArgList(const ComponentFinder &finder,
	const OovStringVec &incDirs)
    {
    // add cpp args
    for(const auto &arg : finder.getProject().getCompileArgs())
	{
	std::string tempArg = arg;
	CompoundValue::quoteCommandLineArg(tempArg);
	addArg(tempArg);
	}

    for(int i=0; static_cast<size_t>(i)<incDirs.size(); i++)
	{
	std::string incStr = std::string("-I") + incDirs[i];
	CompoundValue::quoteCommandLineArg(incStr);
	addArg(incStr);
	}
    }

