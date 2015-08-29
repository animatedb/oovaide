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


/// This is the base class for packages.
///
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

        /// Append an include directory to the package.
        /// This will save relative to mRootDir
        void appendAbsoluteIncDir(OovStringRef const dir);

        /// Append a library to the package.
        /// This will save relative to mRootDir
        void appendAbsoluteLibName(OovStringRef const fn);

        /// Adds the library directories and library names to the package.
        void setOrderedLibs(OovStringVec const &libDirs,
                OovStringVec const &libNames);

        /// Set the path for the root directory of the package, and set the name
        /// of the package from the directory.  See getPackageNameFromDir() for
        /// how the name is created.
        void setRootDirPackage(OovStringRef const rootDir);

        /// The the path for the root directory of the package.
        void setRootDir(OovStringRef const rootDir)
            { mRootDir.setPath(rootDir, FP_Dir); }

        /// Get the package name.
        OovString const &getPkgName() const
            { return mName; }

        /// Get the root directory of the package.
        FilePath const &getRootDir() const
            { return mRootDir; }

        /// Get the library names of the package.
        OovStringVec getLibraryNames() const;

        /// Get the library paths that were found during scanning for the package.
        /// Returns the absolute path.
        OovStringVec getScannedLibraryFilePaths() const;

        /// The the include paths for the package.
        OovStringVec getIncludeDirs() const;

        /// Returns paths made from the mRootDir and potentially multiple mLibDir entries.
        OovStringVec getLibraryDirs() const;

        /// Get the include directories as a string with delimited directories.
        OovString const &getIncludeDirsAsString() const
            { return mIncludeDirs; }

        /// Get the library directories as a string with delimited directories.
        OovString const &getLibraryDirsAsString() const
            { return mLibDirs; }

        /// Get the include names as a string with delimited names.
        OovString const &getLibraryNamesAsString() const
            { return mLibNames; }

        /// Indicates whether the include directories are needed, and the
        /// directories must be scanned to get them.
        bool needIncs() const
            { return(mIncludeDirs == "*"); }

        /// Indicates whether the libary names are needed, and the
        /// directories must be scanned to get them.
        bool needLibs() const
            { return(mLibNames == "*"); }

        /// Indicates whether a directory scan is needed to get the libraries
        /// or include directories.
        bool needDirScan() const
            { return(needLibs() || needIncs());}

        /// Sets the fact that a directory scan is not needed for the project.
        void clearDirScan();

        /// Checks if the library names are ordered for the package.
        /// Libraries are either listed in order in the package, or they
        /// are scanned using a directory search.
        bool areLibraryNamesOrdered() const
            { return(mScannedLibFilePaths.size() == 0); }

        /// Load a package from a package file.
        /// @param pkgName The name of the package to load.
        /// @param file The package file to get the package from.
        void loadFromMap(OovStringRef const pkgName, NameValueFile const &file);

        /// Saves this package to the specified package file.
        /// Call RootDirPackages::savePackages to save.
        /// @param file The package file to save to.
        void saveToMap(NameValueFile &file) const;


    protected:
        /// This is delimited with the default CompoundValue delimiter.
        OovString mName;
        FilePath mRootDir;

        /// This is delimited with the default CompoundValue delimiter.
        /// This is relative to root
        /// CHANGE? - This may get updated by setOrderedLibs(), makeRelative(),
        /// since scanned directories are related to the root package directory.
        OovString mLibDirs;

        /// This is delimited with the default CompoundValue delimiter.
        OovString mLibNames;

        /// This is delimited with the default CompoundValue delimiter.
        OovString mIncludeDirs;          // Relative to root

        /// If this contains paths, then the library directories are not ordered.
        /// This is delimited with the default CompoundValue delimiter.
        /// These paths are relative to the root directory.
        OovString mScannedLibFilePaths;

        OovStringVec getValAddRootToVector(OovStringRef const tagName,
            eFilePathTypes fpt) const;

    private:
        /// Add the package name to the specified package file.
        /// @param pkgName The package name to add.
        /// @param file The file to add the package to.
        bool addUndefinedPackage(OovString const &pkgName,
            NameValueFile &file) const;
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

        /// Set the compile arguments.
        /// @param incDir The include directories separated by default
        ///     CompoundValue delimiter.
        /// @param compileArgs The compile arguments separated by default
        ///     CompoundValue delimiter.
        void setCompileInfo(OovStringRef const incDir, OovStringRef const compileArgs)
            {
            mIncludeDirs = incDir;
            mCompileArgs = compileArgs;
            }

        /// Set the link arguments.
        /// @param libDir The library directories separated by default
        ///     CompoundValue delimiter.
        /// @param libNames The lbrary names separated by default
        ///     CompoundValue delimiter.
        /// @param linkArgs The link arguments separated by default
        ///     CompoundValue delimiter.
        void setLinkInfo(OovStringRef const libDir, OovStringRef const libNames,
                OovStringRef const linkArgs)
            {
            mLibDirs = libDir;
            mLibNames = libNames;
            mLinkArgs = linkArgs;
            }

        /// Get the compile arguments.
        OovStringVec getCompileArgs() const;

        /// Get the link arguments.
        OovStringVec getLinkArgs() const;

        /// Get the compile arguments as a delimited string.
        OovStringRef const getCompileArgsAsStr() const
            { return mCompileArgs; }

        /// Get the link arguments as a delimited string.
        OovStringRef const getLinkArgsAsStr() const
            { return mLinkArgs; }

        void loadFromMap(OovStringRef const name, NameValueFile const &file);
        void saveToMap(NameValueFile &file) const;

    private:
        /// This is delimited with the default CompoundValue delimiter.
        OovString mCompileArgs;

        /// This is delimited with the default CompoundValue delimiter.
        OovString mLinkArgs;
    };


/// This is a generic packages file.
class Packages
    {
    public:
        /// Get a package by name.
        /// @param name The package to get.
        Package getPackage(OovStringRef const name) const;

        /// Insert a package into the file.
        /// @param pkg The package to add.
        void insertPackage(Package const &pkg)
            { pkg.saveToMap(mFile); }

        /// Remove a package from the file.
        /// @param name The name of the package to remove.
        void removePackage(OovString const &name);

        /// Get all packages in the file.
        std::vector<Package> getPackages() const;
#ifndef __linux__
        void read(OovStringRef const fn);
#endif
        NameValueFile &getFile()
            { return mFile; }

    protected:
        NameValueFile mFile;
    };


/// This is for a set of packages required to build a project.  This is
/// originally a copy of the ProjectPackages, but it is resolved so that
/// there are no wildcarded directories, and the libraries are in sorted
/// order during the build.
class BuildPackages
    {
    public:
        BuildPackages(bool readNow);

        /// Read from the build package file.
        bool read();

        /// Get a package by name.
        /// @param name The package to get.
        Package getPackage(OovStringRef const name) const
            { return mPackages.getPackage(name); }

        /// Get all packages.
        std::vector<Package> getPackages() const
            { return mPackages.getPackages(); }

        /// Check if the package exists.
        bool doesPackageExist(OovStringRef pkgName);

        /// Add a package.
        void insertPackage(Package const &pkg)
            { pkg.saveToMap(mPackages.getFile()); }

        /// Save all packages for the build.
        void savePackages()
            { mPackages.getFile().writeFile(); }

    protected:
        Packages mPackages;
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

/// These are packages that are defined to be used by the project.  These
/// package files may have wildcards indicating that the directories must be
/// scanned to get the include and library directories.
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
