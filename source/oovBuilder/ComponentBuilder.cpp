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
#include <stdlib.h>	// for getenv
#include <algorithm>


void ComponentBuilder::build(char const * const srcRootDir,
	char const * const incDepsFilePath,
	char const * const buildDirClass)
    {
    mSrcRootDir = srcRootDir;

    mOutputPath.setPath(Project::getOutputDir(buildDirClass).c_str(), FP_Dir);
    mIntermediatePath.setPath(Project::getIntermediateDir(buildDirClass).c_str(), FP_Dir);

    mToolPathFile.setConfig(buildDirClass);

    deleteFile(getDiagFileName().c_str());
    mIncDirMap.read(incDepsFilePath);
    buildComponents();
    }

bool const ComponentPkgDeps::isDependent(char const * const compName,
	char const * const pkgName) const
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
bool ComponentBuilder::anyIncDirsMatch(char const * const compName,
	RootDirPackage const &pkg)
    {
    std::string compDir = mComponentFinder.getComponentTypesFile().
	    getComponentAbsolutePath(compName);

    std::vector<std::string> incRoots = pkg.getIncludeDirs();
    return mIncDirMap.anyRootDirsMatch(incRoots, compDir.c_str());
    }

void ComponentBuilder::makeOrderedPackageLibs(char const * const compName,
	std::vector<std::string> &libDirs, std::vector<std::string> &sortedLibNames)
    {
    bool didAnything = false;
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    std::vector<Package> packages = buildPackages.getPackages();
    for(auto &pkg : packages)
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    if(!pkg.areLibraryNamesOrdered())
		{
		didAnything = true;
		std::vector<std::string> libNames = pkg.getScannedLibraryFilePaths();
		makeLibSymbols(pkg.getPkgName().c_str(), libNames);
		mObjSymbols.appendOrderedLibs(pkg.getPkgName().c_str(),
			getSymbolBasePath().c_str(), libDirs, sortedLibNames);

		pkg.setOrderedLibs(libDirs, sortedLibNames);
		buildPackages.insertPackage(pkg);
		}
	    }
	}
    if(didAnything)
	buildPackages.savePackages();
    }

void ComponentBuilder::appendOrderedPackageLibs(char const * const compName,
	std::vector<std::string> &libDirs, IndexedStringVec &sortedLibNames)
    {
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    std::vector<Package> packages = buildPackages.getPackages();
// This probably shouldn't be done since it does not include external packages.
// Just leave them in the original order specified.
//	    std::reverse(packages.begin(), packages.end());
    /// @todo - it might be nice to only make symbols
    /// at the time that a component needs the symbols.
    for(auto &pkg : packages)
	{
	int linkOrderIndex = mComponentFinder.getProject().
		getExternalPackageLinkOrder(pkg.getPkgName().c_str());
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    if(!pkg.areLibraryNamesOrdered())
		{
		}
	    else
		{
		std::vector<std::string> pkgLibs = pkg.getLibraryNames();
		for(auto &lib : pkgLibs)
		    sortedLibNames.push_back(IndexedString(linkOrderIndex++, lib));
//		std::copy(pkgLibs.begin(), pkgLibs.end(), std::back_inserter(sortedLibNames));

		std::vector<std::string> pkgLibDirs = pkg.getLibraryDirs();
		std::copy(pkgLibDirs.begin(), pkgLibDirs.end(), std::back_inserter(libDirs));
		}
	    }
	}
    }

std::set<std::string> ComponentBuilder::getComponentCompileArgs(char const * const compName,
	ComponentTypesFile const &file)
    {
    std::set<std::string> compileArgs;
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    for(auto const &arg : pkg.getCompileArgs())
		{
		compileArgs.insert(arg);
		}
	    }
	}
    std::string argStr = file.getComponentBuildArgs(compName);
    for(auto const &arg : CompoundValueRef::parseString(argStr.c_str()))
	{
	if(arg.find("-lnk", 0, 4) == std::string::npos)
	    {
	    compileArgs.insert(arg);
	    }
	}
    return compileArgs;
    }

IndexedStringSet ComponentBuilder::getComponentPackageLinkArgs(char const * const compName,
	ComponentTypesFile const &file)
    {
    IndexedStringSet linkArgs;
    BuildPackages &buildPackages = mComponentFinder.getProject().getBuildPackages();
    for(auto const &pkg : buildPackages.getPackages())
	{
	if(mComponentPkgDeps.isDependent(compName, pkg.getPkgName().c_str()))
	    {
	    int linkOrderIndex = mComponentFinder.getProject().getExternalPackageLinkOrder(
		    pkg.getPkgName().c_str());
	    for(auto const &arg : pkg.getLinkArgs())
		{
		linkArgs.insert(IndexedString(linkOrderIndex, arg));
		}
	    }
	}
    std::string argStr = file.getComponentBuildArgs(compName);
    for(auto const &arg : CompoundValueRef::parseString(argStr.c_str()))
	{
	if(arg.find("-lnk", 0, 4) != std::string::npos)
	    {
	    linkArgs.insert(IndexedString(0, arg.substr(4)));
	    }
	}
    return linkArgs;
    }

std::string makeLibFn(std::string const &outputPath, std::string const &packageName)
    {
    std::string libName = packageName;
    size_t libNamePos = rfindPathSep(libName.c_str());
    if(libNamePos != std::string::npos)
    	libNamePos++;
    else
    	libNamePos = 0;
    libName.insert(libNamePos, "lib");
    return(outputPath + libName + ".a");
    }

class LibTaskListener:public TaskQueueListener
    {
    public:
	std::vector<std::string> const &getBuiltLibs() const
	    { return mBuiltLibs; }
    private:
	std::vector<std::string> mBuiltLibs;
	virtual void extraProcessing(bool success, char const * const outFile,
		char const * const stdOutFn, ProcessArgs const &item) override
	    {
	    if(success)
		mBuiltLibs.push_back(outFile);
	    }
    };

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
//	http://www.mingw.org/wiki/IncludePathHOWTO - may use mingw path as base?
//
// clang user manual says it expects the following directories for mingw32. 64 uses path?
//	    C:/mingw/include
//	    C:/mingw/lib
//	    C:/mingw/lib/gcc/mingw32/4.[3-5].0/include/c++
//
// http://stackoverflow.com/questions/4779759/how-to-determine-inter-library-dependencies
// ldd utility lists the dynamic dependencies of executable files or shared objects
// C:\Program Files\GTK+-Bundle-3.6.1\lib	(contains glib, gmodule stuff on Windows)
void ComponentBuilder::buildComponents()
    {
    ComponentTypesFile const &compTypesFile =
	    mComponentFinder.getComponentTypesFile();
    std::vector<std::string> compNames = compTypesFile.getComponentNames();
    BuildPackages const &buildPackages =
	    mComponentFinder.getProject().getBuildPackages();

    sVerboseDump.logProgress("Generate dependencies");
    // Find component dependencies
    for(const auto &compName : compNames)
	{
	/// @todo - this should be optimized so that only components with source files
	/// that have changed include dependencies are processed.
	// This is done for libraries too because they may need compile switches.
	if(compTypesFile.getComponentType(compName.c_str()) != ComponentTypesFile::CT_Unknown)
	    {
	    for(auto const &pkg : buildPackages.getPackages())
		{
		if(anyIncDirsMatch(compName.c_str(), pkg))
		    {
		    mComponentPkgDeps.addPkgDep(compName.c_str(), pkg.getPkgName().c_str());
		    }
		}
	    }
	}

    sVerboseDump.logProgress("Compile objects");
    // Compile all objects.
    if(compNames.size() > 0)
        {
        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            std::set<std::string> compileArgs = getComponentCompileArgs(name.c_str(),
                compTypesFile);
            if(compTypesFile.getComponentType(name.c_str()) != ComponentTypesFile::CT_Unknown)
                {
                std::vector<std::string> sources =
                    compTypesFile.getComponentSources(name.c_str());

                std::vector<std::string> orderedIncRoots =
                    mComponentFinder.getAllIncludeDirs();
                for(const auto &src : sources)
                    {
                    FilePath absSrc;
                    absSrc.getAbsolutePath(src.c_str(), FP_File);
                    std::vector<std::string> incDirs =
                        mIncDirMap.getOrderedIncludeDirsForSourceFile(absSrc.c_str(),
                        orderedIncRoots);
                    /// @todo - this could be optimized to not check file times of external files.
                    /// @todo - more optimization could use the times in the incdeps file.
                    std::set<IncludedPath> incFilesSet;
                    mIncDirMap.getNestedIncludeFilesUsedBySourceFile(absSrc.c_str(),
                        incFilesSet);
                    std::vector<std::string> incFiles;
                    for(auto const &file : incFilesSet)
                        {
                        incFiles.push_back(file.getFullPath());
                        }
                    makeObj(src, incDirs, incFiles, compileArgs);
                    }
                }
            }
        waitForCompletion();
        }

    sVerboseDump.logProgress("Build libraries");

    // Build all project libraries
    std::vector<std::string> builtLibFileNames;
    std::vector<std::string> allLibFileNames;
    if(compNames.size() > 0)
        {
        LibTaskListener libListener;
        setTaskListener(&libListener);
        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
	    {
	    if(compTypesFile.getComponentType(name.c_str()) == ComponentTypesFile::CT_StaticLib)
		{
		std::vector<std::string> sources =
		    compTypesFile.getComponentSources(name.c_str());
		for(size_t i=0; i<sources.size(); i++)
		    {
		    sources[i] = makeOutputObjectFileName(sources[i].c_str());
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

    sVerboseDump.logProgress("Build programs");

    // Build programs
    std::vector<std::string> projectLibFileNames;
    mObjSymbols.appendOrderedLibFileNames("ProjLibs", getSymbolBasePath().c_str(),
	    projectLibFileNames);
    if(compNames.size() > 0)
        {
        for(const auto &name : compNames)
            {
            std::vector<std::string> externalLibDirs; 	// not in library search order, eliminate dups.
            std::vector<std::string> externalOrderedLibNames;
            makeOrderedPackageLibs(name.c_str(), externalLibDirs, externalOrderedLibNames);
            }

        setupQueue(getNumHardwareThreads());
        for(const auto &name : compNames)
            {
            StdStringVec externalLibDirs; 	// not in library search order, eliminate dups.
            IndexedStringVec externalOrderedPackageLibNames;
            auto type = compTypesFile.getComponentType(name.c_str());
            if(type == ComponentTypesFile::CT_Program ||
        	    type == ComponentTypesFile::CT_SharedLib)
        	{
        	appendOrderedPackageLibs(name.c_str(), externalLibDirs,
        		externalOrderedPackageLibNames);
        	IndexedStringSet compPkgLinkArgs = getComponentPackageLinkArgs(name.c_str(),
        		compTypesFile);

        	std::vector<std::string> sources =
        		compTypesFile.getComponentSources(name.c_str());
        	makeExe(name.c_str(), sources, projectLibFileNames,
        		externalLibDirs, externalOrderedPackageLibNames,
        		compPkgLinkArgs, type == ComponentTypesFile::CT_SharedLib);
        	}
            }
	waitForCompletion();
        }
    sVerboseDump.logProgress("Done building");
    }


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

bool ComponentTaskQueue::runProcess(char const * const procPath,
	char const * const outFile, const OovProcessChildArgs &args,
	InProcMutex &listenerMutex, char const * const stdOutFn)
    {
    FilePath outDir(outFile, FP_File);
    outDir.discardFilename();
    bool success = ensurePathExists(outDir.c_str());
    if(success)
	{
        std::string processStr = "oovBuilder Building ";
        processStr += outFile;
        processStr += '\n';
	File stdoutFile;
        OovProcessBufferedStdListener listener(listenerMutex);
        listener.setProcessIdStr(processStr.c_str());
	if(stdOutFn)
	    {
	    // The ar tool must send its output to a file.
	    stdoutFile.open(stdOutFn, "a");
	    listener.setStdOut(stdoutFile.getFp(), OovProcessStdListener::OP_OutputFile);
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
	success = pipeProc.spawn(procPath, args.getArgv(), listener, exitCode);
	if(!success || exitCode != 0)
	    {
	    fprintf(stderr, "oovBuilder: Unable to build %s\nArguments were: ", outFile);
	    args.printArgs(stderr);
	    }
	}
    else
	{
	fprintf(stderr, "oovBuilder: Unable to create directory %s\n",
	    outDir.c_str());
	}
    fflush(stdout);
    fflush(stderr);
    sVerboseDump.logProcessStatus(success);
    return success;
    }

bool ComponentTaskQueue::processItem(ProcessArgs const &item)
    {
    char const *stdOutFn = item.mStdOutFn.length() ? item.mStdOutFn.c_str() : nullptr;
    bool success = runProcess(item.mProcess.c_str(), item.mOutputFile.c_str(),
	        item.mChildArgs, mListenerStdMutex, stdOutFn);
    if(mListener)
	mListener->extraProcessing(success, item.mOutputFile.c_str(), stdOutFn, item);
    return success;
    }

std::string ComponentBuilder::makeOutputObjectFileName(char const * const srcFile)
    {
    std::string outFileName;
    Project::makeTreeOutBaseFileName(srcFile, mSrcRootDir, mIntermediatePath,
	    outFileName);
    outFileName += ".o";
    return outFileName;
    }

void ComponentBuilder::makeObj(const std::string &srcFile,
	const std::vector<std::string> &incDirs,
	const std::vector<std::string> &incFiles,
	const std::set<std::string> &externPkgCompileArgs)
    {
    if(isSource(srcFile.c_str()))
	{
	int incFileOlderIndex = -1;
	std::string outFileName = makeOutputObjectFileName(srcFile.c_str());
	if(FileStat::isOutputOld(outFileName.c_str(), srcFile.c_str()) ||
		FileStat::isOutputOld(outFileName.c_str(), incFiles, &incFileOlderIndex))
	    {
	    CppChildArgs ca;
	    std::string procPath = mToolPathFile.getCompilerPath();
	    ca.addArg(procPath.c_str());
	    ca.addCompileArgList(mComponentFinder, incDirs);
	    for(auto const &arg : externPkgCompileArgs)
		{
		ca.addArg(arg.c_str());
		}
	    ca.addArg("-o");
	    ca.addArg(outFileName.c_str());
	    ca.addArg(srcFile.c_str());

	    sVerboseDump.logProcess(srcFile.c_str(), ca.getArgv(), ca.getArgc());
            addTask(ProcessArgs(procPath.c_str(), outFileName.c_str(), ca));
	    if(incFileOlderIndex != -1)
		sVerboseDump.logOutputOld(incFiles[incFileOlderIndex].c_str());
            }
	}
    }

bool ComponentsFile::excludesMatch(const std::string &filePath,
	const std::vector<std::string> &excludes)
    {
    bool exclude = false;
    for(const auto &str : excludes)
	{
	FilePath normFilePath(filePath, FP_Dir);
	FilePath normExcludePath(str, FP_Dir);
	if(normFilePath.find(normExcludePath) != std::string::npos)
	    {
	    exclude = true;
	    break;
	    }
	}
    return exclude;
    }

std::string ComponentBuilder::getSymbolBasePath()
    {
    FilePath outPath = mIntermediatePath;
    outPath.appendDir("sym");
    return outPath;
    }

void ComponentBuilder::makeLibSymbols(char const * const clumpName,
	const std::vector<std::string> &files)
    {
    std::string objSymbolTool = mToolPathFile.getObjSymbolPath();

    mObjSymbols.makeClumpSymbols(clumpName, files,
	    getSymbolBasePath().c_str(), objSymbolTool.c_str(), *this);
    }

void ComponentBuilder::makeLib(const std::string &libPath,
	const std::vector<std::string> &objectFileNames)
    {
    std::string outFileName = makeLibFn(mOutputPath, libPath);
    if(FileStat::isOutputOld(outFileName.c_str(), objectFileNames))
	{
	std::string procPath = mToolPathFile.getLibberPath();
	OovProcessChildArgs ca;
	ca.addArg(procPath.c_str());
	ca.addArg("r");
	ca.addArg(outFileName.c_str());
	for(const auto &objName : objectFileNames)
	    {
	    ca.addArg(objName.c_str());
	    }
        sVerboseDump.logProcess(outFileName.c_str(), ca.getArgv(), ca.getArgc());
        addTask(ProcessArgs(procPath.c_str(), outFileName.c_str(), ca));
	}
    }


/// @param projectLibFilePaths Libary names from project. Includes full paths.
/// @param externPkgOrderedLibNames Library names from external packages
/// @param externPkgLinkArgs Link args from external packages
//
// getLinkArgs() contains -l from command line
void ComponentBuilder::makeExe(char const * const compName,
	const std::vector<std::string> &sources,
	const std::vector<std::string> &projectLibFilePaths,
	const StdStringVec &externLibsDirs,
	const IndexedStringVec &externPkgOrderedLibNames,
	const IndexedStringSet &externPkgLinkArgs,
	bool shared)
    {
    std::string exeName = mComponentFinder.makeFileName(compName);
    std::string outFileName = mOutputPath + exeName;
    if(shared)
	outFileName += ".so";
    else
	outFileName = makeExeFilename(outFileName.c_str());

    std::vector<std::string> objects;
    for(const auto &src : sources)
	{
	std::string objName = makeOutputObjectFileName(src.c_str());
	objects.push_back(objName.c_str());
	}

    if(FileStat::isOutputOld(outFileName.c_str(), projectLibFilePaths) ||
	    FileStat::isOutputOld(outFileName.c_str(), objects))
	{
	std::string procPath = mToolPathFile.getCompilerPath();
	OovProcessChildArgs ca;
	ca.addArg(procPath.c_str());
	ca.addArg("-o");
	ca.addArg(outFileName.c_str());
	if(shared)
	    ca.addArg("-shared");

	for(const auto &obj : objects)
	    ca.addArg(obj.c_str());

	IndexedStringVec libNames;	// must be vector for no sorting
	std::set<std::string> libDirs;

	for(size_t li=0; li<projectLibFilePaths.size(); li++)
	    {
	    FilePath libPath(projectLibFilePaths[li].c_str(), FP_File);
	    libDirs.insert(libPath.getDrivePath());
	    libNames.push_back(IndexedString(LOI_InternalProject+li,
		    libPath.getNameExt()));
	    }

	// These libs must be after project libs above for some projects.
	// May have to look at a better way of keeping original order.
	for(const auto &arg : mComponentFinder.getProject().getLinkArgs())
	    {
	    std::string temp = arg.mString;
	    CompoundValue::quoteCommandLineArg(temp);
	    if(temp.compare(0, 2, "-l") == 0)
		libNames.push_back(IndexedString(arg.mLinkOrderIndex, temp.substr(2)));
	    else
		ca.addArg(temp.c_str());
	    }
	for(const auto &arg : externPkgLinkArgs)
	    {
	    std::string temp = arg.mString;
	    CompoundValue::quoteCommandLineArg(temp);
	    if(temp.compare(0, 2, "-l") == 0)
		libNames.push_back(IndexedString(arg.mLinkOrderIndex, temp.substr(2)));
	    else
		ca.addArg(temp.c_str());
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
	    quoteCommandLinePath(quotedPath);
	    std::string arg = std::string("-L") + quotedPath;
	    ca.addArg(arg.c_str());
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
	    //		pango-1.0.lib must be -lpango-1.0
	    FilePath libFn(lib.mString.c_str(), FP_File);
	    if(libFn.getExtension().compare(".a") == 0)
		{
		libFn.discardMatchingHead("lib");
		arg += libFn.getName();		// with no extension
		}
	    else if(libFn.getExtension().compare(".lib") == 0)
		{
		arg += libFn.getName();		// with no extension
		}
	    else
		{
		// cannot discard extension or it will change "atk-1.0" to "atk-1"
		arg += libFn;
		}
	    ca.addArg(arg.c_str());
	    }
	sVerboseDump.logProcess(outFileName.c_str(), ca.getArgv(), ca.getArgc());
        addTask(ProcessArgs(procPath.c_str(), outFileName.c_str(), ca));
	}
    }
