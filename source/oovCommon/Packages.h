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


/// Packages can be in different states.  This mainly applies to Windows.
/// 1. Some packages may have include dirs defined as '*'. This means search
///     for include files below the root dir.  Once scanning is performed,
///     if include files are found, the include paths are filled into the
///     include dirs.
///
/// 2. Some packages may be defined and contain library names that are in link
///     dependency order.  They are in GNU linker format, so may or may not
///     have extensions.
///
/// 3. Some packages may have library names defined as '*'.  These packages are
///     processed by the scanner/finder, and the root dir (and lib dir?)
///      is scanned for libraries which are put into the scanned libs paths
///     with full path names (when combined with the root dir and lib dir), but are
///     not ordered.  When the builder orders them, the lib dir and names
///     are updated, and the lib names are ordered and do not have paths,
///     but they do have extensions.
///
/// 4. When the scanning is complete for a package, the '*' values are
///     cleared from the package.
class RootDirPackage
    {
    public:
        RootDirPackage()
            {}
        RootDirPackage(OovStringRef const pkgName, OovStringRef const rootDir):
                mName(pkgName), mRootDir(rootDir, FP_Dir)
            {}

        bool addUndefinedPackage(OovString const &pkgName,
                NameValueFile &file) const;

        /// This will save relative to mRootDir
        void appendAbsoluteIncDir(OovStringRef const dir);
        /// This will save relative to mRootDir
        void appendAbsoluteLibName(OovStringRef const fn);

        /// This will fill lib names and lib dirs
        void setOrderedLibs(OovStringVec const &libDirs,
                OovStringVec const &libNames);


        void setRootDirPackage(OovStringRef const rootDir);
        void setRootDir(OovStringRef const rootDir)
            { mRootDir.setPath(rootDir, FP_Dir); }

        OovString const &getPkgName() const
            { return mName; }
        FilePath const &getRootDir() const
            { return mRootDir; }
        OovStringVec getExtRefDirs() const;
        OovStringVec getLibraryNames() const;
        OovStringVec getScannedLibraryFilePaths() const;
        OovStringVec getIncludeDirs() const;
        /// Returns paths made from the mRootDir and potentially multiple mLibDir entries.
        OovStringVec getLibraryDirs() const;
        OovString const &getIncludeDirsAsString() const
            { return mIncludeDirs; }
        OovString const &getLibraryDirsAsString() const
            { return mLibDirs; }
        OovString const &getLibraryNamesAsString() const
            { return mLibNames; }

        bool needIncs() const
            { return(mIncludeDirs == "*"); }
        bool needLibs() const
            { return(mLibNames == "*"); }
        bool needDirScan() const
            { return(needLibs() || needIncs());}
        void clearDirScan();
        // Libraries are either listed in order in the package, or they
        // are scanned using a directory search.
        bool areLibraryNamesOrdered() const
            { return(mScannedLibFilePaths.size() == 0); }
        void loadFromMap(OovStringRef const pkgName, NameValueFile const &file);
        /// Call RootDirPackages::savePackages to save.
        void saveToMap(NameValueFile &file) const;


    protected:
        OovString mName;
        FilePath mRootDir;

        OovString mLibDirs;              // Relative to root
        OovString mLibNames;

        OovString mIncludeDirs;          // Relative to root

        OovString mScannedLibFilePaths;

        OovStringVec getValAddRootToVector(OovStringRef const tagName,
                eFilePathTypes fpt) const;
    };

/// This contains information defined either by the pkg-config on linux, or
/// defined by some user on Windows.
class Package:public RootDirPackage
    {
    public:
        Package()
            {}
        Package(OovStringRef const pkgName, OovStringRef const rootDir):
            RootDirPackage(pkgName, rootDir)
            {}
        virtual ~Package()
            {}
        void setCompileInfo(OovStringRef const incDir, OovStringRef const compileArgs)
            {
            mIncludeDirs = incDir;
            mCompileArgs = compileArgs;
            }
        void setLinkInfo(OovStringRef const libDir, OovStringRef const libNames,
                OovStringRef const linkArgs)
            {
            mLibDirs = libDir;
            mLibNames = libNames;
            mLinkArgs = linkArgs;
            }
        virtual OovStringVec getCompileArgs() const;
        virtual OovStringVec getLinkArgs() const;
        OovStringRef const getCompileArgsAsStr() const
            { return mCompileArgs; }
        OovStringRef const getLinkArgsAsStr() const
            { return mLinkArgs; }
        void loadFromMap(OovStringRef const name, NameValueFile const &file);
        void saveToMap(NameValueFile &file) const;

    private:
        OovString mCompileArgs;
        OovString mLinkArgs;
    };

class BuildPackages
    {
    public:
        BuildPackages(bool readNow);
        bool read();
        Package getPackage(OovStringRef const name) const;
        std::vector<Package> getPackages() const;
        bool doesPackageExist(OovStringRef pkgName);
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
        Package getPackage(OovStringRef const name) const;
        void insertPackage(Package const &pkg)
            { pkg.saveToMap(mFile); }
        void removePackage(OovString const &name);
        std::vector<Package> getPackages() const;
#ifndef __linux__
        void read(OovStringRef const fn);
#endif

    protected:
        NameValueFile mFile;

    private:
        bool addUndefinedPackage(OovStringRef const name);
    };

/// This is information about all packages available on the system.
/// On Linux, this uses pkg-config to discover the system packages. On Windows,
/// a package file is read.
class AvailablePackages
    {
    public:
        AvailablePackages();
#ifdef __linux__
        CompoundValue mPackageNames;
#else
        Packages mPackages;
#endif
        OovStringVec getAvailablePackages();
        Package getPackage(OovStringRef const name) const;
    };

/// These are packages that are referred to by the project.
class ProjectPackages:public Packages
    {
    public:
        ProjectPackages(bool readNow);
        bool read();
        void savePackages()
            { mFile.writeFile(); }
        static OovString getFilename();
    };

#endif /* PACKAGES_H_ */
