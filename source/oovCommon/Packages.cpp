/*
 * Packages.cpp
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Packages.h"
#include "Project.h"
#include "OovError.h"
#include "DirList.h"
#include <algorithm>

#define TagPkgNames "PkgNames"
#define TagPkgRootDirSuffix "Root"
#define TagPkgIncDirSuffix "I"
#define TagPkgCppArgsSuffix "Cpp"
#define TagPkgLibDirSuffix "L"
#define TagPkgLibNamesSuffix "l"
#define TagPkgLinkArgsSuffix "Lnk"
#define TagPkgScannedLibPathsSuffix "ScannedLib"


static OovString makeTagName(OovStringRef const pkgName, OovStringRef const suffix)
    {
    OovString tag = "Pkg-";
    tag += pkgName;
    tag += "-";
    tag += suffix;
    return tag;
    }

static void setTagVal(NameValueFile &valFile, OovStringRef const name,
        OovStringRef const suffix, OovStringRef const val)
    {
    std::string tag = makeTagName(name, suffix);
    valFile.setNameValue(tag, val);
    }

static OovString getTagVal(NameValueFile const &valFile, OovStringRef const name,
        OovStringRef const suffix)
    {
    OovString tag = makeTagName(name, suffix);
    return valFile.getValue(tag);
    }

static std::string makeRelative(std::string const &rootDir, OovStringRef const absPath,
        eFilePathTypes fpt)
    {
    FilePath fp(absPath, fpt);

    // Remove the exclusion directories.
    std::string baseRootDir = rootDir;
    size_t pos = baseRootDir.find('!');
    if(pos != std::string::npos)
        {
        baseRootDir.erase(pos);
        }
    // Remove any relative directory info.
    pos = baseRootDir.find('.');
    if(pos != std::string::npos)
        {
        baseRootDir.erase(pos);
        }

    // Don't make relative directories higher than root.
    if(rootDir.length() > fp.length())
        {
        fp.clear();
        }
    else if(fp.compare(0, baseRootDir.length(), baseRootDir) == 0)
        {
        fp.erase(0, rootDir.length());
        }
    return fp;
    }

static void appendStr(std::string &baseStr, OovString const &str)
    {
    if(str.length() > 0)
        {
        CompoundValue val(baseStr);
        if(std::find(val.begin(), val.end(), str) == val.end())
            {
            val.addArg(str);
            }
        baseStr = val.getAsString();
        }
    }

static OovString getPackageNameFromDir(OovStringRef const path)
    {
    FilePath clumpDir(path, FP_Dir);
    size_t pos = clumpDir.getPosLeftPathSep(clumpDir.getPosEndDir(),
            RP_RetPosNatural);
    std::string part = clumpDir.getPathSegment(pos);
    OovString clumpName;

    if(part.compare("lib") == 0)
        {
        pos = clumpDir.getPosLeftPathSep(pos, RP_RetPosNatural);
        clumpName = clumpDir.getPathSegment(pos);
        }
    else
        clumpName = part;
    return clumpName;
    }


/////////////

void RootDirPackage::setRootDirPackage(OovStringRef const rootDir)
    {
    if(mName.length() == 0)
        {
        mName = getPackageNameFromDir(rootDir);
        }
    mRootDir.setPath(rootDir, FP_Dir);
    }

bool RootDirPackage::addUndefinedPackage(OovString const &pkgName, NameValueFile &file) const
    {
    CompoundValue pkgNames;
    pkgNames.parseString(file.getValue(TagPkgNames));
    bool add = std::find(pkgNames.begin(), pkgNames.end(), pkgName) ==
            pkgNames.end();
    if(add)
        {
        pkgNames.push_back(pkgName);
        file.setNameValue(TagPkgNames, pkgNames.getAsString());
        }
    return add;
    }

void RootDirPackage::clearDirScan()
    {
    if(needIncs())
        {
        mIncludeDirs.clear();
        }
    if(needLibs())
        {
        mLibNames.clear();
        }
    }

void RootDirPackage::loadFromMap(OovStringRef const name, NameValueFile const &file)
    {
    mName = name;

    mIncludeDirs = getTagVal(file, name, TagPkgIncDirSuffix);
    mLibDirs = getTagVal(file, name, TagPkgLibDirSuffix);
    mLibNames = getTagVal(file, name, TagPkgLibNamesSuffix);
    mRootDir.setPath(getTagVal(file, name, TagPkgRootDirSuffix), FP_Dir);
    mScannedLibFilePaths = getTagVal(file, name, TagPkgScannedLibPathsSuffix);;
    }

void RootDirPackage::saveToMap(NameValueFile &file) const
    {
    addUndefinedPackage(mName, file);

    // Set the data even if the package already exists.
    setTagVal(file, mName, TagPkgIncDirSuffix, mIncludeDirs);
    setTagVal(file, mName, TagPkgLibDirSuffix, mLibDirs);
    setTagVal(file, mName, TagPkgLibNamesSuffix, mLibNames);
    setTagVal(file, mName, TagPkgRootDirSuffix, mRootDir);
    setTagVal(file, mName, TagPkgScannedLibPathsSuffix, mScannedLibFilePaths);
    }

void RootDirPackage::appendAbsoluteIncDir(OovStringRef const absDir)
    {
    std::string dir = makeRelative(mRootDir, absDir, FP_Dir);
    appendStr(mIncludeDirs, dir);
    }

void RootDirPackage::appendAbsoluteLibName(OovStringRef const fn)
    {
    std::string fp = makeRelative(mRootDir, fn, FP_File);
    appendStr(mScannedLibFilePaths, fp);
    }

void RootDirPackage::setOrderedLibs(OovStringVec const &libDirs,
        OovStringVec const &libNames)
    {
    mScannedLibFilePaths.clear();
    mLibDirs.clear();
    for(auto const &dir : libDirs)
        {
        std::string fp = makeRelative(mRootDir, dir, FP_Dir);
        appendStr(mLibDirs, fp);
        }
    mLibNames = CompoundValueRef::getAsString(libNames);
    }

OovStringVec RootDirPackage::getValAddRootToVector(OovStringRef const val,
        eFilePathTypes fpt) const
    {
    OovStringVec items;
    CompoundValue compVal;
    compVal.parseString(val);
    for(auto const &item : compVal)
        {
        FilePath fp(mRootDir, FP_Dir);
        fp.appendPart(item, fpt);
        FilePath::normalizePathSeps(fp);
        items.push_back(fp);
        }
    return items;
    }

OovStringVec RootDirPackage::getIncludeDirs() const
    {
    return(getValAddRootToVector(mIncludeDirs, FP_Dir));
    }

OovStringVec RootDirPackage::getLibraryDirs() const
    {
    return(getValAddRootToVector(mLibDirs, FP_Dir));
    }

OovStringVec RootDirPackage::getLibraryNames() const
    {
    return CompoundValueRef::parseString(mLibNames);
    }

OovStringVec RootDirPackage::getScannedLibraryFilePaths() const
    {
    return(getValAddRootToVector(mScannedLibFilePaths, FP_File));
    }

#ifndef __linux__
// Warning - this may alter paths (sort them)
static OovString findBestPath(FilePaths &paths)
    {
    OovString bestPath;
    if(paths.size() > 0)
        {
        // Sorting will allow matching the highest version of
        // filepaths that have a similar format.
        std::sort(paths.begin(), paths.end());
        bestPath = paths[paths.size()-1];
        }
    return bestPath;
    }

bool RootDirPackage::winScanAndSetRootDir(OovStringRef rootDir)
    {
    FilePaths dirs;
    dirs.push_back(FilePath("/", FP_Dir));
    dirs.push_back(FilePath("/Program Files", FP_Dir));
    FilePaths paths = findMatchingDirs(dirs, rootDir);
    OovString dir = findBestPath(paths);
    if(dir.length())
        {
        setRootDir(dir);
        }
    return(dir.length());
    }
#endif

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

void Package::loadFromMap(OovStringRef const name, NameValueFile const &file)
    {
    RootDirPackage::loadFromMap(name, file);
    std::string tag = makeTagName(name, TagPkgCppArgsSuffix);
    mCompileArgs = file.getValue(tag);

    tag = makeTagName(name, TagPkgLinkArgsSuffix);
    mLinkArgs = file.getValue(tag);
    }

void Package::saveToMap(NameValueFile &file) const
    {
    RootDirPackage::saveToMap(file);
    std::string tag = makeTagName(getPkgName(), TagPkgCppArgsSuffix);
    file.setNameValue(tag, mCompileArgs);

    tag = makeTagName(getPkgName(), TagPkgLinkArgsSuffix);
    file.setNameValue(tag, mLinkArgs);
    }

OovStringVec Package::getCompileArgs() const
    {
    return CompoundValueRef::parseString(mCompileArgs);
    }

OovStringVec Package::getLinkArgs() const
    {
    return CompoundValueRef::parseString(mLinkArgs);
    }

static bool checkDirs(OovStringVec const &dirs, OovString &badPath)
    {
    OovStatus status(true, SC_File);
    bool ok = true;
    for(auto const &path : dirs)
        {
        if(path.find('*') == std::string::npos)
            {
            FilePath fp(path, FP_Dir);
            ok = FileIsDirOnDisk(fp, status);
            if(!ok)
                {
                badPath = fp;
                break;
                }
            }
        }
    if(status.needReport())
        {
        status.reported();
        }
    return ok;
    }

bool Package::checkDirectories(OovString &badPath) const
    {
    OovStatus status(true, SC_File);
    bool ok = FileIsDirOnDisk(getRootDir(), status);
    if(!ok)
        {
        badPath = getRootDir();
        }
    if(ok)
        {
        ok = checkDirs(getIncludeDirs(), badPath);
        }
    if(ok)
        {
        ok = checkDirs(getLibraryDirs(), badPath);
        }
    if(status.needReport())
        {
        status.reported();
        }
    return ok;
    }

////////////////

Package Packages::getPackage(OovStringRef const name) const
    {
    Package pkg;
    pkg.loadFromMap(name, mFile);
    return pkg;
    }

void Packages::removePackage(OovString const &pkgName)
    {
    CompoundValue pkgNames;
    pkgNames.parseString(mFile.getValue(TagPkgNames));

    auto const &pos = std::find(pkgNames.begin(), pkgNames.end(), pkgName);
    if(pos != pkgNames.end())
        {
        pkgNames.erase(pos);
        mFile.setNameValue(TagPkgNames, pkgNames.getAsString());
        }
    /// @todo - this doesn't remove other junk related to the package.
    }

std::vector<Package> Packages::getPackages() const
    {
    CompoundValue pkgNames;
    pkgNames.parseString(mFile.getValue(TagPkgNames));
    std::vector<Package> packages;
    for(std::string const &name : pkgNames)
        {
        packages.push_back(getPackage(name));
        }
    return packages;
    }

#ifndef __linux__
OovStatusReturn Packages::read(OovStringRef const fn)
    {
    mFile.setFilename(fn);
    OovStatus status = mFile.readFile();
    if(status.needReport())
        {
        OovString str = "Unable to read build packages: ";
        str += fn;
        status.report(ET_Error, str);
        }
    return status;
    }
#endif

////////////////


ProjectPackages::ProjectPackages(bool readNow)
    {
    if(readNow)
        {
        OovStatus status = read();
        if(status.needReport())
            {
            // This package is optional.
            status.reported();
            }
        }
    }

OovString ProjectPackages::getFilename()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovaide-pkg.txt");
    return fn;
    }

OovStatusReturn ProjectPackages::read()
    {
    mFile.setFilename(getFilename());
    // It is ok if the project package file is not present.  There could
    // be no packages yet.
    OovStatus status(true, SC_File);
    if(mFile.isFilePresent(status))
        {
        status = mFile.readFile();
        if(status.needReport())
            {
            OovString str = "Unable to read project packages: ";
            str += getFilename();
            status.report(ET_Error, str);
            }
        }
    return status;
    }



////////////////

BuildPackages::BuildPackages(bool readNow)
    {
    if(readNow)
        {
        OovStatus status = read();
        if(status.needReport())
            {
            // This file is optional.
            status.reported();
            }
        }
    }

OovStatusReturn BuildPackages::read()
    {
    FilePath fn(Project::getBuildPackagesFilePath(), FP_File);
    mPackages.getFile().setFilename(fn);
    return mPackages.getFile().readFile();
    }

bool BuildPackages::doesPackageExist(OovStringRef pkgName)
    {
    std::vector<Package> const &packages = getPackages();
    auto iter = std::find_if(packages.begin(), packages.end(),
            [&pkgName](Package const &pkg)
                { return(pkg.getPkgName() == pkgName.getStr()); });
    return(iter != packages.end());
    }

OovStatusReturn BuildPackages::savePackages()
    {
    OovStatus status(mPackages.getFile().writeFile());
    if(status.needReport())
        {
        OovString str = "Unable to save build packages";
        status.report(ET_Error, str);
        }
    return status;
    }


///////////////

AvailablePackages::AvailablePackages()
    {
#ifndef __linux__
    OovStatus status = mPackages.read("oovaide-allpkgs-win.txt");
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read available packages");
        }
#endif
    }

#ifndef __linux__
OovStringVec AvailablePackages::getAvailablePackages()
    {
    OovStringVec strs;
    for(auto const &pkg : mPackages.getPackages())
        strs.push_back(pkg.getPkgName());
    return strs;
    }

Package AvailablePackages::getPackage(OovStringRef const name) const
    {
    return mPackages.getPackage(name);
    }

#endif
