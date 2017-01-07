/*
 * projFinder.cpp
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */
#include "ComponentFinder.h"
#include "Project.h"
#include "Debug.h"
#include "OovError.h"
#include <set>

#define DEBUG_FINDER 0
#if DEBUG_FINDER
static DebugFile sLog("oov-finder-debug.txt");
#endif


bool InsertOrderedSet::exists(OovStringRef const &str) const
    {
    bool exists = false;
    for(const auto &s : *this)
        {
        if(s.compare(str) == 0)
            {
            exists = true;
            break;
            }
        }
    return exists;
    }

void ScannedComponent::saveComponentFileInfo(
    ScannedComponentInfo::CompFileTypes cft, ProjectReader const &proj,
    OovStringRef const compName, ComponentTypesFile &compFile,
    ScannedComponentInfo &scannedFile, OovStringRef analysisPath,
    OovStringSet const &newFiles) const
    {
    OovStringSet deleteFiles;
    for(auto const &newFile : newFiles)
        {
        OovString newCompName = getComponentName(proj, newFile);
        OovStringVec origFiles = scannedFile.getComponentFiles(compFile, cft, newCompName, false);
        for(auto const &origFileName : origFiles)
            {
            if(newFiles.find(origFileName) == newFiles.end())
                {
                // Found a file that is not in the new set. It must have been
                // deleted. So delete the analysis file if it exists.
                OovString analysisFile = Project::makeAnalysisFileName(origFileName,
                        Project::getSrcRootDirectory(), analysisPath);
                deleteFiles.insert(analysisFile);
                }
            }
        }
    // It should be ok to delete analysis files since they should not conflict
    // with scanning the source directories.
    for(auto const &file : deleteFiles)
        {
        OovStatus status = FileDelete(file.getStr());
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to delete analysis files");
            }
        }
    scannedFile.setComponentFiles(cft, compName, newFiles);
    }

void ScannedComponent::saveComponentSourcesToFile(ProjectReader const &proj,
    OovStringRef const compName, ComponentTypesFile &compFile,
    ScannedComponentInfo &scannedFile, OovStringRef analysisPath) const
    {
    saveComponentFileInfo(ScannedComponentInfo::CFT_JavaSource, proj, compName,
        compFile, scannedFile, analysisPath, mJavaSourceFiles);
    saveComponentFileInfo(ScannedComponentInfo::CFT_CppSource, proj, compName,
        compFile, scannedFile, analysisPath, mCppSourceFiles);
    saveComponentFileInfo(ScannedComponentInfo::CFT_CppInclude, proj, compName,
        compFile, scannedFile, analysisPath, mCppIncludeFiles);
    }

OovString ScannedComponent::getComponentName(ProjectReader const &proj,
    OovStringRef const filePath)
    {
    FilePath path(filePath, FP_File);
    path.discardFilename();
    OovString compName = Project::getSrcRootDirRelativeSrcFileName(path,
            proj.getSrcRootDirectory());
    if(compName.length() == 0)
        {
        compName = Project::getRootComponentName();
        }
    else
        {
        FilePathRemovePathSep(compName, compName.length()-1);
        }
    return compName;
    }

OovString ComponentFinder::getComponentName(OovStringRef const filePath) const
    {
    return ScannedComponent::getComponentName(mProject, filePath);
    }


//////////////

ScannedComponentsInfo::MapIter ScannedComponentsInfo::addComponents(
    OovStringRef const compName)
    {
    FilePath parent = FilePath(compName, FP_Dir).getParent();
    if(parent.length() > 0)
        {
        FilePathRemovePathSep(parent, parent.length()-1);
        addComponents(parent);
        }
    MapIter it = mComponents.find(compName);
    if(it == mComponents.end())
        {
        it = mComponents.insert(std::make_pair(compName, ScannedComponent())).first;
        }
    return it;
    }

void ScannedComponentsInfo::addJavaSourceFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addJavaSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addCppSourceFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addCppSourceFileName(srcFileName);
    }

void ScannedComponentsInfo::addCppIncludeFile(OovStringRef const compName,
    OovStringRef const srcFileName)
    {
    MapIter it = addComponents(compName);
    (*it).second.addCppIncludeFileName(srcFileName);
    }

void ScannedComponentsInfo::initializeComponentTypesFileValues(
    ProjectReader const &proj, ComponentTypesFile &compFile,
    ScannedComponentInfo &scannedFile, OovStringRef analysisPath)
    {
    CompoundValue comps;
    for(const auto &comp : mComponents)
        {
        comps.addArg(comp.first);
        comp.second.saveComponentSourcesToFile(proj, comp.first, compFile,
            scannedFile, analysisPath);
        }
    scannedFile.setComponentNames(comps.getAsString());
    }

//////////////

bool ComponentFinder::readProject(OovStringRef const oovProjectDir,
    OovStringRef buildMode, OovStringRef const buildConfig)
    {
    mBuildConfigName = buildConfig;
    OovStatus status = mProject.readProject(oovProjectDir);
    if(status.ok())
        {
        mProjectBuildArgs.setBuildConfig(buildMode, buildConfig);
        mComponentTypesFile.setBuildEnvironment(&mProjectBuildArgs.getBuildEnv());
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read project for finding components");
        }
    return status.ok();
    }

OovStatus ComponentFinder::scanProject()
    {
    mScanningPackage = nullptr;
    mExcludeDirs = mProjectBuildArgs.getProjectExcludeDirs();
    OovStatus status = recurseDirs(mProject.getSrcRootDirectory().getStr());
    if(status.needReport())
        {
        status.report(ET_Error, "Error scanning project");
        }
    return status;
    }

void ComponentFinder::scanExternalProject(OovStringRef const externalRootSrch,
        Package const *pkg)
    {
    OovString externalRootDir;
    parseProjRefs(externalRootSrch, externalRootDir, mExcludeDirs);

    Package rootPkg;
    if(pkg)
        rootPkg = *pkg;
    rootPkg.setRootDirPackage(externalRootSrch);
    mScanningPackage = &rootPkg;
    mAddIncs = rootPkg.needIncs();
    mAddLibs = rootPkg.needLibs();
    rootPkg.clearDirScan();

    OovStatus status = recurseDirs(externalRootDir.getStr());
    if(status.needReport())
        {
        OovString err = "Error scanning external project ";
        err += externalRootDir;
        status.report(ET_Error, err);
        }

    addBuildPackage(rootPkg);
    }

void ComponentFinder::addBuildPackage(Package const &pkg)
    {
    getProjectBuildArgs().getBuildPackages().insertPackage(pkg);
    OovStatus status = getProjectBuildArgs().getBuildPackages().savePackages();
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to save build packages");
        }
    }

void ComponentFinder::saveProject(OovStringRef analysisPath)
    {
    mScannedInfo.initializeComponentTypesFileValues(mProject, mComponentTypesFile,
        mScannedComponentInfo, analysisPath);
    OovStatus status = mScannedComponentInfo.writeScannedInfo();
    if(status.needReport())
        {
        OovString errStr = "Unable to save component types file";
        status.report(ET_Error, errStr);
        }
    }

bool ComponentFinder::processFile(OovStringRef const filePath)
    {
    /// @todo - find files with no extension? to match things like std::vector include
    if(!excludesMatch(filePath, mExcludeDirs))
        {
        bool cppInc = isCppHeader(filePath);
        bool cppSrc = isCppSource(filePath);
        bool javaSrc = isJavaSource(filePath);
        bool lib = isLibrary(filePath);
        if(mScanningPackage)
            {
            if(cppInc)
                {
                if(mAddIncs)
                    {
                    FilePath path(filePath, FP_File);
                    path.discardFilename();

                    mScanningPackage->appendAbsoluteIncDir(path.getDrivePath());

                    // Insert parent for things like #include "gtk/gtk.h" or "sys/stat.h"
                    FilePath parent(path.getParent());
                    if(parent.length() > 0)
                        mScanningPackage->appendAbsoluteIncDir(parent.getDrivePath());

                    // Insert grandparent for things like "llvm/ADT/APInt.h"
                    FilePath grandparent(parent.getParent());
                    if(grandparent.length() > 0)
                        mScanningPackage->appendAbsoluteIncDir(grandparent.getDrivePath());
                    }
                }
            else if(lib)
                {
                if(mAddLibs)
                    {
                    FilePath path(filePath, FP_File);
                    mScanningPackage->appendAbsoluteLibName(filePath);
                    }
                }
            }
        else
            {
            if(cppSrc)
                {
                std::string name = getComponentName(filePath);
                mScannedInfo.addCppSourceFile(name, filePath);
                }
            else if(cppInc)
                {
                FilePath path(filePath, FP_File);
                path.discardFilename();
                mScannedInfo.addProjectIncludePath(path);
                std::string name = getComponentName(filePath);
                mScannedInfo.addCppIncludeFile(name, filePath);
                }
            else if(javaSrc)
                {
                std::string name = getComponentName(filePath);
                mScannedInfo.addJavaSourceFile(name, filePath);
                }
            }
        }
    return true;
    }

OovStringVec ComponentFinder::getAllIncludeDirs() const
    {
    InsertOrderedSet projIncs = getScannedInfo().getProjectIncludeDirs();
    OovStringVec incs;
    std::copy(projIncs.begin(), projIncs.end(), std::back_inserter(incs));
    for(auto const &pkg : getProjectBuildArgs().getBuildPackages().getPackages())
        {
        OovStringVec pkgIncs = pkg.getIncludeDirs();
        std::copy(pkgIncs.begin(), pkgIncs.end(), std::back_inserter(incs));
        }
    return incs;
    }

enum ComponentFinder::VarFileTypes ComponentFinder::getVariableFileType(OovStringRef const filePath) const
    {
    VarFileTypes vft = VFT_Cpp;
    if(isCppSource(filePath))
        { vft = VFT_Cpp; }
    else if(isJavaSource(filePath))
        { vft = VFT_Java; }
    return vft;
    }

/*
OovString ComponentFinder::getVariableValue(VariableName vn, OovStringRef const srcFile) const
    {
    OovString value;

    char const *platform = Project::getPlatform();
    char const *bldConfigName = getBuildConfigName();
    VarFileTypes vft = getVariableFileType(srcFile);
    OovString compName = getComponentName(srcFile);
    switch(vn)
        {
        case VN_ConvertTool:
            break;

        case VN_ConvertToolArgs:
            break;

        case VN_CombineTool:
// http://www.conifersystems.com/whitepapers/gnu-make/
// Age dependency checking, timestamps are bad when multiple computers are used. Get time
// from central location? Could be simple order counter, but would need a place to store
// the counter for each file.
//
// Compiling is dependent on header files, but is not a build dependency since headers are done.
// So components are only dependent on another component if dependent on the output of the comp.
// Typically combiners are dependent on converters and other combiners.
//          Cpp Lib       rule:   $toolpath r $compName.[lib|a] &infiles.[obj|o]
//          Cpp Link      rule:   $toolpath -o $compName.[exe|so|] [-shared]
            break;

        case VN_IntDeps:
            {
            // How to distinguish between 32/64, etc.
            // Use Any or All to match
            // Use Or And? Always And?
            // Cannot tell difference between bldConfigName and compName, so either
            // position must be used, or tags. Position is bad because it is limited to a
            // fixed number of variables. Allow custom defined variables for each build config.
            //     platform(plt): buildcfg(cfg): component(cmp): filetype(ft): file(f): langtype(lt):
            // With tags, not specified means any, so "[]" or "" means any.
            // [plt=Linux & ft=Cpp][+/=]
            OovString depsStr = mComponentTypesFile.getComponentDepsInternal(compName);
            // Combine platform and bldConfigName?
            //  custom          platform=Linux/Windows
            //  custom          bits=16/32
            //  compName=*
            //  bldConfigName=Analyze/Debug/Release/etc.
            //  vft=Cpp/Java
            }
            break;
        }
    return value;
    }
*/

OovStringVec ComponentFinder::getFileIncludeDirs(OovStringRef const srcFile) const
    {
/*
    OovString dirStr = getVariableValue(VN_IntDeps, srcFile);
    CompoundValue dirs;
    if(dirStr.length() != 0)
        {
        dirs.parseString(dirStr);
        }
    else
        {
        dirs = getAllIncludeDirs();
        }
    return dirs;
*/
    return getAllIncludeDirs();
    }

OovString ComponentFinder::makeActualComponentName(OovStringRef const projName) const
    {
    OovString fn = projName;
    if(strcmp(projName, Project::getRootComponentName()) == 0)
        {
        fn = Project::getRootComponentFileName();
        }
    return fn;
    }

OovString ComponentFinder::getRelCompDir(OovStringRef const projName)
    {
    OovString fn = projName;
    if(strcmp(projName, Project::getRootComponentName()) == 0)
        { fn = ""; }
    return fn;
    }

void ComponentFinder::appendArgs(bool appendSwitchArgs, OovStringRef argStr,
    CppChildArgs &childArgs)
    {
    CompoundValue javaArgs;
    javaArgs.parseString(argStr);
    for(auto const &arg : javaArgs)
        {
        if(appendSwitchArgs == (arg[0] == '-'))
            {
            childArgs.addArg(arg);
            }
        }
    }

void ComponentFinder::parseProjRefs(OovStringRef const arg, OovString &rootDir,
        OovStringVec &excludes)
    {
    excludes.clear();
    OovStringVec tokens = StringSplit(arg, '!');
    if(rootDir.size() == 0)
        rootDir = tokens[0];
    if(tokens.size() > 1)
        {
        excludes.resize(tokens.size()-1);
        std::copy(tokens.begin()+1, tokens.end(), excludes.begin());
        }
    }

bool ComponentFinder::excludesMatch(OovStringRef const filePath,
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

void CppChildArgs::addCompileArgList(const ComponentFinder &finder,
        const OovStringVec &incDirs)
    {
    // add cpp args
    for(const auto &arg : finder.getProjectBuildArgs().getCompileArgs())
        {
        std::string tempArg = arg;
        CompoundValue::quoteCommandLineArg(tempArg);
        addArg(tempArg);
        }

    for(size_t i=0; static_cast<size_t>(i)<incDirs.size(); i++)
        {
        std::string incStr = std::string("-I") + incDirs[i];
        CompoundValue::quoteCommandLineArg(incStr);
        addArg(incStr);
        }
    }

