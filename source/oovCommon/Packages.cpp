/*
 * Packages.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Packages.h"
#include "Project.h"
#include <algorithm>

#define TagPkgNames "PkgNames"
#define TagPkgRootDirSuffix "Root"
#define TagPkgIncDirSuffix "I"
#define TagPkgCppArgsSuffix "Cpp"
#define TagPkgLibDirSuffix "L"
#define TagPkgLibNamesSuffix "l"
#define TagPkgLinkArgsSuffix "Lnk"
#define TagPkgExtRefSuffix "ER"
#define TagPkgScannedLibPathsSuffix "ScannedLib"


static std::string makeTagName(char const * const pkgName, char const * const suffix)
    {
    std::string tag = "Pkg-";
    tag += pkgName;
    tag += "-";
    tag += suffix;
    return tag;
    }

static void setTagVal(NameValueFile &valFile, char const * const name,
	char const * const suffix, char const * const val)
    {
    std::string tag = makeTagName(name, suffix);
    valFile.setNameValue(tag.c_str(), val);
    }

static std::string getTagVal(NameValueFile const &valFile, char const * const name,
	char const * const suffix)
    {
    std::string tag = makeTagName(name, suffix);
    return valFile.getValue(tag.c_str());
    }

static std::string makeRelative(std::string const &rootDir, char const * const absPath,
	eFilePathTypes fpt)
    {
    FilePath fp(absPath, fpt);
    if(fp.compare(0, rootDir.length(), rootDir) == 0)
	{
	fp.erase(0, rootDir.length());
	}
    return fp;
    }

static void appendStr(std::string &val, char const * const str)
    {
    std::vector<std::string> vec = CompoundValueRef::parseString(val.c_str());
    if(vec.size() != 0)
	{
	if(std::find(vec.begin(), vec.end(), str) == vec.end())
	    {
	    val += ';';
	    val += str;
	    }
	}
    else
	val += str;
    }

static std::string getPackageNameFromDir(char const * const path)
    {
    FilePath clumpDir(path, FP_Dir);
    clumpDir.moveToEndDir();
    clumpDir.moveLeftPathSep();
    std::string part = clumpDir.getPathSegment();
    std::string clumpName;

    if(part.compare("lib") == 0)
	{
	clumpDir.moveLeftPathSep();
	clumpName = clumpDir.getPathSegment();
	}
    else
	clumpName = part.c_str();
    return clumpName;
    }


/////////////

void RootDirPackage::setRootDirPackage(char const *rootDir)
    {
    if(mName.length() == 0)
	mName = getPackageNameFromDir(rootDir);
    mRootDir.setPath(rootDir, FP_Dir);
    }

bool RootDirPackage::addUndefinedPackage(char const * const pkgName, NameValueFile &file) const
    {
    CompoundValue pkgNames;
    pkgNames.parseString(file.getValue(TagPkgNames).c_str());
    bool add = std::find(pkgNames.begin(), pkgNames.end(), pkgName) ==
	    pkgNames.end();
    if(add)
	{
	pkgNames.push_back(pkgName);
	file.setNameValue(TagPkgNames, pkgNames.getAsString().c_str());
	}
    return add;
    }

void RootDirPackage::loadFromMap(char const * const name, NameValueFile const &file)
    {
    mName = name;

    mIncludeDir = getTagVal(file, name, TagPkgIncDirSuffix);
    mLibDir = getTagVal(file, name, TagPkgLibDirSuffix);
    mLibNames = getTagVal(file, name, TagPkgLibNamesSuffix);
    mExternalReferenceDir = getTagVal(file, name, TagPkgExtRefSuffix);
    mRootDir.setPath(getTagVal(file, name, TagPkgRootDirSuffix).c_str(), FP_Dir);
    mScannedLibFilePaths = getTagVal(file, name, TagPkgScannedLibPathsSuffix);;
    }

void RootDirPackage::saveToMap(NameValueFile &file) const
    {
    addUndefinedPackage(mName.c_str(), file);

    // Set the data even if the package already exists.
    setTagVal(file, mName.c_str(), TagPkgIncDirSuffix, mIncludeDir.c_str());
    setTagVal(file, mName.c_str(), TagPkgLibDirSuffix, mLibDir.c_str());
    setTagVal(file, mName.c_str(), TagPkgLibNamesSuffix, mLibNames.c_str());
    setTagVal(file, mName.c_str(), TagPkgExtRefSuffix, mExternalReferenceDir.c_str());
    setTagVal(file, mName.c_str(), TagPkgRootDirSuffix, mRootDir.c_str());
    setTagVal(file, mName.c_str(), TagPkgScannedLibPathsSuffix, mScannedLibFilePaths.c_str());
    }

void RootDirPackage::appendAbsoluteIncDir(char const * const absDir)
    {
    std::string dir = makeRelative(mRootDir, absDir, FP_Dir);
    appendStr(mIncludeDir, dir.c_str());
    }

void RootDirPackage::appendAbsoluteLibName(char const * const fn)
    {
    std::string fp = makeRelative(mRootDir, fn, FP_File);
    appendStr(mScannedLibFilePaths, fp.c_str());
    }

void RootDirPackage::setOrderedLibs(std::vector<std::string> const &libDirs,
	std::vector<std::string> const &libNames)
    {
    mScannedLibFilePaths.clear();
    mLibDir.clear();
    for(auto const &dir : libDirs)
	{
	std::string fp = makeRelative(mRootDir, dir.c_str(), FP_Dir);
	appendStr(mLibDir, fp.c_str());
	}
    mLibNames = CompoundValueRef::getAsString(libNames);
    }

std::vector<std::string> RootDirPackage::getValAddRootToVector(std::string const &val,
	eFilePathTypes fpt) const
    {
    std::vector<std::string> items;
    CompoundValue compVal;
    compVal.parseString(val.c_str());
    for(auto const &item : compVal)
	{
	FilePath fp(mRootDir, FP_Dir);
	fp.appendPart(item.c_str(), fpt);
	FilePath::normalizePathSeps(fp);
	items.push_back(fp.c_str());
	}
    return items;
    }

std::vector<std::string> RootDirPackage::getIncludeDirs() const
    {
    return(getValAddRootToVector(mIncludeDir, FP_Dir));
    }

std::vector<std::string> RootDirPackage::getLibraryDirs() const
    {
    return(getValAddRootToVector(mLibDir, FP_Dir));
    }

std::vector<std::string> RootDirPackage::getExtRefDirs() const
    {
    std::string extDir = mExternalReferenceDir;
    if(extDir.length() == 0 && needDirScan())
	{
	extDir = "./";
	}
    return(getValAddRootToVector(extDir, FP_Dir));
    }

std::vector<std::string> RootDirPackage::getLibraryNames() const
    {
    return CompoundValueRef::parseString(mLibNames.c_str());
    }

std::vector<std::string> RootDirPackage::getScannedLibraryFilePaths() const
    {
    return(getValAddRootToVector(mScannedLibFilePaths, FP_File));
    }

/*
bool RootDirPackage::anyIncDirsMatch(std::set<std::string> const &incDirs) const
    {
    bool match = false;

    std::vector<std::string> incRoots = getIncludeDirs();
    for(size_t ir=0; ir<incRoots.size() && !match; ir++)
	{
	for(auto const &incDir : incDirs)
	    {
	    if(incRoots[ir].compare(0, incRoots[ir].length(), incDir) == 0)
		{
		match = true;
		break;
		}
	    }
	}
    return match;
    }
*/

///////////////

void Package::loadFromMap(char const * const name, NameValueFile const &file)
    {
    RootDirPackage::loadFromMap(name, file);
    std::string tag = makeTagName(name, TagPkgCppArgsSuffix);
    mCompileArgs = file.getValue(tag.c_str());

    tag = makeTagName(name, TagPkgLinkArgsSuffix);
    mLinkArgs = file.getValue(tag.c_str());
    }

void Package::saveToMap(NameValueFile &file) const
    {
    RootDirPackage::saveToMap(file);
    std::string tag = makeTagName(getPkgName().c_str(), TagPkgCppArgsSuffix);
    file.setNameValue(tag.c_str(), mCompileArgs.c_str());

    tag = makeTagName(getPkgName().c_str(), TagPkgLinkArgsSuffix);
    file.setNameValue(tag.c_str(), mLinkArgs.c_str());
    }

std::vector<std::string> Package::getCompileArgs() const
    {
    return CompoundValueRef::parseString(mCompileArgs.c_str());
    }

std::vector<std::string> Package::getLinkArgs() const
    {
    return CompoundValueRef::parseString(mLinkArgs.c_str());
    }

////////////////

Package Packages::getPackage(char const * const name) const
    {
    Package pkg;
    pkg.loadFromMap(name, mFile);
    return pkg;
    }

void Packages::removePackage(char const * const pkgName)
    {
    CompoundValue pkgNames;
    pkgNames.parseString(mFile.getValue(TagPkgNames).c_str());

    auto const &pos = std::find(pkgNames.begin(), pkgNames.end(), pkgName);
    if(pos != pkgNames.end())
	{
	pkgNames.erase(pos);
	mFile.setNameValue(TagPkgNames, pkgNames.getAsString().c_str());
	}
    /// @todo - this doesn't remove other junk related to the package.
    }

std::vector<Package> Packages::getPackages() const
    {
    CompoundValue pkgNames;
    pkgNames.parseString(mFile.getValue(TagPkgNames).c_str());
    std::vector<Package> packages;
    for(std::string const &name : pkgNames)
	{
	packages.push_back(getPackage(name.c_str()));
	}
    return packages;
    }

#ifndef __linux__
void Packages::read(char const * const fn)
    {
    mFile.setFilename("oovcde-allpkgs-win.txt");
    mFile.readFile();
    }
#endif

////////////////


ProjectPackages::ProjectPackages(bool readNow)
    {
    if(readNow)
	read();
    }

bool ProjectPackages::read()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovcde-pkg.txt");
    mFile.setFilename(fn.c_str());
    return mFile.readFile();
    }

////////////////

BuildPackages::BuildPackages(bool readNow)
    {
    if(readNow)
	read();
    }

bool BuildPackages::read()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovcde-buildpkg.txt");
    mFile.setFilename(fn.c_str());
    return mFile.readFile();
    }

Package BuildPackages::getPackage(char const * const name) const
    {
    Package pkg;
    pkg.loadFromMap(name, mFile);
    return pkg;
    }

std::vector<Package> BuildPackages::getPackages() const
    {
    CompoundValue pkgNames;
    pkgNames.parseString(mFile.getValue(TagPkgNames).c_str());
    std::vector<Package> packages;
    for(std::string const &name : pkgNames)
	{
	packages.push_back(getPackage(name.c_str()));
	}
    return packages;
    }


///////////////

AvailablePackages::AvailablePackages()
    {
#ifndef __linux__
    mPackages.read("oovcde-allpkgs-win.txt");
#endif
    }

#ifndef __linux__
std::vector<std::string> AvailablePackages::getAvailablePackages()
    {
    std::vector<std::string> strs;
    for(auto const &pkg : mPackages.getPackages())
	strs.push_back(pkg.getPkgName().c_str());
    return strs;
    }

Package AvailablePackages::getPackage(char const * const name) const
    {
    return mPackages.getPackage(name);
    }

#endif
