/*
 * Options.cpp
 *
 *  Created on: Jun 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Options.h"
#include "Project.h"
#include "OovString.h"
#include <string.h>


#ifndef __linux__
/*
// Return is empty if search string not found
static std::string getPathSegment(const std::string &path, OovStringRef const srch)
    {
    OovString lowerPath;
    lowerPath.setLowerCase(path);
    std::string seg;
    size_t pos = lowerPath.find(srch);
    if(pos != std::string::npos)
        {
        std::string arg = "-ER";
        size_t startPos = path.rfind(';', pos);
        if(startPos != std::string::npos)
            startPos++;
        else
            startPos = 0;
        size_t endPos = path.find(';', pos);
        if(endPos == std::string::npos)
            endPos = path.length();
        seg = path.substr(startPos, endPos-startPos);
        }
    return seg;
    }

static bool addExternalReference(const std::string &path, OovStringRef const srch,
        CompoundValue &args)
    {
    std::string seg = getPathSegment(path, srch);
    size_t binPos = seg.find("bin", seg.length()-3);
    if(binPos != std::string::npos)
        {
        seg.resize(binPos);
        removePathSep(seg, binPos-1);
        }
    if(seg.size() > 0)
        {
        std::string arg = "-ER";
        arg += seg;
        args.addArg(arg);
        }
    return seg.size() > 0;
    }
*/
#endif

static void addCLangArgs(CompoundValue &cv)
    {
    cv.addArg("-x");
    cv.addArg("c++");
    cv.addArg("-std=c++11");
    }


static void setCppArgs(NameValueFile &file, OovStringRef filterName,
    OovStringRef const buildConfig, OovStringRef extraArgs)
    {
    BuildVariable buildVar;
    buildVar.setVarName(OptCppArgs);
    buildVar.setFunction(BuildVariable::F_Append);
    buildVar.addFilter(filterName, buildConfig);
    file.setNameValue(buildVar.getVarFilterName().getStr(), extraArgs);
    }


static void setBuildConfigurationPaths(NameValueFile &file,
    bool useclang)
    {
    BuildVariable buildVar;

    // Assume the archiver is installed and on path.
    // llvm-ar gives link error.
    // setNameValue(makeExeFilename("llvm-ar"));
    buildVar.setVarName(OptCppLibPath);
    buildVar.setFunction(BuildVariable::F_Assign);
    file.setNameValue(buildVar.getVarFilterName(), FilePathMakeExeFilename("ar"));

    // llvm-nm gives bad file error on Windows, and has no output on Linux.
    // mPathObjSymbol = "makeExeFilename(llvm-nm)";
    buildVar.setVarName(OptObjSymbolPath);
    file.setNameValue(buildVar.getVarFilterName(), FilePathMakeExeFilename("nm"));

    std::string compiler;
    if(useclang)
        compiler = FilePathMakeExeFilename("clang++");
    else
        compiler = FilePathMakeExeFilename("g++");
    buildVar.setVarName(OptCppCompilerPath);
    file.setNameValue(buildVar.getVarFilterName(), compiler);

    buildVar.setVarName(OptJavaJarPath);
    file.setNameValue(buildVar.getVarFilterName(), "jar");

    buildVar.setVarName(OptJavaCompilerPath);
    file.setNameValue(buildVar.getVarFilterName(), "javac");

    // For analysis mode, set different java paths
    buildVar.addFilter(OptFilterNameBuildConfig, BuildConfigAnalysis);
    file.setNameValue(buildVar.getVarFilterName(), "java");

    buildVar.setVarName(OptJavaAnalyzerPath);
    file.setNameValue(buildVar.getVarFilterName(), "oovJavaParser");
    }

#ifdef __linux__
static OovStringVec getSystemIncludePaths(bool useCLangBuild)
    {
    OovStringVec sysIncPaths;
    FilePath clangOutputFileName(Project::getProjectDirectory(), FP_Dir);
    clangOutputFileName.appendFile("oovaide-temp-clang.txt");
    OovString cmdStr = "echo \"\" | ";
    if(useCLangBuild)
        { cmdStr += "clang++"; }
    else
        { cmdStr += "g++"; }
    cmdStr += " -v -x c++ -E - 2>";
    cmdStr += clangOutputFileName;
    system(cmdStr.getStr());

    File clangFile;
    OovStatus status = clangFile.open(clangOutputFileName, "r");
    if(status.ok())
        {
        char buf[1000];
        bool startSearch = false;
        while(clangFile.getString(buf, sizeof(buf), status))
            {
            char *endLine = strchr(buf, '\n');
            if(endLine)
                {
                *endLine = '\0';
                }
            if(strstr(buf, "#include <...> search"))
                {
                startSearch = true;
                }
            if(strstr(buf, "End of search"))
                {
                startSearch = false;
                }
            if(startSearch && buf[0] == ' ' /*&& strstr(buf, "llvm")*/)
                {
                sysIncPaths.push_back(&buf[1]);
                }
            }
        clangFile.close();
        }
    if(status.ok())
        {
        status = FileDelete(clangOutputFileName);
        }
    if(status.needReport())
        { status.reported(); }
    return sysIncPaths;
    }

static OovStringVec getEnvPathDirs()
    {
    OovString pathStr = GetEnv("PATH");
    OovStringVec pathDirs;
    if(pathStr.length() > 0)
        {
        pathDirs = StringSplit(pathStr.getStr(), ':');
        }
    return pathDirs;
    }
#endif

char const *OptionsDefaults::getPlatform()
    {
#ifdef __linux__
    return OptFilterValuePlatformLinux;
#else
    return OptFilterValuePlatformWindows;
#endif
    }

void OptionsDefaults::setDefaultOptions()
    {
    CompoundValue baseArgs;
    CompoundValue extraCppDocArgs;
    CompoundValue extraCppRlsArgs;
    CompoundValue extraCppDbgArgs;
    CompoundValue extraLinuxArgs;

    mProject.clear();
    baseArgs.addArg("-c");
    bool useCLangBuild = false;
#ifdef __linux__
//    baseArgs.addArg("-ER/usr/include/gtk-3.0");
//    baseArgs.addArg("-ER/usr/lib/x86_64-linux-gnu/glib-2.0/include");
    OovStatus status(true, SC_File);
    for(auto const &dir : getEnvPathDirs())
        {
        FilePath clangPath(dir, FP_Dir);
        clangPath.appendFile("clang++");
        if(FileIsFileOnDisk(clangPath, status))
            { useCLangBuild = true; }
        }
    if(status.needReport())
        {
        status.reported();
        }
#else
    OovString path = GetEnv("PATH");
    if(path.find("LLVM") != std::string::npos)
        {
        useCLangBuild = true;
        }
/*
    // On Windows, GTK includes gmodule, glib, etc., so no reason to get mingw.
    bool success = addExternalReference(path, "gtk", baseArgs);
    if(!success)
        {
        addExternalReference(path, "mingw", baseArgs);
        }
    addExternalReference(path, "qt", baseArgs);
*/
#endif
    // The GNU compiler cannot have these, but the clang compiler requires them.
    // CLang 3.2 does not work on Windows well, so GNU is used on Windows for
    // building, but CLang is used for XMI parsing.
    // These are important for xmi parsing to show template relations
    if(useCLangBuild)
        {
        addCLangArgs(baseArgs);
        }
    else
        {
        addCLangArgs(extraCppDocArgs);
        extraCppDbgArgs.addArg("-std=c++0x");
        extraCppRlsArgs.addArg("-std=c++0x");
        }
#ifdef __linux__
    if(Project::getProjectDirectory().length() > 0)
        {
        OovStringVec sysIncPaths = getSystemIncludePaths(useCLangBuild);
        if(sysIncPaths.size())
            {
            for(auto const &incPath : sysIncPaths)
                {
                extraLinuxArgs.addArg("-isystem");
                extraLinuxArgs.addArg(incPath);
                }
            }
        else
            {
            // llvm/clang was not found. Standard headers may be missing for analysis.
            }
        }
#endif
    extraCppDbgArgs.addArg("-O0");
    extraCppDbgArgs.addArg("-g3");

    extraCppRlsArgs.addArg("-O3");

#ifdef __linux__
    mProject.setNameValue(OptExeDebuggerPath, "/usr/bin/gdb");
#else
    // This works without setting the full path.
    // The path could be /MinGW/bin, or could be /Program Files/mingw-builds/...
    mProject.setNameValue(OptExeDebuggerPath, "gdb.exe");
#endif

    BuildVariable buildVar;
    buildVar.setVarName(OptCppArgs);
    mProject.setNameValue(buildVar.getVarFilterName(), baseArgs.getAsString());

    setCppArgs(mProject, OptFilterNameBuildConfig, BuildConfigAnalysis,
        extraCppDocArgs.getAsString());
    setCppArgs(mProject, OptFilterNameBuildConfig, BuildConfigDebug,
        extraCppDbgArgs.getAsString());
    setCppArgs(mProject, OptFilterNameBuildConfig, BuildConfigRelease,
        extraCppRlsArgs.getAsString());
    setCppArgs(mProject, OptFilterNamePlatform, OptFilterValuePlatformLinux,
        extraLinuxArgs.getAsString());

    setBuildConfigurationPaths(mProject, useCLangBuild);

    OovString str = GetEnv("CLASSPATH");;
    if(str.length())
        {
        buildVar.setVarName(OptJavaClassPath);
        mProject.setNameValue(buildVar.getVarFilterName(), str);
        }
    str = GetEnv("JAVA_HOME");
    if(str.length() == 0)
        {
#ifdef __linux__
        str = "/usr/lib/jvm/default-java";
#endif
        }
    if(str.length())
        {
        buildVar.setVarName(OptJavaJdkPath);
        mProject.setNameValue(buildVar.getVarFilterName(), str);
        }
    }

void GuiOptions::setDefaultOptions()
    {
    setNameValueBool(OptGuiShowAttributes, true);
    setNameValueBool(OptGuiShowOperations, true);
    setNameValueBool(OptGuiShowOperParams, true);
    setNameValueBool(OptGuiShowAttrTypes, true);
    setNameValueBool(OptGuiShowOperTypes, true);
    setNameValueBool(OptGuiShowPackageName, true);
    setNameValueBool(OptGuiShowOovSymbols, true);
    setNameValueBool(OptGuiShowTemplateRelations, true);
    setNameValueBool(OptGuiShowOperParamRelations, true);
    setNameValueBool(OptGuiShowOperBodyVarRelations, true);
    setNameValueBool(OptGuiShowRelationKey, true);

    setNameValueBool(OptGuiShowCompImplicitRelations, false);
    /*
    #ifdef __linux__
        setNameValue(OptEditorPath, "/usr/bin/gedit");
    #else
        setNameValue(OptEditorPath, "\\Windows\\notepad.exe");
    #endif
    */
#ifdef __linux__
    setNameValue(OptGuiEditorPath, "./oovEdit");
#else
    setNameValue(OptGuiEditorPath, "oovEdit.exe");
#endif
    setNameValue(OptGuiEditorLineArg, "+");
    }

OovStatusReturn GuiOptions::read()
    {
    setFilename(Project::getGuiOptionsFilePath());
    OovStatus status = NameValueFile::readFile();
    if(status.ok())
        {
        setDefaultOptions();
        }
    return status;
    }
