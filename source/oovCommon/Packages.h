/*
 * Packages.h
 *
 *  Created on: Jan 20, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef PACKAGES_H_
#define PACKAGES_H_

#include "NameValueFile.h"
#include "FilePath.h"
#include <set>


/// This class basically represents two types of packages.
/// 1. Some packages may be defined and contain library names that are in link
///	dependency order.  They are in GNU linker format, so may or may not
///	have extensions.
///
/// 2. Some packages may have no library names.  These packages are processed
///	by the builder, and the external ref root dir is scanned for libraries
///	which are put into the lib name field with full path names (when
///	combined with the root dir and lib dir), but are
///	not ordered.  When the builder orders them, the lib dir and names
///	are updated, and the lib names are ordered and do not have paths,
///	but they do have extensions.
class RootDirPackage
    {
    public:
	RootDirPackage()
	    {}
	RootDirPackage(char const * const pkgName, char const * const rootDir):
		mName(pkgName), mRootDir(rootDir, FP_Dir)
	    {}

	bool addUndefinedPackage(char const * const pkgName, NameValueFile &file) const;

	/// This will save relative to mRootDir
	void appendAbsoluteIncDir(char const * const dir);
	/// This will save relative to mRootDir
	void appendAbsoluteLibName(char const * const fn);

	/// This will fill lib names and lib dirs
	void setOrderedLibs(std::vector<std::string> const &libDirs,
		std::vector<std::string> const &libNames);


	void setRootDirPackage(char const *rootDir);
	void setRootDir(char const *rootDir)
	    { mRootDir.setPath(rootDir, FP_Dir); }
	void setExternalReferenceDir(std::string const &extRefDir)
	    { mExternalReferenceDir = extRefDir; }

	std::string getPkgName() const
	    { return mName; }
	std::string getRootDir() const
	    { return mRootDir; }
	std::vector<std::string> getExtRefDirs() const;
	std::vector<std::string> getLibraryNames() const;
	std::vector<std::string> getScannedLibraryFilePaths() const;
	std::vector<std::string> getIncludeDirs() const;
	/// Returns paths made from the mRootDir and potentially multiple mLibDir entries.
	std::vector<std::string> getLibraryDirs() const;
	std::string const &getIncludeDirsAsString() const
	    { return mIncludeDir; }
	std::string const &getLibraryDirsAsString() const
	    { return mLibDir; }
	std::string const &getLibraryNamesAsString() const
	    { return mLibNames; }
	std::string const &getExtRefDirsAsString() const
	    { return mExternalReferenceDir; }

//	bool anyIncDirsMatch(std::set<std::string> const &includes) const;
	bool needDirScan() const
	    { return(mLibNames.size() == 0);}
	bool areLibraryNamesOrdered() const
	    { return(mScannedLibFilePaths.size() == 0); }
	void loadFromMap(char const * const pkgName, NameValueFile const &file);
	/// Call RootDirPackages::savePackages to save.
	void saveToMap(NameValueFile &file) const;


    protected:
	std::string mName;
	FilePath mRootDir;

	std::string mLibDir;			// Relative to root
	std::string mLibNames;

	std::string mIncludeDir;		// Relative to root
	std::string mExternalReferenceDir;	// Relative to root. "./" means same as root

	std::string mScannedLibFilePaths;

	std::vector<std::string> getValAddRootToVector(std::string const &tagName,
		eFilePathTypes fpt) const;
    };

/// This contains information defined either by the pkg-config on linux, or
/// defined by some user on Windows.
class Package:public RootDirPackage
    {
    public:
	Package()
	    {}
	Package(char const * const pkgName, char const * const rootDir):
	    RootDirPackage(pkgName, rootDir)
	    {}
	virtual ~Package()
	    {}
	void setCompileInfo(std::string const &incDir, std::string const &compileArgs)
	    {
	    mIncludeDir = incDir;
	    mCompileArgs = compileArgs;
	    }
	void setLinkInfo(std::string const &libDir, std::string const &libNames,
		std::string const &linkArgs)
	    {
	    mLibDir = libDir;
	    mLibNames = libNames;
	    mLinkArgs = linkArgs;
	    }
	virtual std::vector<std::string> getCompileArgs() const;
	virtual std::vector<std::string> getLinkArgs() const;
	std::string const &getCompileArgsAsStr() const
	    { return mCompileArgs; }
	std::string const &getLinkArgsAsStr() const
	    { return mLinkArgs; }
	void loadFromMap(char const * const name, NameValueFile const &file);
	void saveToMap(NameValueFile &file) const;

    private:
	std::string mCompileArgs;
	std::string mLinkArgs;
    };

class BuildPackages
    {
    public:
	BuildPackages(bool readNow);
	bool read();
	Package getPackage(char const * const name) const;
	std::vector<Package> getPackages() const;
	void insertPackage(Package const &pkg)
	    { pkg.saveToMap(mFile); }
	void savePackages()
	    { mFile.writeFile(); }

    protected:
	NameValueFile mFile;
    };

class Packages
    {
    public:
	Package getPackage(char const * const name) const;
	void insertPackage(Package const &pkg)
	    { pkg.saveToMap(mFile); }
	void removePackage(char const * const name);
	std::vector<Package> getPackages() const;
#ifndef __linux__
	void read(char const * const fn);
#endif

    protected:
	NameValueFile mFile;

    private:
	bool addUndefinedPackage(char const * const name);
    };

/// This is information about all packages available on the system.
class AvailablePackages
    {
    public:
	AvailablePackages();
#ifdef __linux__
	CompoundValue mPackageNames;
#else
	Packages mPackages;
#endif
	std::vector<std::string> getAvailablePackages();
	Package getPackage(char const * const name) const;
    };

/// These are packages that are referred to by the project.
class ProjectPackages:public Packages
    {
    public:
	ProjectPackages(bool readNow);
	bool read();
	void savePackages()
	    { mFile.writeFile(); }
    };

#endif /* PACKAGES_H_ */
