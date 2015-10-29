/*
 * ComponentBuilder.cpp
 *
 *  Created on: Sep 4, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentBuilder.h"
#include "srcFileParser.h"
#include "ObjSymbols.h"
#include "File.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>     // for getenv
#include <algorithm>

TaskQueueListener::~TaskQueueListener()
    {}

void ComponentBuilder::build(eProcessModes mode, OovStringRef const incDepsFilePath,
        OovStringRef const buildDirClass)
    {
    mSrcRootDir.setPath(Project::getSrcRootDirectory(), FP_Dir);
    if(mode == PM_CovInstr)
        {
        mOutputPath.setPath(Project::getCoverageSourceDirectory(), FP_Dir);
        }
    else
        {
        mOutputPath.setPath(Project::getBuildOutputDir(buildDirClass), FP_Dir);
        }
    mIntermediatePath.setPath(Project::getIntermediateDir(buildDirClass), FP_Dir);

    mToolPathFile.setConfig(buildDirClass);

    OovStatus status = FileDelete(getDiagFileName());
    if(status.needReport())
        {
        OovString err = "Unable to delete file ";
        err += getDiagFileName();
        status.report(ET_Error, err);
        }
    mIncDirMap.read(incDepsFilePath);
    if(mode == PM_CovInstr)
        {
        sVerboseDump.logProgress("Instrument source");
        processSourceForComponents(PM_CovInstr);
        }
    else
        {
        buildComponents();
        }
    }

bool ComponentPkgDeps::isDependent(OovStringRef const compName,
        OovStringRef const pkgName) const
    {
    bool dependent = false;
    auto const &mapIt = mCompPkgMap.find(compName);
    if(mapIt != mCompPkgMap.end())
        {
        auto const &pkgSet = mapIt->second;
        dependent = (pkgSet.find(pkgName) != pkgSet.end());
        }
    return dependent;
    }


// libNames = all libs from -EP, -ER and -l
bool ComponentBuilder::anyIncDirsMatch(OovStringRef const compName,
        RootDirPackage const &pkg)
    {
    std::string compDir = mComponentFinder.getComponentTypesFile().
            getComponentAbsolutePath(compName);

    OovStringVec incRoots = pkg.getIncludeDirs();
    return mIncDirMap.anyRootDirsMatch(incRoots, compDir);
    }

void ComponentBuilder::makeOrderedPackageLibs(OovStringRef const compName)
    {
    bool didAnything = false;
    BuildPackages &buildPackages = mComponentFinder.getProjectBuildArgs().getBuildPackages();
    std::vector<Package> packages = buildPackages.getPackages();
    for(auto &pkg : packages)
        {
        if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName()))
            {
            if(!pkg.areLibraryNamesOrdered())
                {
                OovStringVec libDirs;       // not in library search order, eliminate dups.
                OovStringVec sortedLibNames;

                didAnything = true;
                OovStringVec libNames = pkg.getScannedLibraryFilePaths();
                makeLibSymbols(pkg.getPkgName(), libNames);
                mObjSymbols.appendOrderedLibs(pkg.getPkgName(),
                        getSymbolBasePath(), libDirs, sortedLibNames);

                pkg.setOrderedLibs(libDirs, sortedLibNames);
                buildPackages.insertPackage(pkg);
                }
            }
        }
    if(didAnything)
        {
        OovStatus status = buildPackages.savePackages();
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to save build packages for libraries");
            }
        }
    }

void ComponentBuilder::appendOrderedPackageLibs(OovStringRef const compName,
        OovStringVec &libDirs, IndexedStringVec &sortedLibNames)
    {
    BuildPackages &buildPackages = mComponentFinder.getProjectBuildArgs().getBuildPackages();
    std::vector<Package> packages = buildPackages.getPackages();
// This probably shouldn't be done since it does not include external packages.
// Just leave them in the original order specified.
//          std::reverse(packages.begin(), packages.end());
    /// @todo - it might be nice to only make symbols
    /// at the time that a component needs the symbols.
    for(auto &pkg : packages)
        {
        unsigned int linkOrderIndex = mComponentFinder.getProjectBuildArgs().
                getExternalPackageLinkOrder(pkg.getPkgName());
        if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName()))
            {
            if(!pkg.areLibraryNamesOrdered())
                {
                }
            else
                {
                OovStringVec pkgLibs = pkg.getLibraryNames();
                for(auto &lib : pkgLibs)
                    {
                    sortedLibNames.push_back(IndexedString(linkOrderIndex++, lib));
                    }
//              std::copy(pkgLibs.begin(), pkgLibs.end(), std::back_inserter(sortedLibNames));

                OovStringVec pkgLibDirs = pkg.getLibraryDirs();
                std::copy(pkgLibDirs.begin(), pkgLibDirs.end(), std::back_inserter(libDirs));
                }
            }
        }
    }

OovStringSet ComponentBuilder::getComponentCompileArgs(OovStringRef const compName,
        ComponentTypesFile const & /* file */ )
    {
    OovStringSet compileArgs;
    BuildPackages &buildPackages = mComponentFinder.getProjectBuildArgs().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
        {
        if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName()))
            {
            for(auto const &arg : pkg.getCompileArgs())
                {
                compileArgs.insert(arg);
                }
            }
        }
// lnk args should not be sent to clang?
// clang compiler gives, "unknown argument: '-mwindows'" when -lnk-mwindows is used.
// instead, use: "-lnk-Wl,--subsystem,windows"
// https://gcc.gnu.org/ml/gcc-help/2004-01/msg00225.html
/*
    OovString argStr = file.getComponentBuildArgs(compName);
    for(auto const &arg : CompoundValueRef::parseString(argStr))
        {
        if(arg.find("-lnk", 0, 4) == std::string::npos)
            {
            compileArgs.insert(arg);
            }
        }
*/
    return compileArgs;
    }

IndexedStringSet ComponentBuilder::getComponentPackageLinkArgs(OovStringRef const compName,
        ComponentTypesFile const &file)
    {
    IndexedStringSet linkArgs;
    BuildPackages &buildPackages = mComponentFinder.getProjectBuildArgs().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
        {
        if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName()))
            {
            unsigned int linkOrderIndex = mComponentFinder.getProjectBuildArgs().
                getExternalPackageLinkOrder(pkg.getPkgName());

            // It is possible to look at special handling of pthreads by gcc using
            //          gcc -dumpspecs | grep pthread
            // Generally for compiling it uses special flags, and for linking adds the lib.
            OovStringVec linkArgsVec = pkg.getLinkArgs();
            // Special handling for pthread
            OovStringVec compileArgs = pkg.getCompileArgs();
            for(auto const &arg : compileArgs)
                {
                if(arg == "-pthread")
                    {
                    linkArgsVec.push_back(arg);
                    }
                }
            for(auto const &arg : linkArgsVec)
                {
                linkArgs.insert(IndexedString(linkOrderIndex, arg));
                }
            }
        }
    std::string argStr = file.getComponentBuildArgs(compName);
    for(auto const &arg : CompoundValueRef::parseString(argStr))
        {
        if(arg.find("-lnk", 0, 4) != std::string::npos)
            {
            linkArgs.insert(IndexedString(0, arg.substr(4)));
            }
        }
    return linkArgs;
    }

static OovString makeLibFn(OovStringRef const outputPath, OovStringRef const packageName)
    {
    OovString libName = packageName;
    size_t libNamePos = FilePathGetPosEndDir(libName);
    libName.insert(libNamePos, "lib");
    OovString outFn = outputPath;
    outFn += libName + ".a";
    return(outFn);
    }

class LibTaskListener:public TaskQueueListener
    {
    public:
        OovStringVec const &getBuiltLibs() const
            { return mBuiltLibs; }
    private:
        OovStringVec mBuiltLibs;
        virtual void extraProcessing(bool success, OovStringRef const outFile,
                OovStringRef const /*stdOutFn*/, ProcessArgs const & /*item*/) override;
    };

void LibTaskListener::extraProcessing(bool success, OovStringRef const outFile,
    OovStringRef const /*stdOutFn*/, ProcessArgs const & /*item*/)
    {
    if(success)
        mBuiltLibs.push_back(outFile);
    }

void ComponentBuilder::processSourceForComponents(eProcessModes pm)
    {
    ComponentTypesFile const &compTypesFile =
            mComponentFinder.getComponentTypesFile();
    OovStringVec compNames = compTypesFile.getComponentNames();
    if(compNames.size() > 0)
        {
        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            // compTypesFile is not used!!!
            OovStringSet compileArgs = getComponentCompileArgs(name,
                compTypesFile);
            ComponentTypesFile::eCompTypes compType = compTypesFile.getComponentType(name);
            if(compType != ComponentTypesFile::CT_Unknown &&
                compType != ComponentTypesFile::CT_JavaJarLib &&
                compType != ComponentTypesFile::CT_JavaJarProg)
                {
                OovStringVec cppSources = compTypesFile.getComponentFiles(
                    ComponentTypesFile::CFT_CppSource, name);
                if(pm == PM_CovInstr)
                    {
                    OovStringVec includes = compTypesFile.getComponentFiles(
                        ComponentTypesFile::CFT_CppInclude, name);
                    cppSources.insert(cppSources.end(), includes.begin(), includes.end());
                    }

                OovStringVec orderedIncRoots = mComponentFinder.getAllIncludeDirs();
                for(const auto &src : cppSources)
                    {
                    FilePath absSrc;
                    absSrc.getAbsolutePath(src, FP_File);
                    OovStringVec incDirs =
                        mIncDirMap.getOrderedIncludeDirsForSourceFile(absSrc,
                        orderedIncRoots);
                    /// @todo - this could be optimized to not check file times of external files.
                    /// @todo - more optimization could use the times in the incdeps file.
                    std::set<IncludedPath> incFilesSet;
                    mIncDirMap.getNestedIncludeFilesUsedBySourceFile(absSrc,
                        incFilesSet);
                    OovStringVec incFiles;
                    for(auto const &file : incFilesSet)
                        {
                        incFiles.push_back(file.getFullPath());
                        }
                    processCppSourceFile(pm, src, incDirs, incFiles, compileArgs);
                    }
                }
            if(compType == ComponentTypesFile::CT_JavaJarLib ||
                compType == ComponentTypesFile::CT_JavaJarProg)
                {
                bool prog = (compType == ComponentTypesFile::CT_JavaJarProg);
                OovStringVec javaSources = compTypesFile.getComponentFiles(
                        ComponentTypesFile::CFT_JavaSource, name);
                processJavaSourceFiles(pm, prog, name, javaSources, compileArgs);
                }
            }
        waitForCompletion();
        }
    }

void ComponentBuilder::generateDependencies()
    {
    ComponentTypesFile const &compTypesFile =
            mComponentFinder.getComponentTypesFile();
    OovStringVec compNames = compTypesFile.getComponentNames();
    BuildPackages const &buildPackages =
            mComponentFinder.getProjectBuildArgs().getBuildPackages();

    sVerboseDump.logProgress("Generate dependencies");
    // Find component dependencies
    for(const auto &compName : compNames)
        {
        /// @todo - this should be optimized so that only components with source files
        /// that have changed include dependencies are processed.
        // This is done for libraries too because they may need compile switches.
        ComponentTypesFile::eCompTypes compType =
                compTypesFile.getComponentType(compName);
        if(compType != ComponentTypesFile::CT_Unknown &&
            compType != ComponentTypesFile::CT_JavaJarLib &&
            compType != ComponentTypesFile::CT_JavaJarProg)
            {
            for(auto const &pkg : buildPackages.getPackages())
                {
                if(anyIncDirsMatch(compName, pkg))
                    {
                    mComponentPkgDeps.addPkgDep(compName, pkg.getPkgName());
                    }
                }
            }
        }
    }

// GCC-4.3 has c++0x
// LLVM 3.2 has c++0x and experimental binaries for mingw32/x86
// LLVM 3.3 has c++11
// G++ 4.7.2 and clang3.2 may work with -stdlib=libstdc++

//
// Some points about compilation:
//   - Both clang and gnu/mingw compilers have built-in include path parsing.
//   The interesting point is that if include paths to mingw are specified on
//   the command line, then the compile will fail when the wrong "string" file
//   is found. It is best to discard any mingw directory include paths, and let
//   the compiler do its own thing.
//
//   - Some #includes such as "gtk/gtk.h" must save the include path as the
//   parent directory location, so these cannot be merely found by looking
//   at the include file name.
//
//   - For optimized dependency checking, there is usually no need to check
//   file dependencies of files outside of the project,
//
// Compilers find mingw directories without passing as -I args
//      http://www.mingw.org/wiki/IncludePathHOWTO - may use mingw path as base?
//
// clang user manual says it expects the following directories for mingw32. 64 uses path?
//          C:/mingw/include
//          C:/mingw/lib
//          C:/mingw/lib/gcc/mingw32/4.[3-5].0/include/c++
//
// http://stackoverflow.com/questions/4779759/how-to-determine-inter-library-dependencies
// ldd utility lists the dynamic dependencies of executable files or shared objects
// C:\Program Files\GTK+-Bundle-3.6.1\lib       (contains glib, gmodule stuff on Windows)
void ComponentBuilder::buildComponents()
    {
    ComponentTypesFile const &compTypesFile =
            mComponentFinder.getComponentTypesFile();
    OovStringVec compNames = compTypesFile.getComponentNames();

    sVerboseDump.logProgress("Generating package dependencies");
    generateDependencies();
    sVerboseDump.logProgress("Compile objects");
    // Compile all objects.
    processSourceForComponents(PM_Build);
    sVerboseDump.logProgress("Build libraries");

    // Build all project libraries
    OovStringVec builtLibFileNames;
    OovStringVec allLibFileNames;
    if(compNames.size() > 0)
        {
        LibTaskListener libListener;
        setTaskListener(&libListener);
        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            if(compTypesFile.getComponentType(name) == ComponentTypesFile::CT_StaticLib)
                {
                OovStringVec sources = compTypesFile.getComponentFiles(
                    ComponentTypesFile::CFT_CppSource, name);
                for(size_t i=0; i<sources.size(); i++)
                    {
                    sources[i] = makeOutputObjectFileName(sources[i]);
                    }
                if(sources.size() > 0)
                    {
                    allLibFileNames.push_back(makeLibFn(mOutputPath, name));
                    makeLib(name, sources);
                    }
                }
            }
        waitForCompletion();
        builtLibFileNames = libListener.getBuiltLibs();
        setTaskListener(nullptr);

        if(builtLibFileNames.size() > 0)
           {
           makeLibSymbols("ProjLibs", allLibFileNames);
           }
        }


    // Build programs
    OovStringVec projectLibFileNames;
    mObjSymbols.appendOrderedLibFileNames("ProjLibs", getSymbolBasePath(),
            projectLibFileNames);
    if(compNames.size() > 0)
        {
        sVerboseDump.logProgress("Order external package libraries");

        for(const auto &name : compNames)
            {
            makeOrderedPackageLibs(name);
            }

        sVerboseDump.logProgress("Build programs ");

        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            OovStringVec externalLibDirs;       // not in library search order, eliminate dups.
            IndexedStringVec externalOrderedPackageLibNames;
            auto type = compTypesFile.getComponentType(name);
            if(type == ComponentTypesFile::CT_Program ||
                    type == ComponentTypesFile::CT_SharedLib)
                {
                appendOrderedPackageLibs(name, externalLibDirs,
                        externalOrderedPackageLibNames);
                IndexedStringSet compPkgLinkArgs = getComponentPackageLinkArgs(name,
                        compTypesFile);

                OovStringVec sources = compTypesFile.getComponentFiles(
                    ComponentTypesFile::CFT_CppSource, name);
                makeExe(name, sources, projectLibFileNames,
                        externalLibDirs, externalOrderedPackageLibNames,
                        compPkgLinkArgs, type == ComponentTypesFile::CT_SharedLib);
                }
            }
        for(const auto &name : compNames)
            {
            auto type = compTypesFile.getComponentType(name);
            if(type == ComponentTypesFile::CT_JavaJarLib ||
                type == ComponentTypesFile::CT_JavaJarProg)
                {
                bool prog = (type == ComponentTypesFile::CT_JavaJarProg);
                OovStringVec sources = compTypesFile.getComponentFiles(
                    ComponentTypesFile::CFT_JavaSource, name);
                makeJar(name, sources, prog);
                }
            }
        waitForCompletion();
        }
    sVerboseDump.logProgress("Done building");
    }


bool ComponentTaskQueue::runProcess(OovStringRef const procPath,
    OovStringRef const outFile, const OovProcessChildArgs &args,
    InProcMutex &listenerMutex, OovStringRef const stdOutFn,
    OovStringRef const workingDir)
    {
    FilePath outDir(outFile, FP_File);
    outDir.discardFilename();
    bool success = true;
    OovStatus status = FileEnsurePathExists(outDir);
    if(status.ok())
        {
        File stdoutFile;        // This must have a lifetime greater than listener.
        OovString processStr = "oovBuilder Building ";
        processStr += outFile;
        processStr += '\n';
        printf("%s", processStr.getStr());
        fflush(stdout);
        OovProcessBufferedStdListener listener(listenerMutex);
        listener.setProcessIdStr(processStr);
        if(stdOutFn)
            {
            // The ar tool must send its output to a file.
            status = stdoutFile.open(stdOutFn, "a");
            listener.setStdOut(stdoutFile.getFp(), OovProcessStdListener::OP_OutputFile);
            if(status.needReport())
                {
                OovString err = "Unable to open library file output ";
                err += stdOutFn;
                status.report(ET_Error, err);
                }
            }
/*
        else if(sVerboseDump.isOpen())
            {
            listener.setErrOut(sVerboseDump.getFp(), OovProcessStdListener::OP_OutputStdAndFile);
            listener.setStdOut(sVerboseDump.getFp(), OovProcessStdListener::OP_OutputStdAndFile);
            }
*/
        int exitCode;
        OovPipeProcess pipeProc;
        success = pipeProc.spawn(procPath, args.getArgv(), listener, exitCode, workingDir);
        if(!success)
            fprintf(stderr, "OovBuilder: Unable to execute process %s\n", procPath.getStr());
        if(!success || exitCode != 0)
            {
            fprintf(stderr, "oovBuilder: Unable to build %s\n", outFile.getStr());
            if(workingDir)
                { fprintf(stderr, "  Working dir: %s\n", workingDir.getStr()); }
            fprintf(stderr, "  Arguments were: ");
            args.printArgs(stderr);
            }
        }
    if(status.needReport())
        {
        OovString err = "oovBuilder: Unable to create directory ";
        err += outDir.getStr();
        status.report(ET_Error, err);
        }
    fflush(stdout);
    fflush(stderr);
    return status.ok() && success;
    }

bool ComponentTaskQueue::processItem(ProcessArgs const &item)
    {
    char const *stdOutFn = item.mStdOutFn.length() ? item.mStdOutFn.getStr() : nullptr;
    char const *workingDir = nullptr;
    if(item.mWorkingDir.length() > 0)
        {
        workingDir = item.mWorkingDir.getStr();
        }
    bool success = runProcess(item.mProcess, item.mOutputFile,
        item.mChildArgs, mListenerStdMutex, stdOutFn, workingDir);
    if(mListener)
        mListener->extraProcessing(success, item.mOutputFile, stdOutFn, item);
    return success;
    }

OovString ComponentBuilder::makeOutputObjectFileName(OovStringRef const srcFile)
    {
    OovString outFileName = Project::makeTreeOutBaseFileName(srcFile,
        mSrcRootDir, mIntermediatePath);
    outFileName += ".o";
    return outFileName;
    }

OovString ComponentBuilder::makeIntermediateClassDirName(OovStringRef const compName)
    {
    FilePath path(mIntermediatePath, FP_Dir);
    if(strcmp(compName, Project::getRootComponentName()) != 0)
        { path.appendDir(compName); }
    return path;
    }

OovString ComponentBuilder::makeOutputJarName(OovStringRef const compName)
    {
    FilePath outFileName(mOutputPath, FP_Dir);
    if(strcmp(compName, Project::getRootComponentName()) != 0)
        { outFileName.appendDir(compName); }
    OovString jarName = mComponentFinder.makeActualComponentName(compName);
    outFileName.appendFile(jarName);
    outFileName.appendExtension("jar");
    return outFileName;
    }

void ComponentBuilder::processCppSourceFile(eProcessModes pm, OovStringRef const srcFile,
        OovStringVec const &incDirs, OovStringVec const &incFiles,
        OovStringSet const &externPkgCompileArgs)
    {
    bool processFile = isCppSource(srcFile);
    if(pm == PM_CovInstr && !processFile)
        {
        processFile = isCppHeader(srcFile);
        }
    if(processFile)
        {
        static size_t BadIndex = static_cast<size_t>(-1);
        size_t incFileOlderIndex = BadIndex;
        OovString outFileName;
        if(pm == PM_CovInstr)
            {
            outFileName = Project::makeCoverageSourceFileName(srcFile, mSrcRootDir);
            }
        else
            {
            outFileName = makeOutputObjectFileName(srcFile);
            }
        OovStatus status(true, SC_File);
        if(FileStat::isOutputOld(outFileName, srcFile, status) ||
                FileStat::isOutputOld(outFileName, incFiles, status, &incFileOlderIndex))
            {
            CppChildArgs ca;
            OovString procPath;
            if(pm == PM_CovInstr)
                {
                procPath = mToolPathFile.getCovInstrToolPath();
                }
            else
                {
                procPath = mToolPathFile.getCompilerPath();
                }
            ca.addArg(procPath);
            ca.addArg(srcFile);
            if(pm == PM_CovInstr)
                {
                ca.addArg(mSrcRootDir);
                ca.addArg(mOutputPath);
                }
            ca.addCompileArgList(mComponentFinder, incDirs);
            for(auto const &arg : externPkgCompileArgs)
                {
                ca.addArg(arg);
                }
            ca.addArg("-o");
            ca.addArg(outFileName);

            sVerboseDump.logProcess(srcFile, ca.getArgv(), static_cast<int>(ca.getArgc()));
            addTask(ProcessArgs(procPath, outFileName, ca));
            if(incFileOlderIndex != BadIndex)
                sVerboseDump.logOutputOld(incFiles[static_cast<size_t>(incFileOlderIndex)]);
            }
        }
    }

void ComponentBuilder::processJavaSourceFiles(eProcessModes pm, bool prog,
    OovStringRef compName, OovStringVec javaSources,
    const OovStringSet &externPkgCompileArgs)
    {
    OovString intDirName = makeIntermediateClassDirName(compName);
    OovString outFileName = makeOutputJarName(compName);

    OovStatus status(true, SC_File);
    bool old = false;
    for(auto const &srcFile : javaSources)
        {
        if(FileStat::isOutputOld(outFileName, srcFile, status))
            {
            old = true;
            break;
            }
        if(!status.ok())
            {
            break;
            }
        }
    if(old)
        {
/*
        if(status.ok())
            {
            status = FileEnsurePathExists(outDirName);
            }
*/
        if(status.ok())
            {
            status = FileEnsurePathExists(intDirName);
            }
        FilePath srcFileListFn(intDirName, FP_Dir);
        srcFileListFn.appendFile("sources.txt");
        File srcFileList;
        if(status.ok())
            {
            status = srcFileList.open(srcFileListFn, "w");
            }
        if(status.ok())
            {
            for(auto const &srcFile : javaSources)
                {
                OovString str = srcFile + "\n";
                status = srcFileList.putString(str);
                if(!status.ok())
                    {
                    break;
                    }
                }
            if(status.ok())
                {
                srcFileList.close();
                }
            }
        if(status.ok())
            {
            CppChildArgs ca;
            OovString procPath;
            if(pm == PM_CovInstr)
                {
//            procPath = mToolPathFile.getCovInstrToolPath();
                }
            else
                {
                procPath = mToolPathFile.getJavaCompilerPath();
                }
            ca.addArg(procPath);
            ca.addArg("-d");
            ca.addArg(intDirName);
            OovString srcArg = "@";
            srcArg += srcFileListFn;
            ca.addArg(srcArg);
/*
        if(pm == PM_CovInstr)
            {
            ca.addArg(mSrcRootDir);
            ca.addArg(mOutputPath);
            }
        ca.addCompileArgList(mComponentFinder, incDirs);
        for(auto const &arg : externPkgCompileArgs)
            {
            ca.addArg(arg);
            }
        ca.addArg("-o");
        ca.addArg(outFileName);
*/
OovString str = "classes for ";
str += compName;
            sVerboseDump.logProcess(srcFileListFn, ca.getArgv(), static_cast<int>(ca.getArgc()));
            addTask(ProcessArgs(procPath, str, ca));
            }
//        if(incFileOlderIndex != BadIndex)
//            sVerboseDump.logOutputOld(incFiles[static_cast<size_t>(incFileOlderIndex)]);
        }
    }

bool ComponentsFile::excludesMatch(OovStringRef const filePath,
        OovStringVec const &excludes)
    {
    bool exclude = false;
    for(const auto &str : excludes)
        {
        FilePath normFilePath(filePath, FP_File);
        FilePath normExcludePath(str, FP_File);
        if(normFilePath.find(normExcludePath) != std::string::npos)
            {
            exclude = true;
            break;
            }
        }
    return exclude;
    }

OovString ComponentBuilder::getSymbolBasePath()
    {
    FilePath outPath = mIntermediatePath;
    outPath.appendDir("sym");
    return outPath;
    }

void ComponentBuilder::makeLibSymbols(OovStringRef const clumpName,
        OovStringVec const &files)
    {
    std::string objSymbolTool = mToolPathFile.getObjSymbolPath();

    OovString str = "Make lib symbols: ";
    str += clumpName;
    str += "\n";
    sVerboseDump.logProgress(str);

    mObjSymbols.makeClumpSymbols(clumpName, files,
            getSymbolBasePath(), objSymbolTool, *this);
    }

void ComponentBuilder::makeLib(OovStringRef const libPath,
        OovStringVec const &objectFileNames)
    {
    OovString outFileName = makeLibFn(mOutputPath, libPath);
    OovStatus status(true, SC_File);
    if(FileStat::isOutputOld(outFileName, objectFileNames, status))
        {
        OovString procPath = mToolPathFile.getLibberPath();
        OovProcessChildArgs ca;
        ca.addArg(procPath);
        ca.addArg("r");
        ca.addArg(outFileName);
        for(const auto &objName : objectFileNames)
            {
            ca.addArg(objName);
            }
        sVerboseDump.logProcess(outFileName, ca.getArgv(), static_cast<int>(ca.getArgc()));
        addTask(ProcessArgs(procPath, outFileName, ca));
        }
    }

static void appendLibName(OovString libName, size_t linkOrderIndex,
    IndexedStringVec &libNames,
    OovProcessChildArgs &ca)
    {
    CompoundValue::quoteCommandLineArg(libName);
    if(libName.compare(0, 2, "-l") == 0)
        libNames.push_back(IndexedString(linkOrderIndex, libName.substr(2)));
    else
        ca.addArg(libName);
    }

/// @param projectLibFilePaths Libary names from project. Includes full paths.
/// @param externPkgOrderedLibNames Library names from external packages
/// @param externPkgLinkArgs Link args from external packages
//
// getLinkArgs() contains -l from command line
void ComponentBuilder::makeExe(OovStringRef const compName,
        OovStringVec const &sources,
        OovStringVec const &projectLibFilePaths,
        OovStringVec const &externLibsDirs,
        const IndexedStringVec &externPkgOrderedLibNames,
        const IndexedStringSet &externPkgLinkArgs,
        bool shared)
    {
    OovString exeName = mComponentFinder.makeActualComponentName(compName);
    OovString outFileName = mOutputPath + exeName;
    if(shared)
        outFileName += ".so";
    else
        outFileName = FilePathMakeExeFilename(outFileName);

    OovStringVec objects;
    for(const auto &src : sources)
        {
        OovString objName = makeOutputObjectFileName(src);
        objects.push_back(objName);
        }

    OovStatus status(true, SC_File);
    if(FileStat::isOutputOld(outFileName, projectLibFilePaths, status) ||
            FileStat::isOutputOld(outFileName, objects, status))
        {
        OovString procPath = mToolPathFile.getCompilerPath();
        OovProcessChildArgs ca;
        ca.addArg(procPath);
        ca.addArg("-o");
        ca.addArg(outFileName);
        if(shared)
            ca.addArg("-shared");

        for(const auto &obj : objects)
            ca.addArg(obj);

        IndexedStringVec libNames;      // must be vector for no sorting
        std::set<std::string> libDirs;

        for(size_t li=0; li<projectLibFilePaths.size(); li++)
            {
            FilePath libPath(projectLibFilePaths[li], FP_File);
            libDirs.insert(libPath.getDrivePath());
            libNames.push_back(IndexedString(LOI_InternalProject+li,
                    libPath.getNameExt()));
            }

        // These libs must be after project libs above for some projects.
        // May have to look at a better way of keeping original order.
        for(const auto &arg : mComponentFinder.getProjectBuildArgs().getLinkArgs())
            {
            appendLibName(arg.mString, arg.mLinkOrderIndex, libNames, ca);
            }
        for(const auto &arg : externPkgLinkArgs)
            {
            appendLibName(arg.mString, arg.mLinkOrderIndex, libNames, ca);
            }

        for(auto const &dir : externLibsDirs)
            {
            libDirs.insert(dir);
            }
        std::copy(externPkgOrderedLibNames.begin(), externPkgOrderedLibNames.end(),
                std::back_inserter(libNames));

        for(const auto &path : libDirs)
            {
            std::string quotedPath = path;
            FilePathQuoteCommandLinePath(quotedPath);
            std::string arg = std::string("-L") + quotedPath;
            ca.addArg(arg);
            }

        std::sort(libNames.begin(), libNames.end(),
                [](IndexedString const &a, IndexedString const &b) -> bool
                    { return(a.mLinkOrderIndex < b.mLinkOrderIndex); }
                );
        for(const auto &lib : libNames)
            {
            std::string arg = "-l";
            // Removing .a or .lib works in Windows.
            // On Windows, clang is libclang.lib, and the link arg must be -llibclang
            //          pango-1.0.lib must be -lpango-1.0
            FilePath libFn(lib.mString, FP_File);
            if(libFn.getExtension().compare(".a") == 0)
                {
                libFn.discardMatchingHead("lib");
                arg += libFn.getName();         // with no extension
                }
            else if(libFn.getExtension().compare(".lib") == 0)
                {
                arg += libFn.getName();         // with no extension
                }
            else
                {
                // cannot discard extension or it will change "atk-1.0" to "atk-1"
                arg += libFn;
                }
            ca.addArg(arg);
            }
        sVerboseDump.logProcess(outFileName, ca.getArgv(), ca.getArgc());
        addTask(ProcessArgs(procPath, outFileName, ca));
        }
    }

void ComponentBuilder::makeJar(OovStringRef const rawCompName,
    OovStringVec const &sources, bool prog)
    {
    OovString compName = mComponentFinder.makeActualComponentName(rawCompName);
    OovString outFileName = makeOutputJarName(rawCompName);

    OovString procPath = mToolPathFile.getJavaJarToolPath();
    OovProcessChildArgs ca;
    ca.addArg(procPath);
    OovString jarTypeArgs = "cf";
    if(prog)
        { jarTypeArgs += "m"; }
    ca.addArg(jarTypeArgs);
    ca.addArg(outFileName);

    OovString outCompName = mComponentFinder.makeOutputComponentName(rawCompName);
    if(prog)
        {
        FilePath manFile(mComponentFinder.getComponentTypesFile().
            getComponentAbsolutePath(outCompName), FP_Dir);

        manFile.appendFile("Manifest.txt");
        ca.addArg(manFile);
        }
    // Go through the java source files and convert to directories that have class files.
    std::set<OovString> relClassDirs;
    for(auto const &src : sources)
        {
        FilePath compSourcePath(Project::getSrcRootDirectory(), FP_Dir);
        compSourcePath.appendDir(outCompName);
        OovString srcFn = Project::getSrcRootDirRelativeSrcFileName(src,
            compSourcePath);
        FilePath relFn(srcFn, FP_File);
        relFn.discardFilename();
        relClassDirs.insert(relFn);
        }
    for(auto const &dir : relClassDirs)
        {
        OovString str = dir;
        str += "*.class";
        ca.addArg(str);
        }
    OovStringVec jarLibs = mComponentFinder.getComponentTypesFile().
            getComponentNamesByType(ComponentTypesFile::CT_JavaJarLib);
    for(auto const &jar : jarLibs)
        {
        OovString jarFn = makeOutputJarName(jar);
        ca.addArg(jarFn);
        }
    ProcessArgs procArgs(procPath, outFileName, ca);
    OovString intDirName = makeIntermediateClassDirName(rawCompName);
    procArgs.mWorkingDir = intDirName;
    addTask(procArgs);
    }
