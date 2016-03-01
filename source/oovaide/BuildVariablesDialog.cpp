/*
 * BuildVariablesDialog.cpp
 *
 *  Created on: Feb 9, 2016
 *  \copyright 2016 DCBlaha.  Distributed under the GPL.
 */

#include "BuildVariablesDialog.h"
#include "BuildVariables.h"
#include "Components.h"
#include <string.h>
#include <algorithm>

static BuildVariablesDialog *sBuildVarDialog;
static BuildVarSettingsDialog *sBuildVarSettingsDialog;

enum VarTypes
    {
    VT_CompType,
    VT_CppArgs,
    VT_CppCompilerPath,
    VT_CppLibPath,
    VT_ObjSymbolPath,

    VT_JavaArgs,
    VT_JavaAnalyzerPath,
    VT_JavaCompilerPath,
    VT_JavaJarPath,
    VT_JavaClassPath,
    VT_JavaJdkPath,

    VT_First = 0,
    VT_Last = VT_JavaJdkPath
    };

enum FilterTypes
    {
    FT_BuildConfig,
    FT_BuildMode,
    FT_Component,
    FT_Platform,
    };

static int BAD_INDEX = -1;


int StringIdVector::getIndex(OovStringRef val) const
    {
    int index = BAD_INDEX;
    auto typeIter = std::find_if(begin(), end(),
        [val](StringIdItem const &item)
        { return(strcmp(val, item.mUserString) == 0); });
    if(typeIter != end())
        {
        index = typeIter-begin();
        }
    return index;
    }

int StringIdVector::getIndex(int id) const
    {
    int index = BAD_INDEX;
    auto typeIter = std::find_if(begin(), end(),
        [id](StringIdItem const &item)
        { return(item.mId == id); });
    if(typeIter != end())
        {
        index = typeIter-begin();
        }
    return index;
    }

int StringIdVector::getId(OovStringRef val) const
    {
    int id = BAD_INDEX;
    int index = getIndex(val);
    if(index != BAD_INDEX)
        {
        id = (*this)[index].mId;
        }
    return id;
    }

static int getComboId(OovStringRef comboName, StringIdVector const &vec)
    {
    GtkComboBoxText *typeBox = GTK_COMBO_BOX_TEXT(
        Builder::getBuilder()->getWidget(comboName));
    return vec.getId(Gui::getText(typeBox));
    }

static void initComboBox(OovStringRef comboBoxName,
    std::vector<StringIdItem> &vec, int initialIndex)
    {
    GtkComboBoxText *box = GTK_COMBO_BOX_TEXT(
        Builder::getBuilder()->getWidget(comboBoxName));
    Gui::clear(box);
    for(auto const &filter : vec)
        {
        Gui::appendText(box, filter.mUserString);
        }
    Gui::setSelected(box, initialIndex);
    }

static void initVarTypes(std::vector<StringIdItem> &varTypes)
    {
    varTypes.clear();
    varTypes.push_back(StringIdItem("C++ Arguments", VT_CppArgs));
    varTypes.push_back(StringIdItem("Component Types", VT_CompType));
    varTypes.push_back(StringIdItem("C++ Compiler Path", VT_CppCompilerPath));
    varTypes.push_back(StringIdItem("C++ Library Path", VT_CppLibPath));
    varTypes.push_back(StringIdItem("Object Symbol Path", VT_ObjSymbolPath));
    varTypes.push_back(StringIdItem("Java Arguments", VT_JavaArgs));
    varTypes.push_back(StringIdItem("Java Analyzer Path", VT_JavaAnalyzerPath));
    varTypes.push_back(StringIdItem("Java Compiler Path", VT_JavaCompilerPath));
    varTypes.push_back(StringIdItem("Java Class Path", VT_JavaClassPath));
    varTypes.push_back(StringIdItem("Java Jar Path", VT_JavaJarPath));
    varTypes.push_back(StringIdItem("Java JDK Path", VT_JavaJdkPath));
    }

static char const *getVarTypeStr(VarTypes vt)
    {
    char const *varTypeStr = nullptr;
    switch(vt)
        {
        case VT_CppArgs:                varTypeStr = OptCppArgs;            break;
        case VT_CompType:               varTypeStr = OptCompType;           break;
        case VT_CppCompilerPath:        varTypeStr = OptCppCompilerPath;    break;
        case VT_CppLibPath:             varTypeStr = OptCppLibPath;         break;
        case VT_ObjSymbolPath:          varTypeStr = OptObjSymbolPath;      break;
        case VT_JavaArgs:               varTypeStr = OptJavaArgs;           break;
        case VT_JavaAnalyzerPath:       varTypeStr = OptJavaAnalyzerPath;   break;
        case VT_JavaCompilerPath:       varTypeStr = OptJavaCompilerPath;   break;
        case VT_JavaClassPath:          varTypeStr = OptJavaClassPath;      break;
        case VT_JavaJarPath:            varTypeStr = OptJavaJarPath;        break;
        case VT_JavaJdkPath:            varTypeStr = OptJavaJdkPath;        break;
        }
    return varTypeStr;
    }

static VarTypes getVarTypeFromStr(OovStringRef str)
    {
    VarTypes vt = VT_First;
    for(int i=VT_First; i<=VT_Last; i++)
        {
        if(strcmp(getVarTypeStr(static_cast<VarTypes>(i)), str) == 0)
            {
            vt = static_cast<VarTypes>(i);
            break;
            }
        }
    return vt;
    }

//////////

BuildVarSettingsDialog::BuildVarSettingsDialog(ProjectReader &project, GtkWindow *parentWnd):
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("BuildVariableSettingDialog")),
        parentWnd)
    {
    sBuildVarSettingsDialog = this;
    mFilterValuesTreeView.init(*Builder::getBuilder(), "FilterValuesTreeview",
        "Filter Settings");
    mFiltersTreeView.init(*Builder::getBuilder(), "BuildVarFiltersTreeview",
        "Filters");
    mBuildConfigs.parseString(project.getValue(OptBuildConfigs));
    }

bool BuildVarSettingsDialog::editVariable(OovString &varStr)
    {
    mVarStr = varStr;
    bool ok = run(true);
    if(ok)
        {
        varStr = Gui::getText(GTK_ENTRY(Builder::getBuilder()->getWidget(
            "BuildVarSettingEntry")));
        }
    return ok;
    }

// Much of the initialization is performed here to reduce the memory usage
// of the dialog when it is constructed, and is only allocated when the
// dialog is displayed.
void BuildVarSettingsDialog::beforeRun()
    {
    mFilterNames.clear();
    mFilterNames.push_back(StringIdItem("Build Configuration", FT_BuildConfig));
    mFilterNames.push_back(StringIdItem("Build Mode", FT_BuildMode));
    mFilterNames.push_back(StringIdItem("Component", FT_Component));
    mFilterNames.push_back(StringIdItem("Platform", FT_Platform));
    initComboBox("FilterNameComboboxtext", mFilterNames, 0);

    // Initialize things from the build variable string.
    BuildVariable buildVar;
    buildVar.initVarFromString(mVarStr);

    initVarTypes(mVarTypes);
    int index = mVarTypes.getIndex(getVarTypeFromStr(buildVar.getVarName()));
    initComboBox("BuildVarTypeComboboxtext", mVarTypes, index);

    mFunctions.clear();
    mFunctions.push_back(StringIdItem("Append", BuildVariable::F_Append));
    mFunctions.push_back(StringIdItem("Assign", BuildVariable::F_Assign));
    mFunctions.push_back(StringIdItem("Insert", BuildVariable::F_Insert));
    index = mFunctions.getIndex(buildVar.getFunction());
    initComboBox("BuildVarFunctionComboboxtext", mFunctions, index);

    mFiltersTreeView.clear();
    GuiTreeItem parent;
    for(auto const &filter : buildVar.getVarFilterList())
        {
        mFiltersTreeView.appendText(parent, filter.getFilterAsString());
        }

    Gui::setText(
        GTK_ENTRY(Builder::getBuilder()->getWidget("BuildVarValueEntry")),
        buildVar.getVarValue());

    updateSettingText();
    }

void BuildVarSettingsDialog::updateFilterValueList()
    {
    int index = getComboId("FilterNameComboboxtext", mFilterNames);
    if(index != BAD_INDEX)
        {
        mFilterValuesTreeView.clear();
        GuiTreeItem parent;
        switch(index)
            {
            case FT_BuildConfig:
                mFilterValuesTreeView.appendText(parent, BuildConfigAnalysis);
                mFilterValuesTreeView.appendText(parent, BuildConfigDebug);
                mFilterValuesTreeView.appendText(parent, BuildConfigRelease);
                for(auto const &cfgName : mBuildConfigs)
                    {
                    mFilterValuesTreeView.appendText(parent, cfgName);
                    }
                break;

            case FT_BuildMode:
                mFilterValuesTreeView.appendText(parent, OptFilterValueBuildModeAnalyze);
                mFilterValuesTreeView.appendText(parent, OptFilterValueBuildModeBuild);
                break;

            case FT_Component:
                {
                ScannedComponentInfo scannedCompInfo;
                OovStatus status = scannedCompInfo.readScannedInfo();
                for(auto const &name : scannedCompInfo.getComponentNames())
                    {
                    mFilterValuesTreeView.appendText(parent, name);
                    }
                }
                break;

            case FT_Platform:
                mFilterValuesTreeView.appendText(parent, OptFilterValuePlatformLinux);
                mFilterValuesTreeView.appendText(parent, OptFilterValuePlatformWindows);
                break;
            }
        }
    }

void BuildVarSettingsDialog::addFilterList()
    {
    GuiTreeItem parent;
    OovString filterTag;
    int index = getComboId("FilterNameComboboxtext", mFilterNames);
    if(index != BAD_INDEX)
        {
        switch(index)
            {
            case FT_BuildConfig:
                filterTag = OptFilterNameBuildConfig;
                break;

            case FT_BuildMode:
                filterTag = OptFilterNameBuildMode;
                break;

            case FT_Component:
                filterTag = OptFilterNameComponent;
                break;

            case FT_Platform:
                filterTag = OptFilterNamePlatform;
                break;
            }
        }
    OovStringVec filterVals = mFilterValuesTreeView.getSelected();
    if(filterVals.size() > 0)
        {
        VariableFilter filter(filterTag, filterVals[0]);
        mFiltersTreeView.appendText(parent, filter.getFilterAsString());
        }
    updateSettingText();
    }

void BuildVarSettingsDialog::removeFilterList()
    {
    mFiltersTreeView.removeSelected();
    updateSettingText();
    }

void BuildVarSettingsDialog::updateSettingText()
    {
    GuiTreeItem parent;
    BuildVariable buildVar;
    int index = getComboId("BuildVarTypeComboboxtext", mVarTypes);
    if(index != BAD_INDEX)
        {
        buildVar.setVarName(getVarTypeStr(static_cast<VarTypes>(index)));
        }
    for(int i=0; i<mFiltersTreeView.getNumChildren(parent); i++)
        {
        GuiTreeItem child(false);
        if(mFiltersTreeView.getNthChild(parent, i, child))
            {
            VariableFilter filter;
            filter.initFilterFromString(mFiltersTreeView.getText(child));
            buildVar.addFilter(filter);
            }
        }
    index = getComboId("BuildVarFunctionComboboxtext", mFunctions);
    if(index != BAD_INDEX)
        {
        buildVar.setFunction(static_cast<BuildVariable::eFunctions>(index));
        }
    buildVar.setVarValue(Gui::getText(
        GTK_ENTRY(Builder::getBuilder()->getWidget("BuildVarValueEntry"))));
    Gui::setText(GTK_ENTRY(Builder::getBuilder()->getWidget("BuildVarSettingEntry")),
        buildVar.getVarDefinition());
    }


extern "C" G_MODULE_EXPORT void on_FilterNameComboboxtext_changed(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->updateFilterValueList();
    }

extern "C" G_MODULE_EXPORT void on_BuildVarAddFilterButton_clicked(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->addFilterList();
    }

extern "C" G_MODULE_EXPORT void on_BuildVarRemoveFilterButton_clicked(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->removeFilterList();
    }

extern "C" G_MODULE_EXPORT void on_BuildVarFunctionComboboxtext_changed(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->updateSettingText();
    }

extern "C" G_MODULE_EXPORT void on_BuildVarTypeComboboxtext_changed(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->updateSettingText();
    }

extern "C" G_MODULE_EXPORT void on_BuildVarValueEntry_changed(
    GtkWidget *button, gpointer data)
    {
    sBuildVarSettingsDialog->updateSettingText();
    }

//////////

BuildVariablesDialog::BuildVariablesDialog(ProjectReader &project,
    GtkWindow *parentWnd):
    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("BuildVariablesDialog")),
        parentWnd),
    mProject(project),
    mBuildVarSettingsDialog(project, GTK_WINDOW(
        Builder::getBuilder()->getWidget("BuildVariablesDialog")))
    {
    sBuildVarDialog = this;
    mVarListTreeView.init(*Builder::getBuilder(), "BuildVariablesTreeview",
        "Variable Settings");
    }

// Much of the initialization is performed here to reduce the memory usage
// of the dialog when it is constructed, and is only allocated when the
// dialog is displayed.
void BuildVariablesDialog::beforeRun()
    {
    initVarTypes(mVarTypes);
    initComboBox("VariableTypeComboboxtext", mVarTypes, 0);
    updateVarList();
    }

void BuildVariablesDialog::updateVarList()
    {
    int index = getComboId("VariableTypeComboboxtext", mVarTypes);
    if(index != BAD_INDEX)
        {
        OovStringVec vars = mProject.getMatchingNames(getVarTypeStr(
            static_cast<VarTypes>(index)));
        mVarListTreeView.clear();
        GuiTreeItem parent;
        for(auto const &str : vars)
            {
            BuildVariable buildVar;
            OovString val = mProject.getValue(str);
            buildVar.initVarFromString(str, val);
            mVarListTreeView.appendText(parent, buildVar.getVarDefinition());
            }
        }
    }

void BuildVariablesDialog::editVariable()
    {
    OovStringVec vars = mVarListTreeView.getSelected();
    if(vars.size() > 0)
        {
        OovString var = vars[0];
        if(sBuildVarSettingsDialog->editVariable(var))
            {
            mVarListTreeView.removeSelected();
            GuiTreeItem parent;
            mVarListTreeView.appendText(parent, var);

            BuildVariable origBuildVar;
            origBuildVar.initVarFromString(vars[0]);
            mProject.removeName(origBuildVar.getVarFilterName());

            BuildVariable newBuildVar;
            newBuildVar.initVarFromString(var);
            mProject.setNameValue(newBuildVar.getVarFilterName(), newBuildVar.getVarValue());
            }
        }
    }

void BuildVariablesDialog::removeVariableSetting()
    {
    OovStringVec vars = mVarListTreeView.getSelected();
    if(vars.size() > 0)
        {
        BuildVariable origBuildVar;
        origBuildVar.initVarFromString(vars[0]);
        mVarListTreeView.removeSelected();
        mProject.removeName(origBuildVar.getVarFilterName());
        }
    }

void BuildVariablesDialog::addVariableSetting()
    {
    GuiTreeItem parent;
    BuildVariable buildVar;
    buildVar.setVarName(OptCppArgs);
    OovString var = buildVar.getVarDefinition();
    mVarListTreeView.appendText(parent, var);

    BuildVariable newBuildVar;
    newBuildVar.initVarFromString(var);
    mProject.setNameValue(newBuildVar.getVarFilterName(), newBuildVar.getVarValue());
    }

bool BuildVariablesDialog::runAdvancedDialog()
    {
    return sBuildVarDialog->run(true);
    }

extern "C" G_MODULE_EXPORT void on_VariableTypeComboboxtext_changed(
    GtkWidget *button, gpointer data)
    {
    sBuildVarDialog->updateVarList();
    }

extern "C" G_MODULE_EXPORT void on_VariableRemoveButton_clicked(
    GtkWidget *button, gpointer data)
    {
    sBuildVarDialog->removeVariableSetting();
    }

extern "C" G_MODULE_EXPORT void on_VariableEditButton_clicked(
    GtkWidget *button, gpointer data)
    {
    sBuildVarDialog->editVariable();
    }

extern "C" G_MODULE_EXPORT void on_VariableAddButton_clicked(
    GtkWidget *button, gpointer data)
    {
    sBuildVarDialog->addVariableSetting();
    }
