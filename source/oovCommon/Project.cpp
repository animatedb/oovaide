/*
 * Project.cpp
 *
 *  Created on: Jul 16, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Project.h"
#include "Packages.h"
#include "OovError.h"
#ifdef __linux__
#include <unistd.h>     // For readlink
#endif

OovString Project::sProjectDirectory;
OovString Project::sSourceRootDirectory;
#ifndef __linux__
OovString Project::sArgv0;
#endif

static char sCoverageSourceDir[] = "oov-cov";
static char sCoverageProjectDir[] = "oov-cov-oovaide";



OovString makeBuildConfigArgName(OovStringRef const baseName,
        OovStringRef const buildConfig)
    {
    OovString name = baseName;
    name += '-';
    name += buildConfig;
    return name;
    }

void Project::setArgv0(OovStringRef arg)
    {
#ifndef __linux__
    sArgv0 = arg;
#endif
    }

OovString Project::getComponentTypesFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovaide-comptypes.txt");
    return fn;
    }

OovString Project::getComponentSourceListFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovaide-tmp-compsources.txt");
    return fn;
    }

OovString Project::getPackagesFilePath()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovaide-pkg.txt");
    return fn;
    }

OovString Project::getBuildPackagesFilePath()
    {
    FilePath fn(Project::getProjectDirectory(), FP_Dir);
    fn.appendFile("oovaide-tmp-buildpkg.txt");
    return fn;
    }

OovStringRef const Project::getSrcRootDirectory()
    {
    if(sSourceRootDirectory.length() == 0)
        {
        NameValueFile file(getProjectFilePath());
        OovStatus status = file.readFile();
        if(status.needReport())
            {
            OovString str = "Unable to read project file to get source: ";
            str += getProjectFilePath();
            status.report(ET_Error, str);
            }
        sSourceRootDirectory = file.getValue(OptSourceRootDir);
        }
    return sSourceRootDirectory;
    }

OovString const Project::getBinDirectory()
    {
    static OovString path;
    if(path.length() == 0)
        {
        OovStatus status(true, SC_File);
#ifdef __linux__
        char buf[300];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
        if(len >= 0)
            {
            buf[len] = '\0';
            path = buf;
            size_t pos = path.rfind('/');
            if(pos == std::string::npos)
                {
                pos = path.rfind('\\');
                }
            if(pos != std::string::npos)
                {
                path.resize(pos+1);
                }
            }
/*
        if(FileIsFileOnDisk("./oovaide", status))
            {
            path = "./";
            }
        else if(FileIsFileOnDisk("/usr/local/bin/oovaide", status))
            {
            path = "/usr/local/bin/";
            }
*/
        else
            {
            path = "/usr/bin";
            }
#else
        if(FileIsFileOnDisk("./oovaide.exe", status))
            {
            path = "./";
            }
        else
            {
            path = FilePathGetDrivePath(sArgv0);
            }
#endif
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to get bin directory");
            }
        }
    return path;
    }

static bool isDevelopment()
    {
    static enum eDevel { Uninit, Devel, Deploy } devel;
    if(devel == Uninit)
        {
        OovString path = Project::getBinDirectory();
        size_t pos = path.find("bin");
        if(pos != std::string::npos)
            {
            path.replace(pos, 3, "lib");
            }
        OovStatus status(true, SC_File);
        if(FileIsDirOnDisk(path, status))
            {
            devel = Deploy;
            }
        else
            {
            devel = Devel;
            }
        if(status.needReport())
            {
            status.reported();
            }
        }
    return(devel == Devel);
    }

OovString const Project::getLibDirectory()
    {
    OovString path = getBinDirectory();
#ifdef __linux__
    // During development, the libs/.so are in the bin dir.
    if(!isDevelopment())
        {
        size_t pos = path.find("bin");
        if(pos != std::string::npos)
            {
            path.replace(pos, 3, "lib");
            }
        }
#endif
    return path;
    }

OovString const Project::getDataDirectory()
    {
    FilePath path(getBinDirectory(), FP_Dir);
#ifdef __linux__
    if(isDevelopment())
        {
        path.appendDir("data");
        }
    else
        {
        size_t pos = path.find("bin");
        if(pos != std::string::npos)
            {
            path.replace(pos, 3, "share/oovaide/data");
            }
        }
#else
    path.appendDir("data");
#endif
    return path;
    }

OovString const Project::getDocDirectory()
    {
    FilePath path(getBinDirectory(), FP_Dir);
#ifdef __linux__
    if(isDevelopment())
        {
        path.appendDir("../../web");
        }
    else
        {
        size_t pos = path.find("bin");
        if(pos != std::string::npos)
            {
            path.replace(pos, 3, "share/oovaide/doc");
            }
        }
#else
    path.appendDir("data");
#endif
    OovStatus status(true, SC_File);
    if(!FileIsDirOnDisk(path, status))
        {
        path.setPath("http://oovaide.sourceforge.net", FP_Dir);
        }
    return path;
    }

OovString Project::getProjectFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovaide.txt");
    return fn;
    }

OovString Project::getGuiOptionsFilePath()
    {
    FilePath fn(sProjectDirectory, FP_Dir);
    fn.appendFile("oovaide-gui.txt");
    return fn;
    }

static FilePath getDir(OovStringRef prefix, OovStringRef buildDirClass,
    OovStringRef projDir)
    {
    std::string outClass(prefix);
    outClass += buildDirClass;
    FilePath dir(projDir, FP_Dir);
    dir.appendDir(outClass);
    return dir;
    }

FilePath Project::getOutputDir()
    {
    FilePath fp(Project::getProjectDirectory(), FP_Dir);
    fp.appendDir("output");
    OovStatus status = FileEnsurePathExists(fp);
    if(status.needReport())
        {
        OovString err = "Unable to create directory ";
        err += fp;
        status.report(ET_Error, fp);
        }
    return fp;
    }

FilePath Project::getBuildOutputDir(OovStringRef const buildDirClass)
    {
    return getDir("out-", buildDirClass, getProjectDirectory());
    }

FilePath Project::getIntermediateDir(OovStringRef const buildDirClass)
    {
    return getDir("bld-", buildDirClass, getProjectDirectory());
    }

OovString Project::getSrcRootDirRelativeSrcFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir)
    {
    OovString relSrcFileName = srcFileName;
    size_t pos = relSrcFileName.find(srcRootDir.getStr());
    if(pos != std::string::npos)
        {
        relSrcFileName.erase(pos, srcRootDir.numBytes());
        FilePathRemovePathSep(relSrcFileName, 0);
        }
    return relSrcFileName;
    }

OovString Project::getSrcRootDirRelativeSrcFileName(OovStringRef const srcFileName)
    {
    return getSrcRootDirRelativeSrcFileName(srcFileName, getSourceRootDirectory());
    }

OovString Project::getSrcRootDirRelativeSrcFileDir(OovStringRef const srcRootDir,
        OovStringRef const srcFileName)
    {
    FilePath relSrcFileDir(srcFileName, FP_Dir);
    size_t pos = relSrcFileDir.find(srcRootDir);
    if(pos != std::string::npos)
        {
        relSrcFileDir.erase(pos, srcRootDir.numBytes());
        FilePathRemovePathSep(relSrcFileDir, 0);
        }
    return relSrcFileDir;
    }

OovString Project::getSrcRootDirRelativeSrcFileDir(OovStringRef const srcFileName)
    {
    return getSrcRootDirRelativeSrcFileDir(Project::getSrcRootDirectory(), srcFileName);
    }

static OovString getAbsoluteDirFromSrcRootDirRelativeDir(
    OovStringRef const relSrcFileDir)
    {
    FilePath absSrcFileDir(Project::getSrcRootDirectory(), FP_Dir);
    absSrcFileDir.appendDir(relSrcFileDir);
    return absSrcFileDir;
    }

OovString Project::makeOutBaseFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir, OovStringRef const outFilePath)
    {
    OovString file = getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir);
    if(file[0] == '/')
        file.erase(0, 1);
    file.replaceStrs("_", "_u");
    file.replaceStrs("/", "_s");
    file.replaceStrs(".", "_d");
    FilePath outFileName(outFilePath, FP_Dir);
    outFileName.appendFile(file);
    return outFileName;
    }

OovString Project::recoverFileName(OovStringRef const srcFileName)
    {
    OovString file = srcFileName;
    file.replaceStrs("_u", "_");
    file.replaceStrs("_s", "/");
    file.replaceStrs("_d", ".");
    return file;
    }

OovString Project::makeTreeOutBaseFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir, OovStringRef const outFilePath)
    {
    OovString file = getSrcRootDirRelativeSrcFileName(srcFileName, srcRootDir);
    if(file[0] == '/')
        file.erase(0, 1);

    FilePath tmpOutFileName(outFilePath, FP_Dir);
    tmpOutFileName.appendFile(file);
    tmpOutFileName.discardExtension();
    return tmpOutFileName;
    }

OovString Project::makeAnalysisFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir, OovStringRef const analysisDir)
    {
    OovString outFileName = makeOutBaseFileName(srcFileName,
            srcRootDir, analysisDir);
    outFileName += ".xmi";
    return outFileName;
    }

OovString Project::getCoverageSourceDirectory()
    {
    FilePath coverageDir(getProjectDirectory(), FP_Dir);
    coverageDir.appendDir(sCoverageSourceDir);
    return coverageDir;
    }

OovString Project::makeCoverageSourceFileName(OovStringRef const srcFileName,
        OovStringRef const srcRootDir)
    {
    OovString coverageDir = getCoverageSourceDirectory();
    OovString outFileName = makeTreeOutBaseFileName(srcFileName, srcRootDir,
            coverageDir);
    FilePath srcFn(srcFileName, FP_File);
    outFileName += srcFn.getExtension();
    return outFileName;
    }

OovString Project::getCoverageProjectDirectory()
    {
    FilePath coverageDir(getProjectDirectory(), FP_Dir);
    coverageDir.appendDir(sCoverageProjectDir);
    return coverageDir;
    }


OovStatusReturn ProjectReader::readProject(OovStringRef const oovProjectDir)
    {
    Project::setProjectDirectory(oovProjectDir);
    setFilename(Project::getProjectFilePath());
    OovStatus status = readFile();
    if(status.ok())
        {
        Project::setSourceRootDirectory(getValue(OptSourceRootDir));
        }
    return status;
    }

void ProjectBuildArgs::loadBuildArgs(OovStringRef const buildConfigName)
    {
    OovStatus status = mProjectPackages.read();
    if(status.needReport())
        {
        // These packages are optional.
        status.clearError();
        }
    status = mBuildPackages.read();
    if(status.needReport())
        {
        // These packages are optional.
        status.clearError();
        }

    OovStringVec args;
    CompoundValue baseArgs;
    baseArgs.parseString(mProjectOptions.getValue(OptBaseArgs));
    for(auto const &arg : baseArgs)
        {
        args.push_back(arg);
        }

    std::string optionExtraArgs = makeBuildConfigArgName(OptExtraBuildArgs,
            buildConfigName);
    CompoundValue extraArgs;
    extraArgs.parseString(mProjectOptions.getValue(optionExtraArgs));
    extraArgs.quoteAllArgs();
    for(auto const &arg : extraArgs)
        {
        args.push_back(arg);
        }
    parseArgs(args);
    }

void ProjectBuildArgs::parseArgs(OovStringVec const &args)
    {
    unsigned int linkOrderIndex = LOI_AfterInternalProject;
    for(auto const &arg : args)
        {
        if(arg.find("-ER", 0, 3) == 0)
            {
            addExternalArg(arg);
            }
        else if(arg.find("-EP", 0, 3) == 0)
            {
            addExternalPackageName(linkOrderIndex, arg.substr(3));
            linkOrderIndex += LOI_PackageIncrement;
            }
        else if(arg.find("-bv", 0, 3) == 0)
            {
            mVerbose = true;
            }
        else if(arg.find("-lnk", 0, 4) == 0)
            {
            addLinkArg(linkOrderIndex++, arg.substr(4));
            }
        else
            {
            addCompileArg(arg);
            }
        }
    for(auto const &arg : args)
        {
        if(arg.find("-EP", 0, 3) == 0)
            handleExternalPackage(arg.substr(3));
        }
    }

CompoundValue ProjectBuildArgs::getProjectExcludeDirs() const
    {
    CompoundValue val;
    val.parseString(mProjectOptions.getValue(OptProjectExcludeDirs));
// This doesn't work.
//    val.push_back(sCoverageSourceDir);
//    val.push_back(sCoverageProjectDir);
    for(size_t i=0; i<val.size(); i++)
        {
        val[i] = getAbsoluteDirFromSrcRootDirRelativeDir(val[i]);
        }
    return val;
    }

void ProjectBuildArgs::handleExternalPackage(OovStringRef const pkgName)
    {
    addPackageCrcName(pkgName);
    Package pkg = mProjectPackages.getPackage(pkgName);
    OovStringVec cppArgs = pkg.getCompileArgs();
    for(auto const &arg : cppArgs)
        {
        addPackageCrcCompileArg(arg);
        }
    OovStringVec incDirs = pkg.getIncludeDirs();
    for(auto const &dir : incDirs)
        {
        std::string arg = "-I";
        arg += dir;
        addPackageCrcCompileArg(arg);
        }
    OovStringVec linkArgs = pkg.getLinkArgs();
    for(auto const &arg : linkArgs)
        {
        addPackageCrcLinkArg(arg);
        }
    OovStringVec libDirs = pkg.getLibraryDirs();
    for(auto const &dir : libDirs)
        {
        std::string arg = "-L";
        arg += dir;
        addPackageCrcLinkArg(arg);
        }
    OovStringVec libNames = pkg.getLibraryNames();
    for(auto const &dir : libNames)
        {
        std::string arg = "-l";
        arg += dir;
        addPackageCrcLinkArg(arg);
        }
    }

const OovStringVec ProjectBuildArgs::getAllCrcCompileArgs() const
    {
    OovStringVec vec;
    vec = mCompileArgs;
    std::copy(mPackageCrcCompileArgs.begin(), mPackageCrcCompileArgs.end(),
            std::back_inserter(vec));
    std::copy(mPackageCrcNames.begin(), mPackageCrcNames.end(),
            std::back_inserter(vec));
    return vec;
    }

const OovStringVec ProjectBuildArgs::getAllCrcLinkArgs() const
    {
    OovStringVec vec;
    for(auto item : mLinkArgs)
        vec.push_back(item.mString);
    std::copy(mPackageCrcLinkArgs.begin(), mPackageCrcLinkArgs.end(),
            std::back_inserter(vec));
    return vec;
    }

unsigned int ProjectBuildArgs::getExternalPackageLinkOrder(OovStringRef const pkgName) const
    {
    unsigned int index = LOI_AfterInternalProject;
    for(auto &item : mExternalPackageNames)
        {
        if(item.mString.compare(pkgName) == 0)
            index = item.mLinkOrderIndex;
        }
    return index;
    }
