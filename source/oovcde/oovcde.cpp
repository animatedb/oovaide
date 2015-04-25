//============================================================================
// Name        : oovcde.cpp
// \copyright 2013 DCBlaha.  Distributed under the GPL.
//============================================================================

#include "oovcde.h"
#include "Options.h"
#include "OptionsDialog.h"
#include "ProjectSettingsDialog.h"
#include "BuildSettingsDialog.h"
#include "BuildConfigReader.h"
#include "Svg.h"
#include "Gui.h"
#include "OovString.h"
#include "File.h"
#include "Version.h"
#include "Complexity.h"
#include "Duplicates.h"
#include "StaticAnalysis.h"
#include <stdlib.h>


static oovGui gOovGui;
static OptionsDialog *sOptionsDialog;


bool WindowBuildListener::onBackgroundProcessIdle(bool &complete)
    {
    bool didSomething = false;
    bool appended = false;
    std::string tempStdStr;
	{
	LockGuard guard(mMutex);
	if(mStdOutAndErr.length())
	    {
	    tempStdStr = mStdOutAndErr;
	    mStdOutAndErr.clear();
	    didSomething = true;
	    appended = true;
	    }
	}
    bool atEnd = GuiTextBuffer::isCursorAtEnd(GuiTextBuffer::getBuffer(mStatusTextView));
    Gui::appendText(mStatusTextView, tempStdStr);
    complete = mComplete;
    if(mComplete)
	{
	Gui::appendText(mStatusTextView, "\nComplete\n");
	appended = true;
	mComplete = false;
	}
    if(atEnd)
	GuiTextBuffer::moveCursorToEnd(GuiTextBuffer::getBuffer(mStatusTextView));
    if(appended)
	{
	Gui::scrollToCursor(mStatusTextView);
	}
    return didSomething;
    }

void WindowBuildListener::processComplete()
    {
    mComplete = true;
    }



void oovGui::init()
    {
    mProject.setBackgroundProcessListener(&mWindowBuildListener);
    mProject.setStatusListener(&mProjectStatusListener);
    mWindowBuildListener.initListener(mBuilder);
    mBuilder.connectSignals();
    mClassList.init(mBuilder);
    mComponentList.init(mBuilder, "ModuleTreeview", "Module List");
    mOperationList.init(mBuilder);
    mZoneList.init();
    mJournalList.init(mBuilder);
    mJournal.init(mBuilder, mProject.getModelData(), *this, mProjectStatusListener);
    g_idle_add(onIdle, this);
    updateMenuEnables(ProjectStatus());
    }

oovGui::~oovGui()
    {
    mProject.stopAndWaitForBackgroundComplete();
    mJournal.stopAndWaitForBackgroundComplete();
    clearAnalysis();
    g_idle_remove_by_data(this);
    }

void oovGui::clearAnalysis()
    {
    mProject.clearAnalysis();
    mJournal.clear();
    mClassList.clear();
    mComponentList.clear();
    clearSelectedComponent();
    mJournalList.clear();
    mOperationList.clear();
    mZoneList.clear();
    updateGuiForProjectChange();
    }

bool oovGui::canStartAnalysis()
    {
    bool success = mProject.isProjectIdle();
    if(!success)
    	{
    	success = Gui::messageBox("Do you want to stop the current analysis?",
    	    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO);
    	if(success)
    	    {
    	    getProject().stopAndWaitForBackgroundComplete();
    	    }
    	}
    if(success)
	{
	clearAnalysis();
	}
    return success;
    }

void oovGui::updateGuiForProjectChange()
    {
    ProjectStatus projStat = mProject.getProjectStatus();
    if(projStat != getLastProjectStatus())
	{
	updateMenuEnables(projStat);
	if(projStat.mAnalysisStatus != mLastProjectStatus.mAnalysisStatus)
	    {
	    mLastProjectStatus.mAnalysisStatus = projStat.mAnalysisStatus;
	    if(projStat.mAnalysisStatus & ProjectStatus::AS_Loaded)
		{
		updateGuiForAnalysis();
		}
	    }
	mLastProjectStatus = projStat;
	}
    }

void oovGui::updateGuiForAnalysis()
    {
    if(gBuildOptions.read())
	{
	gGuiOptions.read();
	ModelData &modelData = mProject.getModelData();
	mClassList.clear();
	for(size_t i=0; i<modelData.mTypes.size(); i++)
	    {
	    if(modelData.mTypes[i]->getDataType() == DT_Class)
		mClassList.appendText(modelData.mTypes[i]->getName());
	    }
	gOovGui.updateComponentList();
	mClassList.sort();
	if(sOptionsDialog)
	    sOptionsDialog->updateBuildConfig();
	mZoneList.update();
	Gui::setCurrentPage(GTK_NOTEBOOK(mBuilder.getWidget("ListNotebook")), 0);
	}
    }

gboolean oovGui::onBackgroundIdle(gpointer data)
    {
    bool complete;
    bool didSomething = mWindowBuildListener.onBackgroundProcessIdle(complete);
    if(complete)
	{
	clearAnalysis();
	updateComponentList();
	mProject.loadAnalysisFiles();
	didSomething = true;
	}
    // This must be after running analysis functions.
    updateGuiForProjectChange();
    mProjectStatusListener.idleUpdateProgress();
    if(!didSomething)
	{
	sleepMs(5);
	}
    return true;
    }

void oovGui::updateMenuEnables(ProjectStatus const &projStat)
    {
    bool idle = projStat.isIdle();
    bool open = projStat.mProjectOpen;
    bool analRdy = projStat.isAnalysisReady();

    Builder *builder = Builder::getBuilder();

    gtk_widget_set_sensitive(
	    builder->getWidget("NewModuleMenuitem"), open);
    gtk_widget_set_sensitive(
	    builder->getWidget("SaveDrawingMenuitem"), open);
    gtk_widget_set_sensitive(
	    builder->getWidget("SaveDrawingAsMenuitem"), open);

    gtk_widget_set_sensitive(
	    builder->getWidget("EditOptionsmenuitem"), open);
    gtk_widget_set_sensitive(
    	    builder->getWidget("ProjectSettingsMenuitem"), open);
    gtk_widget_set_sensitive(
	    builder->getWidget("BuildAnalyzeMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
	    builder->getWidget("StopAnalyzeMenuitem"), !idle);
    gtk_widget_set_sensitive(
	    builder->getWidget("ComplexityMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
	    builder->getWidget("MemberUsageMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
	    builder->getWidget("DuplicatesMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
	    builder->getWidget("ProjectStatsMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
	    builder->getWidget("LineStatsMenuitem"), idle && analRdy);

    gtk_widget_set_sensitive(
	    builder->getWidget("BuildSettingsMenuitem"), open);
    gtk_widget_set_sensitive(
	    builder->getWidget("BuildDebugMenuitem"), idle && open);
    gtk_widget_set_sensitive(
	    builder->getWidget("BuildReleaseMenuitem"), idle && open);
    gtk_widget_set_sensitive(
	    builder->getWidget("StopBuildMenuitem"), !idle);
    gtk_widget_set_sensitive(
	    builder->getWidget("MakeCMakeMenuitem"), open);

    gtk_widget_set_sensitive(
	    builder->getWidget("InstrumentMenuitem"), idle && open);
    gtk_widget_set_sensitive(
	    builder->getWidget("CoverageBuildMenuitem"), idle && open);
    gtk_widget_set_sensitive(
	    builder->getWidget("CoverageStatsMenuitem"), idle && open);
    gtk_widget_set_sensitive(
	    builder->getWidget("StopInstrumentMenuitem"), !idle);
    }

void oovGui::displayClass(OovStringRef const className)
    {
    // While the graph is initialized, there is no name.
    if(std::string(className).length() > 0)
	{
	updateClassList(className);
	updateOperationList(mProject.getModelData(), className);
	mJournal.displayClass(className);
	updateJournalList();
	}
    }

void oovGui::addClass(OovStringRef const className)
    {
    mJournal.addClass(className);
    }

void oovGui::displayOperation(OovStringRef const className,
	OovStringRef const operName, bool isConst)
    {
    // While the graph is initialized, there is no name.
    if(std::string(className).length() > 0)
	{
	if(std::string(operName).length() > 0)
	    {
	    mJournal.displayOperation(className, operName, isConst);
	    updateJournalList();
	    }
	}
    }

static bool checkAnyComponents()
    {
    ComponentTypesFile compFile;
    compFile.read();
    bool success = compFile.anyComponentsDefined();
    if(!success)
	{
	Gui::messageBox("Define some components in Build/Settings",
		GTK_MESSAGE_INFO);
	}
    return success;
    }

void oovGui::runSrcManager(OovStringRef const buildConfigName,
	OovProject::eSrcManagerOptions smo)
    {
    mWindowBuildListener.clearStatusTextView(getBuilder());
    bool success = true;
    char const *str = nullptr;
    switch(smo)
	{
	case OovProject::SM_Analyze:
	    str = "\nAnalyzing\n";
	    break;

	case OovProject::SM_Build:
	    str = "\nBuilding\n";
	    success = checkAnyComponents();
	    break;

	case OovProject::SM_CovInstr:
	    str = "\nInstrumenting\n";
	    success = checkAnyComponents();
	    break;

	case OovProject::SM_CovBuild:
	    str = "\nBuilding coverage\n";
	    break;

	case OovProject::SM_CovStats:
	    str = "\nGenerating coverage statistics\n";
	    break;

	default:
	    break;
	}
    if(success)
	{
	mWindowBuildListener.onStdOut(str, std::string(str).length());
	mProject.runSrcManager(buildConfigName, str, smo);
	}
    updateMenuEnables(mProject.getProjectStatus());
    }

std::string oovGui::getDefaultDiagramName()
    {
    const JournalRecord *rec = mJournal.getCurrentRecord();
    std::string fn;
    if(rec)
	{
	fn += rec->getFullName(false);
	fn += ".svg";
	}
    else
	fn = "Unknown.svg";
    return fn;
    }

void oovGui::updateJournalList()
    {
    mJournalList.clear();
    for(const auto &rec : mJournal.getRecords())
	{
	mJournalList.appendText(rec->getFullName(true));
	}
    }

void oovGui::updateClassList(OovStringRef const className)
    {
    mClassList.setSelected(className);
    }

void oovGui::updateOperationList(const ModelData &modelData,
	OovStringRef const className)
    {
    mOperationList.clear();
    const ModelClassifier *cls = modelData.getTypeRef(className)->getClass();
    if(cls)
	{
	for(size_t i=0; i<cls->getOperations().size(); i++)
	    {
	    const ModelOperation *oper = cls->getOperations()[i].get();
	    std::string opStr = oper->getName();
	    if(oper->isConst())
		{
		opStr += ' ';
		opStr += "const";
		}
	    mOperationList.appendText(opStr);
	    }
	mOperationList.sort();
	}
    }

int oovGui::getStatusSourceFile(std::string &fn)
    {
    GtkWidget *widget = getBuilder().getWidget("StatusTextview");
    GtkTextView *textView = GTK_TEXT_VIEW(widget);
    int lineNum = 0;
    std::string line = Gui::getCurrentLineText(textView);
    size_t pos = 0;
    while(pos != std::string::npos)
	{
	pos = line.find(':', pos);
	if(pos != std::string::npos)
	    {
	    if(pos+1 < line.length())
		pos++;
	    else
		break;
	    if(isdigit(line[pos]))
		{
		break;
		}
	    }
	}
    if(pos != std::string::npos)
	{
	size_t endPos = line.find(':', pos+1);
	if(endPos != std::string::npos)
	    {
	    OovString numStr(line.substr(pos, endPos-pos));
	    if(numStr.getInt(0, INT_MAX, lineNum))
		fn = line.substr(0, pos-1);
	    }
	}
    return lineNum;
    }

static void displayBrowserFile(OovStringRef const fileName)
    {
    char const *fn = fileName;
#ifdef __linux__
    FilePath fpTest(fn, FP_File);
    if(!FileIsFileOnDisk(fpTest))
#else
    if(!FileIsFileOnDisk(fn))
#endif
	{
	fn = "..\\web\\userguide\\oovcdeuserguide.shtml";
	}
#ifdef __linux__
    pid_t pid=fork();
    if(!pid)
	{
	FilePath fp(fn, FP_File);
	char const *prog = "/usr/bin/xdg-open";
	char const *args[3];
	args[0] = prog;
	args[1] = fp.c_str();
	args[2] = nullptr;
//printf("FF %s\n", fp.c_str());
        execvp(prog, const_cast<char**>(args));
	}
#else
    ShellExecute(NULL, "open", fn, NULL, NULL, SW_SHOWNORMAL);
#endif
    }

void oovGui::makeComplexityFile()
    {
    std::string fn;
    if(createComplexityFile(mProject.getModelData(), fn))
	{
	displayBrowserFile(fn);
	}
    else
	{
	OovString str = "Unable to write " + fn + " to project directory";
	Gui::messageBox(str);
	}
    }

void oovGui::makeMemberUseFile()
    {
    std::string fn;
    if(createStaticAnalysisFile(mProject.getModelData(), fn))
	{
	displayBrowserFile(fn);
	}
    else
	{
	OovString str = "Unable to write " + fn + " to project directory";
	Gui::messageBox(str);
	}
    }

void oovGui::makeLineStats()
    {
    std::string fn;
    if(createLineStatsFile(mProject.getModelData(), fn))
	{
	displayBrowserFile(fn);
	}
    else
	{
	OovString str = "Unable to write " + fn + " to project directory";
	Gui::messageBox(str);
	}
    }

void oovGui::makeDuplicatesFile()
    {
    std::string fn;
    eDupReturn ret = createDuplicatesFile(fn);
    if(ret == DR_Success)
	{
	displayBrowserFile(fn);
	}
    else if(ret == DR_NoDupFilesFound)
	{
	Gui::messageBox("Unable to find dup files. Delete the analysis "
		"directory, and use -dups in "
		"Analysis/Settings/Build Arguments/Analyze/Extra Build Arguments");
	}
    else
	{
	Gui::messageBox("Unable to write file to project output directory");
	}
    }

void oovGui::displayProjectStats()
    {
    OovString str;
    if(createProjectStats(mProject.getModelData(), str))
	{
	Gui::messageBox(str, GTK_MESSAGE_INFO);
	}
    else
	{
	Gui::messageBox("Unable get project statistics");
	}
    }

class OptionsDialogUpdate:public OptionsDialog
    {
    virtual void updateOptions() override
	{
	gOovGui.getJournal().cppArgOptionsChangedUpdateDrawings();
	gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
	}
    virtual void buildConfig(OovStringRef const name) override
	{
	gOovGui.runSrcManager(name, OovProject::SM_Build);
	}
    };

int main(int argc, char *argv[])
    {
    gtk_init(&argc, &argv);
    if(gOovGui.getBuilder().addFromFile("oovcdeLayout.glade"))
	{
	gOovGui.init();
	OptionsDialogUpdate optionsDlg;
	sOptionsDialog = &optionsDlg;
	BuildSettingsDialog buildDlg;

	GtkWidget *window = gOovGui.getBuilder().getWidget("TopWindow");
	gtk_widget_show(window);
	gtk_main();
	}
    else
	{
	Gui::messageBox("The file oovcdeLayout.glade must be in the executable directory.");
	}
    return 0;
    }

extern "C" G_MODULE_EXPORT void on_NewProjectMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    ProjectSettingsDialog dlg(gOovGui.getWindow(), true);
    if(dlg.runDialog())
	{
	if(gOovGui.canStartAnalysis())
	    {
	    OovProject::eNewProjectStatus projStatus;
	    if(gOovGui.getProject().newProject(
		    dlg.getProjectDir(), dlg.getExcludeDirs(), projStatus))
		{
		switch(projStatus)
		    {
		    case OovProject::NP_CreatedProject:
			gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
			gtk_widget_hide(gOovGui.getBuilder().getWidget("NewProjectDialog"));
			break;

		    case OovProject::NP_CantCreateDir:
			Gui::messageBox("Unable to create project directory");
			break;

		    case OovProject::NP_CantCreateFile:
			Gui::messageBox("Unable to write project file");
			break;
		    }
		}
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_OpenProjectMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    OovString projectDir;
    PathChooser ch;
    if(ch.ChoosePath(gOovGui.getWindow(), "Open OOVCDE Project Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, projectDir))
	{
	if(gOovGui.canStartAnalysis())
	    {
	    gGuiOptions.read();	// Make editor available for component list.
	    bool openedProject = false;
	    if(gOovGui.getProject().openProject(projectDir, openedProject))
		{
		if(openedProject)
		    {
		    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
		    }
		else
		    {
		    Gui::messageBox("Unable to open project file");
		    }
		}
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ProjectSettingsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ProjectSettingsDialog dlg(gOovGui.getWindow(), false);
    if(dlg.runDialog())
	{
	if(gOovGui.canStartAnalysis())
	    {
	    gBuildOptions.setNameValue(OptProjectExcludeDirs,
		    dlg.getExcludeDirs().getAsString(';'));
	    gBuildOptions.writeFile();
	    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
	    }
	}
    }

////// New Module Dialog ////////////

extern "C" G_MODULE_EXPORT void on_NewModule_ModuleEntry_changed(
	GtkWidget *widget, gpointer data)
    {
    OovString module = Gui::getText(GTK_ENTRY(widget));
    OovString interfaceName = module + ".h";
    OovString implementationName = module + ".cpp";
    Gui::setText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_InterfaceEntry")), interfaceName);
    Gui::setText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_ImplementationEntry")), implementationName);
    }

// In glade, set the callback for the dialog's GtkWidget "delete-event" to
// gtk_widget_hide_on_delete for the title bar close button to work.
extern "C" G_MODULE_EXPORT void on_NewModuleCancelButton_clicked(
	GtkWidget *widget, gpointer data)
    {
    gtk_widget_hide(gOovGui.getBuilder().getWidget("NewModuleDialog"));
    }

extern "C" G_MODULE_EXPORT void on_NewModuleOkButton_clicked(
	GtkWidget *widget, gpointer data)
    {
    gtk_widget_hide(gOovGui.getBuilder().getWidget("NewModuleDialog"));
    OovString basePath = Project::getSrcRootDirectory();
    OovString interfaceName = Gui::getText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_InterfaceEntry")));
    OovString implementationName = Gui::getText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_ImplementationEntry")));
    OovString compName = Gui::getText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
	    "NewModule_ComponentComboboxtext")));

    FilePath compDir(basePath, FP_Dir);
    if(compName != Project::getRootComponentName())
	compDir.appendDir(compName);
    // Create the new component directory if it doesn't exist.
    FileEnsurePathExists(compDir);

    if(!FileIsFileOnDisk(interfaceName))
	{
	FilePath tempInt(compDir, FP_Dir);
	tempInt.appendFile(interfaceName);
	File intFile(tempInt, "w");
	fprintf(intFile.getFp(), "// %s", interfaceName.c_str());
	}
    else
	Gui::messageBox("Interface already exists", GTK_MESSAGE_INFO);

    if(!FileIsFileOnDisk(implementationName))
	{
	FilePath tempImp(compDir, FP_Dir);
	tempImp.appendFile(implementationName);
	File impFile(tempImp, "w");
	fprintf(impFile.getFp(), "// %s", implementationName.c_str());
	impFile.close();
	viewSource(tempImp, 1);
	}
    else
	Gui::messageBox("Implementation already exists", GTK_MESSAGE_INFO);

    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
    }

extern "C" G_MODULE_EXPORT void on_NewModuleMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    Dialog dlg(GTK_DIALOG(gOovGui.getBuilder().getWidget("NewModuleDialog")),
	    GTK_WINDOW(Builder::getBuilder()->getWidget("MainWindow")));
    ComponentTypesFile componentsFile;
    Gui::clear(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
	    "NewModule_ComponentComboboxtext")));
    if(componentsFile.read())
	{
	OovStringVec names = componentsFile.getComponentNames();
	if(names.size() == 0)
	    {
	    Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
		    "NewModule_ComponentComboboxtext")), Project::getRootComponentName());
	    }
	for(auto const &name : names)
	    {
	    Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
		    "NewModule_ComponentComboboxtext")), name);
	    }
	Gui::setSelected(GTK_COMBO_BOX(gOovGui.getBuilder().getWidget(
		"NewModule_ComponentComboboxtext")), 0);
	}
    dlg.run();
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    OovString fn = gOovGui.getLastSavedPath();
    SvgWriter svg(fn);
    gOovGui.getJournal().saveFile(svg.getFile());
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingAsMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    OovString fn = gOovGui.getLastSavedPath();
    ch.setDefaultPath(fn);
    if(ch.ChoosePath(gOovGui.getWindow(), "Save Drawing (.SVG)",
	    GTK_FILE_CHOOSER_ACTION_SAVE, fn))
	{
	gOovGui.setLastSavedPath(fn);
	SvgWriter svg(fn);
	gOovGui.getJournal().saveFile(svg.getFile());
	}
    }

extern "C" G_MODULE_EXPORT void on_BuildAnalyzeMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
    }

extern "C" G_MODULE_EXPORT void on_ComplexityMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.makeComplexityFile();
    }

extern "C" G_MODULE_EXPORT void on_MemberUsageMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.makeMemberUseFile();
    }

extern "C" G_MODULE_EXPORT void on_DuplicatesMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.makeDuplicatesFile();
    }

extern "C" G_MODULE_EXPORT void on_BuildDebugMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigDebug, OovProject::SM_Build);
    }

extern "C" G_MODULE_EXPORT void on_BuildReleaseMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigRelease, OovProject::SM_Build);
    }

extern "C" G_MODULE_EXPORT void on_StopBuildMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.stopSrcManager();
    }

extern "C" G_MODULE_EXPORT void on_InstrumentMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_CovInstr);
    }

extern "C" G_MODULE_EXPORT void on_CoverageBuildMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_CovBuild);
    }

extern "C" G_MODULE_EXPORT void on_CoverageStatsMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigAnalysis, OovProject::SM_CovStats);
    }

extern "C" G_MODULE_EXPORT void on_MakeCMakeMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    OovProcessChildArgs args;
    OovString proc = FilePathMakeExeFilename("./oovCMaker");

    OovString projNameArg;
    Dialog cmakeDlg(GTK_DIALOG(gOovGui.getBuilder().getWidget("CMakeDialog")));
    if(cmakeDlg.run(true))
	{
	GtkEntry *entry = GTK_ENTRY(gOovGui.getBuilder().getWidget("CMakeProjectNameEntry"));
	projNameArg = OovString("-n") + OovString(Gui::getText(entry));
	}

    bool putInSource = Gui::messageBox("Put files in source directory?",
	    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO);
    args.addArg(proc);
    args.addArg(Project::getProjectDirectory());
    if(putInSource)
	args.addArg("-w");
    args.addArg(projNameArg);
    spawnNoWait(proc, args.getArgv());
    std::string stat = std::string("Building CMake files for ") +
	    Project::getProjectDirectory() + ".";
    if(putInSource)
	stat += "\nCheck the source directory for CMake files.";
    else
	stat += "\nCheck the oovcde directory for CMake files.";
    Gui::messageBox(stat, GTK_MESSAGE_INFO);
    }

static bool sDisplayClassViewRightClick = false;

extern "C" G_MODULE_EXPORT void on_ClassTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    if(gOovGui.getProject().isAnalysisReady())
	{
	std::string className = gOovGui.getSelectedClass();
	if(sDisplayClassViewRightClick)
	    {
	    gOovGui.addClass(className);
	    sDisplayClassViewRightClick = false;
	    }
	else
	    {
	    gOovGui.displayClass(className);
	    }
	}
    }

extern "C" G_MODULE_EXPORT bool on_ClassTreeview_button_press_event(
	GtkWidget *button, GdkEvent *event, gpointer data)
    {
//    GtkTreeView *view = GTK_TREE_VIEW(mGui.getBuilder().getWidget("ClassTreeView"));
    GdkEventButton *eventBut = reinterpret_cast<GdkEventButton*>(event);
    if(eventBut->button == 3)	// Right button
	{
	sDisplayClassViewRightClick = true;
	}
    return false;	// Not handled - continue processing
    }

extern "C" G_MODULE_EXPORT void on_OperationsTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    std::string className = gOovGui.getSelectedClass();;
    std::string operName = gOovGui.getSelectedOperation();
    size_t spacePos = operName.find(' ');
    bool isConst = false;
    if(spacePos != std::string::npos)
	{
	operName.resize(spacePos);
	isConst = true;
	}
    gOovGui.displayOperation(className, operName, isConst);
    }

class NotebookHandler
    {
    public:
	enum ePageIndices { PI_Component, PI_Zone, PI_Class, PI_Portion, PI_Seq, PI_Journal };
	int mCurrentPage;
    };

NotebookHandler gNotebook;

extern "C" G_MODULE_EXPORT void on_JournalTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    if(gNotebook.mCurrentPage == NotebookHandler::PI_Journal)
	{
	gOovGui.getJournal().setCurrentRecord(gOovGui.getSelectedJournalIndex());
	gtk_widget_queue_draw(gOovGui.getBuilder().getWidget("DiagramDrawingarea"));
	}
    }

extern "C" G_MODULE_EXPORT void on_ModuleTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    // The component list should be available all the time as long as a
    // component can be selected.
//    if(gOovGui.getProject().isAnalysisReady())
	{
	std::string fn = gOovGui.getSelectedComponent();
	if(fn.length() > 0)
	    {
	    viewSource(fn, 1);
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ZoneTreeview_cursor_changed(
	GtkWidget *widget, gpointer data)
    {
    if(gOovGui.getProject().isAnalysisReady())
	{
	ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
	if(zoneDiagram)
	    {
	    bool show = false;
	    std::string comp = gOovGui.getZoneList().getComponentTree().getSelected('/');
	    if(comp.length() > 0)
		{
		show = gOovGui.getZoneList().getComponentTree().toggleSelectedCheckbox();
		zoneDiagram->setFilter(comp, !show);
		zoneDiagram->updateDiagram();
		}
	    }
	}
    }

extern "C" G_MODULE_EXPORT gboolean on_ZoneTreeview_button_press_event(
	GtkWidget *widget, const GdkEventButton *event)
    {
    // This prevents the right click display of popup from changing a checkbox.
    return(event->button != 1);
    }

extern "C" G_MODULE_EXPORT gboolean on_ZoneTreeview_button_release_event(
	GtkWidget *widget, const GdkEventButton *event)
    {
    bool handled = false;
    if(event->button != 1)
	{
	handled = true;
	if(gOovGui.getProject().isAnalysisReady())
	    {
	    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
	    if(zoneDiagram)
		{
		zoneDiagram->listDisplayContextMenu(event);
		}
	    }
	}
    return handled;
    }


extern "C" G_MODULE_EXPORT void on_ListNotebook_switch_page(GtkNotebook *notebook,
	GtkWidget *page, guint page_num, gpointer user_data)
    {
    gNotebook.mCurrentPage = page_num;
    switch(page_num)
	{
	case NotebookHandler::PI_Component:
	    gOovGui.clearSelectedComponent();;
	    gOovGui.getJournal().displayComponents();
	    gOovGui.updateJournalList();
	    break;

	case NotebookHandler::PI_Zone:
	    if(gOovGui.getProject().isAnalysisReady())
		{
		gOovGui.getJournal().displayWorldZone();
		gOovGui.updateJournalList();
		}
	    break;

	case NotebookHandler::PI_Class:
	    on_ClassTreeview_cursor_changed(nullptr, nullptr);
	    break;

	case NotebookHandler::PI_Portion:
	    {
	    std::string className = gOovGui.getSelectedClass();;
	    if(std::string(className).length() > 0)
		{
		gOovGui.getJournal().displayPortion(className);
		gOovGui.updateJournalList();
		}
	    }
	    break;

	// Operation is always related to class, so it must be initialized
	// to first operation of class.
	case NotebookHandler::PI_Seq:
	    on_OperationsTreeview_cursor_changed(nullptr, nullptr);
	    break;

	case NotebookHandler::PI_Journal:
	    on_JournalTreeview_cursor_changed(nullptr, nullptr);
	    break;
	}
    }

extern "C" G_MODULE_EXPORT gboolean on_StatusTextview_button_press_event(
	GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
	{
	std::string fn;
	int line = gOovGui.getStatusSourceFile(fn);
	if(fn.length() > 0)
	    {
	    viewSource(fn, line);
	    }
	}
    return FALSE;
    }

extern "C" G_MODULE_EXPORT void on_ProjectStatsMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gOovGui.displayProjectStats();
    }

extern "C" G_MODULE_EXPORT void on_LineStatsMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gOovGui.makeLineStats();
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutmenuitem_activate(GtkWidget *widget, gpointer data)
    {
    OovStringRef const comments =
	    "This program generates diagrams from C++ files, and allows"
	    " graphical navigation between drawings and source code.";
    gtk_show_about_dialog(nullptr, "program-name", "Oovcde",
	    "version", "Version " OOV_VERSION, "comments", comments.getStr(), nullptr);
    }

extern "C" G_MODULE_EXPORT void on_HelpContentsMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    const char *fn = "..\\..\\web\\userguide\\oovcdeuserguide.shtml";
    displayBrowserFile(fn);
    }
