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

    OovStatus status = FileDelete(getDiagFileName());
    if(status.needReport())
        {
        OovString err = "Unable to delete file ";
        err += getDiagFileName();
        status.report(ET_Error, err);
        }
    status = mIncDirMap.read(incDepsFilePath);
    if(status.needReport())
        {
        status.reported();      // Inc dir map is optional.
        }
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
    OovStringVec consumerFiles = mComponentFinder.getScannedComponentInfo().
        getComponentFiles(mComponentFinder.getComponentTypesFile(),
        ScannedComponentInfo::CFT_CppSource, compName);
    OovStringVec consumerIncs = mComponentFinder.getScannedComponentInfo().
        getComponentFiles(mComponentFinder.getComponentTypesFile(),
        ScannedComponentInfo::CFT_CppInclude, compName);
    std::copy(consumerIncs.begin(), consumerIncs.end(), std::back_inserter(consumerFiles));

    OovStringVec incRoots = pkg.getIncludeDirs();
    return mIncDirMap.anyRootDirsMatch(incRoots, consumerFiles);
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

OovStringSet ComponentBuilder::getComponentPackageCompileArgs(OovStringRef const compName)
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
    return linkArgs;
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
    ScannedComponentInfo const &scannedInfoFile =
        mComponentFinder.getScannedComponentInfo();
    ComponentTypesFile const &compTypes =
        mComponentFinder.getComponentTypesFile();
    OovStringVec compNames = scannedInfoFile.getComponentNames();
    if(compNames.size() > 0)
        {
        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            OovStringSet compileArgs = getComponentPackageCompileArgs(name);
            eCompTypes compType = compTypes.getComponentType(name);
            if(compType != CT_Unknown && compType != CT_JavaJarLib &&
                compType != CT_JavaJarProg)
                {
                OovStringVec cppSources = scannedInfoFile.getComponentFiles(
                    compTypes, ScannedComponentInfo::CFT_CppSource, name);
                if(pm == PM_CovInstr)
                    {
                    OovStringVec includes = scannedInfoFile.getComponentFiles(
                        compTypes, ScannedComponentInfo::CFT_CppInclude, name);
                    cppSources.insert(cppSources.end(), includes.begin(), includes.end());
                    }

                for(const auto &src : cppSources)
                    {
                    FilePath absSrc;
                    absSrc.getAbsolutePath(src, FP_File);
                    OovStringVec orderedCompIncRoots = mComponentFinder.getFileIncludeDirs(src);
                    OovStringVec orderedIncDirs =
                        mIncDirMap.getOrderedIncludeDirsForSourceFile(absSrc,
                        orderedCompIncRoots);
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
                    processCppSourceFile(pm, src, orderedIncDirs, incFiles, compileArgs);
                    }
                }
            if(compType == CT_JavaJarLib || compType == CT_JavaJarProg)
                {
                OovStringVec javaSources = scannedInfoFile.getComponentFiles(
                    compTypes, ScannedComponentInfo::CFT_JavaSource, name);
                processJavaSourceFiles(pm, name, javaSources /*, compileArgs*/);
                }
            }
        waitForCompletion();
        }
    }

void ComponentBuilder::generateDependencies()
    {
    ComponentTypesFile const &compTypesFile = getComponentTypesFile();
    OovStringVec compNames = compTypesFile.getDefinedComponentNames();
    BuildPackages const &buildPackages =
        mComponentFinder.getProjectBuildArgs().getBuildPackages();

    sVerboseDump.logProgress("Generate dependencies");
    // Find component dependencies
    for(const auto &compName : compNames)
        {
        /// @todo - this should be optimized so that only components with source files
        /// that have changed include dependencies are processed.
        // This is done for libraries too because they may need compile switches.
        eCompTypes compType = compTypesFile.getComponentType(compName);
        if(compType != CT_Unknown && compType != CT_JavaJarLib &&
            compType != CT_JavaJarProg)
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
    ScannedComponentInfo const &scannedInfoFile =
        mComponentFinder.getScannedComponentInfo();
    ComponentTypesFile const &compTypesFile =
            mComponentFinder.getComponentTypesFile();
    OovStringVec compNames = compTypesFile.getDefinedComponentNames();

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
            if(compTypesFile.getComponentType(name) == CT_StaticLib)
                {
                OovStringVec sources = scannedInfoFile.getComponentFiles(
                    compTypesFile, ScannedComponentInfo::CFT_CppSource, name);
                for(size_t i=0; i<sources.size(); i++)
                    {
                    sources[i] = makeOutputObjectFileName(sources[i]);
                    }
                if(sources.size() > 0)
                    {
                    allLibFileNames.push_back(makeLibFn(name));
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
            if(type == CT_Program || type == CT_SharedLib)
                {
                appendOrderedPackageLibs(name, externalLibDirs,
                        externalOrderedPackageLibNames);
                IndexedStringSet compPkgLinkArgs = getComponentPackageLinkArgs(name,
                        compTypesFile);

                OovStringVec sources = scannedInfoFile.getComponentFiles(
                    compTypesFile, ScannedComponentInfo::CFT_CppSource, name);
                makeExe(name, sources, projectLibFileNames,
                        externalLibDirs, externalOrderedPackageLibNames,
                        compPkgLinkArgs, type == CT_SharedLib);
                }
            }
        for(const auto &name : compNames)
            {
            auto type = compTypesFile.getComponentType(name);
            if(type == CT_JavaJarLib || type == CT_JavaJarProg)
                {
                bool prog = (type == CT_JavaJarProg);
                OovStringVec sources = scannedInfoFile.getComponentFiles(
                    compTypesFile, ScannedComponentInfo::CFT_JavaSource, name);
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
            OovString ownerComp = getComponentTypesFile().getComponentNameOwner(srcFile);
            mComponentFinder.setCompConfig(ownerComp);

            CppChildArgs ca;
            OovString procPath;
            if(pm == PM_CovInstr)
                {
                procPath = mComponentFinder.getProjectBuildArgs().getCovInstrToolPath();
                }
            else
                {
                procPath = mComponentFinder.getProjectBuildArgs().getCompilerPath();
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

void ComponentBuilder::processJavaSourceFiles(eProcessModes pm,
    OovStringRef compName, OovStringVec javaSources /*,
    const OovStringSet &externPkgCompileArgs*/)
    {
    OovString intDirName = ComponentTypesFile::getComponentDir(
        mIntermediatePath, compName);
//    OovString outFileName = ComponentTypesFile::getComponentFileName(
//        mOutputPath, compName, "jar");

    OovStatus status(true, SC_File);
    bool old = false;
    OovString relCompDir = mComponentFinder.getRelCompDir(compName);
    for(auto const &srcFile : javaSources)
        {
        FilePath outFileName(Project::makeTreeOutBaseFileName(srcFile,
            mSrcRootDir, mIntermediatePath), FP_File);
        outFileName.appendExtension(".class");
        if(FileStat::isOutputOld(outFileName, srcFile, status))
            {
            OovString ownerComp = getComponentTypesFile().getComponentNameOwner(srcFile);
            mComponentFinder.setCompConfig(ownerComp);
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
//                procPath = mToolPathFile.getCovInstrToolPath();
                procPath = mComponentFinder.getProjectBuildArgs().getJavaCompilerPath();
                }
            else
                {
                procPath = mComponentFinder.getProjectBuildArgs().getJavaCompilerPath();
                }
            ca.addArg(procPath);
            ca.addArg("-d");
            ca.addArg(intDirName);

            OovString classPathStr = mComponentFinder.getProjectBuildArgs().getJavaClassPath();
            if(classPathStr.length() > 0)
                {
                ca.addArg("-cp");
                FilePathQuoteCommandLinePath(classPathStr);
                ca.addArg(classPathStr);
                }

            ComponentFinder::appendArgs(true, mComponentFinder.getProjectBuildArgs().getJavaArgs(), ca);
            ComponentFinder::appendArgs(false, mComponentFinder.getProjectBuildArgs().getJavaArgs(), ca);
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

OovString ComponentBuilder::getSymbolBasePath()
    {
    FilePath outPath = mIntermediatePath;
    outPath.appendDir("sym");
    return outPath;
    }

void ComponentBuilder::makeLibSymbols(OovStringRef const clumpName,
        OovStringVec const &files)
    {
    std::string objSymbolTool = mComponentFinder.getProjectBuildArgs().getObjSymbolPath();

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
    OovString outFileName = makeLibFn(libPath);
    OovStatus status(true, SC_File);
    if(FileStat::isOutputOld(outFileName, objectFileNames, status))
        {
        OovString ownerComp = getComponentTypesFile().getComponentNameOwner(libPath);
        mComponentFinder.setCompConfig(ownerComp);
        OovString procPath = mComponentFinder.getProjectBuildArgs().getLibberPath();
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
        OovString ownerComp = getComponentTypesFile().getComponentNameOwner(compName);
        mComponentFinder.setCompConfig(ownerComp);
        OovString procPath = mComponentFinder.getProjectBuildArgs().getCompilerPath();
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

void ComponentBuilder::makeJar(OovStringRef const compName,
    OovStringVec const &sources, bool prog)
    {
    OovString outFileName = makeOutputJarName(compName);
    OovString relCompDir = mComponentFinder.getRelCompDir(compName);

    // Get the class file names in order to check times.
    OovStringVec classFilePaths;
    for(auto const &srcFile : sources)
        {
        FilePath outFileName(Project::makeTreeOutBaseFileName(srcFile,
            mSrcRootDir, mIntermediatePath), FP_File);
        outFileName.appendExtension("class");
        classFilePaths.push_back(outFileName);
        }

    OovStringVec projectJarFilePaths;
    if(prog)
        {
        // Get the jar file names in order to check times.
        OovStringVec jarLibs = mComponentFinder.getComponentTypesFile().
            getDefinedComponentNamesByType(CT_JavaJarLib);
        for(auto const &jar : jarLibs)
            {
            OovString jarFn = makeOutputJarName(jar);
            projectJarFilePaths.push_back(jarFn);
            }
        }
    OovStatus status(true, SC_File);
    if(FileStat::isOutputOld(outFileName, projectJarFilePaths, status) ||
        FileStat::isOutputOld(outFileName, classFilePaths, status))
        {
        OovString ownerComp = getComponentTypesFile().getComponentNameOwner(compName);
        mComponentFinder.setCompConfig(ownerComp);
        OovString procPath = mComponentFinder.getProjectBuildArgs().getJavaJarToolPath();
        OovProcessChildArgs ca;
        ca.addArg(procPath);
        OovString jarTypeArgs = "cf";
        if(prog)
            { jarTypeArgs += "m"; }
        ca.addArg(jarTypeArgs);
        ca.addArg(outFileName);

        if(prog)
            {
            FilePath manFile(mComponentFinder.getComponentTypesFile().
                getComponentAbsolutePath(relCompDir), FP_Dir);

            manFile.appendFile("Manifest.txt");
            ca.addArg(manFile);
            }

        // Go through the java source files and convert to directories that have class files.
        std::set<OovString> relClassDirs;
        for(auto const &src : sources)
            {
            FilePath compSourcePath(Project::getSrcRootDirectory(), FP_Dir);
            compSourcePath.appendDir(relCompDir);
            OovString srcFn = Project::getSrcRootDirRelativeSrcFileName(src,
                compSourcePath);
            FilePath relFn(srcFn, FP_File);
            relFn.discardFilename();
            relClassDirs.insert(relFn);
            }
        for(auto const &dir : relClassDirs)
            {
            // Don't need to use absolute path since the working dir is set
            // below in procArgs.
            OovString str = dir;
            str += "*.class";
            ca.addArg(str);
            }
        if(prog)
            {
            for(auto const &jarFn : projectJarFilePaths)
                {
                ca.addArg(jarFn);
                }
            }
        ProcessArgs procArgs(procPath, outFileName, ca);
        OovString intDirName = ComponentTypesFile::getComponentDir(
            mIntermediatePath, compName);
        procArgs.mWorkingDir = intDirName;
        addTask(procArgs);
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to check file times");
        }
    }
