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

void ScannedComponent::saveComponentSourcesToFile(char const * const compName, NameValueFile &compFile) const
    {
    CompoundValue objArgs;
    for(const auto &src : mSourceFiles)
	{
	objArgs.addArg(src.c_str());
	}
    std::string tag = ComponentTypesFile::getCompTagName(compName, "src");
    std::string val = compFile.getValue(tag.c_str());
    compFile.setNameValue(tag.c_str(), objArgs.getAsString().c_str());

    CompoundValue incArgs;
    for(const auto &src : mIncludeFiles)
	{
	incArgs.addArg(src.c_str());
	}
    tag = ComponentTypesFile::getCompTagName(compName, "inc");
    val = compFile.getValue(tag.c_str());
    compFile.setNameValue(tag.c_str(), incArgs.getAsString().c_str());
    }

//////////////

void ScannedComponentsInfo::addSourceFile(char const * const compName, char const * const srcFileName)
    {
    auto it = mComponents.find(compName);
    if(it == mComponents.end())
	{
	it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
	}
    (*it).second.addSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addIncludeFile(char const * const compName, char const * const srcFileName)
    {
    auto it = mComponents.find(compName);
    if(it == mComponents.end())
	{
	it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
	}
    (*it).second.addIncludeFileName(srcFileName);
    }

static void setFileValues(char const * const tagName, const std::vector<std::string> &vals,
	NameValueFile &compFile)
    {
    CompoundValue incArgs;
    for(const auto &inc : vals)
	{
	incArgs.addArg(inc.c_str());
	}
    compFile.setNameValue(tagName, incArgs.getAsString().c_str());
    }

void ScannedComponentsInfo::setProjectComponentsFileValues(NameValueFile &compFile)
    {
    setFileValues("Components-init-proj-incs", mProjectIncludeDirs, compFile);
    }

void ScannedComponentsInfo::initializeComponentTypesFileValues(NameValueFile &compFile)
    {
    CompoundValue comps;
    for(const auto &comp : mComponents)
	{
	comps.addArg(comp.first.c_str());
	comp.second.saveComponentSourcesToFile(comp.first.c_str(), compFile);
	}
    compFile.setNameValue("Components", comps.getAsString().c_str());
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
    recurseDirs(mProject.getSrcRootDirectory().c_str());
    }

void ComponentFinder::scanExternalProject(char const * const externalRootSrch,
	Package const *pkg)
    {
    std::string externalRootDir;
    ComponentsFile::parseProjRefs(externalRootSrch, externalRootDir, mExcludes);

    Package rootPkg;
    if(pkg)
	rootPkg = *pkg;
    rootPkg.setRootDirPackage(externalRootSrch);
    mScanningPackage = &rootPkg;

    recurseDirs(externalRootDir.c_str());

    getProject().getBuildPackages().insertPackage(rootPkg);
    getProject().getBuildPackages().savePackages();
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
    Project::getSrcRootDirRelativeSrcFileName(path.getWithoutEndPathSep(),
	    mProject.getSrcRootDirectory().c_str(), compName);
    if(compName.length() == 0)
    	{
	path.moveLeftPathSep();
	mRootPathName = path.getPathSegment();
	compName = "<Root>";
    	}
    return compName;
    }

bool ComponentFinder::processFile(const std::string &filePath)
    {
    /// @todo - find files with no extension? to match things like std::vector include
    bool inc = isHeader(filePath.c_str());
    bool src = isSource(filePath.c_str());
    bool lib = isLibrary(filePath.c_str());
    if(mScanningPackage)
	{
	if(!ComponentsFile::excludesMatch(filePath, mExcludes))
	    {
	    if(inc)
		{
		FilePath path(filePath, FP_File);
		path.discardFilename();

		// Insert parent for things like #include "gtk/gtk.h" or "sys/stat.h"
		FilePath parent(path);
		parent.moveToEndDir();
		parent.moveLeftPathSep();
		parent.discardTail();
		mScanningPackage->appendAbsoluteIncDir(parent.getDrivePath().c_str());
		mScanningPackage->appendAbsoluteIncDir(path.getDrivePath().c_str());
		}
	    else if(lib)
		{
		FilePath path(filePath, FP_File);
		mScanningPackage->appendAbsoluteLibName(filePath.c_str());
		}
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

