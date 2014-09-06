//============================================================================
// Name        : oovcde.cpp
// \copyright 2013 DCBlaha.  Distributed under the GPL.
//============================================================================

#include "oovcde.h"
#include "DirList.h"
#include "Xmi2Object.h"
#include "Options.h"
#include "OptionsDialog.h"
#include "BuildSettingsDialog.h"
#include "BuildConfigReader.h"
#include "Svg.h"
#include "Gui.h"
#include "OovString.h"
#include "File.h"
#include "Version.h"
#include <stdlib.h>


static oovGui gOovGui;
static OptionsDialog *sOptionsDialog;


bool WindowBuildListener::onBackgroundProcessIdle(bool &completed)
    {
    bool didSomething = false;
    completed = false;
    std::string tempStdStr;
	{
	LockGuard guard(mMutex);
	if(mStdOutAndErr.length())
	    {
	    tempStdStr = mStdOutAndErr;
	    mStdOutAndErr.clear();
	    didSomething = true;
	    }
	}
    Gui::appendText(mStatusTextView, tempStdStr);
    if(mProcessComplete)
	{
	char const * const str = "\nComplete\n";
	Gui::appendText(mStatusTextView, str);
	mProcessComplete = false;
	completed = true;
	didSomething = true;
	}
    if(didSomething)
	Gui::scrollToCursor(mStatusTextView);
    return didSomething;
    }

void Menu::updateMenuEnables()
    {
    bool idle = mGui.mBackgroundProc.isIdle();
    bool open = mGui.isProjectOpen();

    if(idle != mBuildIdle || open != mProjectOpen || mInit)
	{
	gtk_widget_set_sensitive(mGui.getBuilder().getWidget(
		"EditOptionsmenuitem"), open);

	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("NewModuleMenuitem"), open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("SaveDrawingMenuitem"), open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("SaveDrawingAsMenuitem"), open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("BuildAnalyzeMenuitem"), idle && open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("BuildSettingsMenuitem"), open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("BuildDebugMenuitem"), idle && open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("BuildReleaseMenuitem"), idle && open);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("StopBuildMenuitem"), !idle);
	gtk_widget_set_sensitive(
		mGui.getBuilder().getWidget("MakeCMakeMenuitem"), open);
	mBuildIdle = idle;
	mProjectOpen = open;
	mInit = false;
	}
    }

void oovGui::init()
    {
    mBuilder.connectSignals();
    mClassList.init(mBuilder);
    mComponentList.init(mBuilder);
    mOperationList.init(mBuilder);
    mJournalList.init(mBuilder);
    mJournal.init(mBuilder, mModelData, *this);
    g_idle_add(onIdle, this);
    initListener(mBuilder);
    mMenu.updateMenuEnables();
    setProjectOpen(false);
    }

void oovGui::clear()
    {
    mJournal.clear();
    mModelData.clear();
    mClassList.clear();
    mComponentList.clear();
    mJournalList.clear();
    mOperationList.clear();
    mProjectOpen = false;
#if(LAZY_UPDATE)
    setBackgroundUpdateClassListSize(0);
#endif
    }

gboolean oovGui::onIdle(gpointer data)
    {
    oovGui *gui = reinterpret_cast<oovGui*>(data);
    if(gui->mOpenProject)
	{
	gui->openProject();
	gui->mOpenProject = false;
	}
    bool completed;
    if(!gui->onBackgroundProcessIdle(completed)
#if(LAZY_UPDATE)
	&& !gui->backgroundUpdateClassListItem()
#endif
	    )
	{
	sleepMs(5);
	}
    if(completed)
	gui->mOpenProject = true;
    gui->mMenu.updateMenuEnables();
    return true;
    }

void oovGui::openProject()
    {
    gOovGui.clear();
    BuildConfigReader buildConfig;
    std::vector<std::string> fileNames;
    bool open = getDirListMatchExt(buildConfig.getAnalysisPath().c_str(),
	    FilePath(".xmi", FP_File), fileNames);
    if(open)
	{
	int typeIndex = 0;
	BackgroundDialog backDlg;
	backDlg.setDialogText("Loading files.");
	backDlg.setProgressIterations(fileNames.size());
	for(size_t i=0; i<fileNames.size() ; i++)
	    {
	    File file(fileNames[i].c_str(), "r");
	    if(file.isOpen())
		{
		loadXmiFile(file.getFp(), mModelData, fileNames[i].c_str(), typeIndex);
		}
	    if(!backDlg.updateProgressIteration(i))
		{
		break;
		}
	    }
	mModelData.resolveModelIds();
#if(LAZY_UPDATE)
	setBackgroundUpdateClassListSize(mModelData.mTypes.size());
#else
	for(size_t i=0; i<mModelData.mTypes.size(); i++)
	    {
	    if(mModelData.mTypes[i]->getDataType() == DT_Class)
		mClassList.appendText(mModelData.mTypes[i]->getName().c_str());
	    }
	mClassList.sort();
#endif
	}
    if(sOptionsDialog)
	sOptionsDialog->openedProject();
    updateComponentList();
    setProjectOpen(open);
    }

void oovGui::displayClass(char const * const className)
    {
    // While the graph is initialized, there is no name.
    if(std::string(className).length() > 0)
	{
	updateClassList(className);
	updateOperationList(mModelData, className);
	mJournal.displayClass(className);
	updateJournalList();
	}
    }

void oovGui::addClass(char const * const className)
    {
    mJournal.addClass(className);
    }

void oovGui::displayOperation(char const * const className,
	char const * const operName, bool isConst)
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

bool oovGui::runSrcManager(char const * const buildConfigName)
    {
    bool success = true;
#ifdef __linux__
    char const * const procPath = "./oovBuilder";
#else
    char const * const procPath = "./oovBuilder.exe";
#endif
    clearListener(getBuilder());
    OovProcessChildArgs args;
    args.addArg(procPath);
    args.addArg(Project::getProjectDirectory().c_str());

    mMenu.updateMenuEnables();
    char const *str = NULL;
    if(std::string(buildConfigName) == BuildConfigAnalysis)
	{
	str = "\nAnalyzing\n";
	}
    else
	{
	str = "\nBuilding\n";
	args.addArg(makeBuildConfigArgName("-bld", buildConfigName).c_str());
	ComponentTypesFile compFile;
	compFile.read();
	success = compFile.anyComponentsDefined();
	if(!success)
	    {
	    Gui::messageBox("Define some components in Build/Settings",
		    GTK_MESSAGE_INFO);
	    }
	}
    if(success)
	{
	onStdOut(str, std::string(str).length());
	success = mBackgroundProc.startProcess(procPath, args.getArgv());
	}
    return success;
    }

void oovGui::stopSrcManager()
    {
    mBackgroundProc.childProcessKill();
    }

void oovGui::updateProject()
    {
    if(gBuildOptions.read())
	{
	gGuiOptions.read();
//	mOpenProject = true;
	runSrcManager(BuildConfigAnalysis);
	}
    else
	Gui::messageBox("Unable to open project");
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
	mJournalList.appendText(rec->getFullName(true).c_str());
	}
    }

void oovGui::updateClassList(char const * const className)
    {
    mClassList.setSelected(className);
    }

void oovGui::updateOperationList(const ModelData &modelData,
	char const * const className)
    {
    mOperationList.clear();
    const ModelClassifier *cls = modelData.getTypeRef(className)->getClass();
    if(cls)
	{
	for(size_t i=0; i<cls->getOperations().size(); i++)
	    {
	    const ModelOperation *oper = cls->getOperations()[i].get();
	    std::string opStr = oper->getName().c_str();
	    if(oper->isConst())
		{
		opStr += ' ';
		opStr += "const";
		}
	    mOperationList.appendText(opStr.c_str());
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
	    OovString numStr(line.substr(pos, endPos-pos).c_str());
	    if(numStr.getInt(0, INT_MAX, lineNum))
		fn = line.substr(0, pos-1);
	    }
	}
    return lineNum;
    }



class OptionsDialogUpdate:public OptionsDialog
    {
    virtual void updateOptions()
	{
	gOovGui.getJournal().cppArgOptionsChangedUpdateDrawings();
	gOovGui.updateProject();
	}
    virtual void buildConfig(char const * const name)
	{
	gOovGui.runSrcManager(name);
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

///// New Project Dialog ///////

extern "C" G_MODULE_EXPORT void on_RootSourceDirButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    std::string srcRootDir;
    if(ch.ChoosePath(gOovGui.getWindow(), "Open Root Source Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, srcRootDir))
	{
	GtkEntry *dirEntry = GTK_ENTRY(gOovGui.getBuilder().getWidget(
		"RootSourceDirEntry"));
	gtk_entry_set_text(dirEntry, srcRootDir.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_OovcdeProjectDirButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    std::string projectDir;
    if(ch.ChoosePath(gOovGui.getWindow(), "Create OOVCDE Project Directory",
	    GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
	projectDir))
	{
	GtkEntry *dirEntry = GTK_ENTRY(gOovGui.getBuilder().getWidget(
		"OovcdeProjectDirEntry"));
	gtk_entry_set_text(dirEntry, projectDir.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_RootSourceDirEntry_changed(
	GtkWidget *button, gpointer data)
    {
    gBuildOptions.setDefaultOptions();
    gGuiOptions.setDefaultOptions();
    GtkEntry *dirEntry = GTK_ENTRY(gOovGui.getBuilder().getWidget("RootSourceDirEntry"));
    FilePath rootSrcText(gtk_entry_get_text(dirEntry), FP_Dir);
    gBuildOptions.setNameValue(OptSourceRootDir, rootSrcText.c_str());

    GtkEntry *projDirEntry = GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "OovcdeProjectDirEntry"));

    removePathSep(rootSrcText, rootSrcText.length()-1);
    rootSrcText.appendFile("-oovcde");
    gtk_entry_set_text(projDirEntry, rootSrcText.c_str());
    }

extern "C" G_MODULE_EXPORT void on_ExcludeDirsButton_clicked(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    std::string dir;
    if(ch.ChoosePath(gOovGui.getWindow(), "Add Exclude Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, dir))
	{
	GtkTextView *dirTextView = GTK_TEXT_VIEW(gOovGui.getBuilder().getWidget(
		"ExcludeDirsTextview"));
	dir += '\n';
	Gui::appendText(dirTextView, dir.c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_NewProjectOkButton_clicked(
	GtkWidget *button, gpointer data)
    {
    GtkEntry *dirEntry = GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "OovcdeProjectDirEntry"));
    std::string projDir = gtk_entry_get_text(dirEntry);
    if(projDir.length())
	{
	ensureLastPathSep(projDir);
	if(ensurePathExists(projDir.c_str()))
	    {
	    Project::setProjectDirectory(projDir.c_str());

	    gBuildOptions.setFilename(Project::getProjectFilePath().c_str());

	    GtkTextView *excDirTextView = GTK_TEXT_VIEW(gOovGui.getBuilder().getWidget(
		    "ExcludeDirsTextview"));
	    CompoundValue val;
	    val.parseString(Gui::getText(excDirTextView).c_str(), '\n');
	    gBuildOptions.setNameValue(OptProjectExcludeDirs, val.getAsString(';').c_str());

	    gGuiOptions.setFilename(Project::getGuiOptionsFilePath().c_str());
	    if(gBuildOptions.writeFile())
		{
		gGuiOptions.writeFile();
		gOovGui.updateProject();
		}
	    else
		{
		Gui::messageBox("Unable to write project file");
		gOovGui.setProjectOpen(false);
		}
	    }
	else
	    Gui::messageBox("Unable to create project directory");
	gtk_widget_hide(gOovGui.getBuilder().getWidget("NewProjectDialog"));
	}
    }

// In glade, set the callback for the dialog's GtkWidget "delete-event" to
// gtk_widget_hide_on_delete for the title bar close button to work.
extern "C" G_MODULE_EXPORT void on_NewProjectCancelButton_clicked(
	GtkWidget *button, gpointer data)
    {
    gtk_widget_hide(gOovGui.getBuilder().getWidget("NewProjectDialog"));
    }

extern "C" G_MODULE_EXPORT void on_NewProjectMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    Dialog dlg(GTK_DIALOG(gOovGui.getBuilder().getWidget("NewProjectDialog")),
	    GTK_WINDOW(Builder::getBuilder()->getWidget("MainWindow")));
    dlg.run();
    }

////// New Module Dialog ////////////

extern "C" G_MODULE_EXPORT void on_NewModule_ModuleEntry_changed(
	GtkWidget *widget, gpointer data)
    {
    std::string module = Gui::getText(GTK_ENTRY(widget));
    std::string interfaceName = module + ".h";
    std::string implementationName = module + ".cpp";
    Gui::setText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_InterfaceEntry")), interfaceName.c_str());
    Gui::setText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_ImplementationEntry")), implementationName.c_str());
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
    std::string basePath = Project::getSrcRootDirectory();
    std::string interfaceName = Gui::getText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_InterfaceEntry")));
    std::string implementationName = Gui::getText(GTK_ENTRY(gOovGui.getBuilder().getWidget(
	    "NewModule_ImplementationEntry")));
    std::string compName = Gui::getText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
	    "NewModule_ComponentComboboxtext")));

    FilePath compDir(basePath, FP_Dir);
    if(compName != "<Root>")
	compDir.appendDir(compName.c_str());
    // Create the new component directory if it doesn't exist.
    ensurePathExists(compDir.c_str());

    if(!fileExists(interfaceName.c_str()))
	{
	FilePath tempInt(compDir, FP_Dir);
	tempInt.appendFile(interfaceName.c_str());
	File intFile(tempInt.c_str(), "w");
	fprintf(intFile.getFp(), "// %s", interfaceName.c_str());
	}
    else
	Gui::messageBox("Interface already exists", GTK_MESSAGE_INFO);

    if(!fileExists(implementationName.c_str()))
	{
	FilePath tempImp(compDir, FP_Dir);
	tempImp.appendFile(implementationName.c_str());
	File impFile(tempImp.c_str(), "w");
	fprintf(impFile.getFp(), "// %s", implementationName.c_str());
	impFile.close();
	viewSource(tempImp.c_str(), 1);
	}
    else
	Gui::messageBox("Implementation already exists", GTK_MESSAGE_INFO);

    gOovGui.updateProject();
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
	std::vector<std::string> names = componentsFile.getComponentNames();
	if(names.size() == 0)
	    {
	    Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
		    "NewModule_ComponentComboboxtext")), "<Root>");
	    }
	for(auto const &name : names)
	    {
	    Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui.getBuilder().getWidget(
		    "NewModule_ComponentComboboxtext")), name.c_str());
	    }
	Gui::setSelected(GTK_COMBO_BOX(gOovGui.getBuilder().getWidget(
		"NewModule_ComponentComboboxtext")), 0);
	}
    dlg.run();
    }

//////////////////

extern "C" G_MODULE_EXPORT void on_OpenProjectMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    std::string projectDir;
    PathChooser ch;
    if(ch.ChoosePath(gOovGui.getWindow(), "Open OOVCDE Project Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, projectDir))
	{
	Project::setProjectDirectory(projectDir.c_str());
	gOovGui.clear();
	gOovGui.updateProject();
	}
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    std::string fn = gOovGui.getLastSavedPath();
    SvgWriter svg(fn.c_str());
    gOovGui.getJournal().saveFile(svg.getFile());
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingAsMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    PathChooser ch;
    std::string fn = gOovGui.getLastSavedPath();
    ch.setDefaultPath(fn.c_str());
    if(ch.ChoosePath(gOovGui.getWindow(), "Save Drawing (.SVG)",
	    GTK_FILE_CHOOSER_ACTION_SAVE, fn))
	{
	gOovGui.setLastSavedPath(fn);
	SvgWriter svg(fn.c_str());
	gOovGui.getJournal().saveFile(svg.getFile());
	}
    }

extern "C" G_MODULE_EXPORT void on_BuildAnalyzeMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigAnalysis);
    }

extern "C" G_MODULE_EXPORT void on_BuildDebugMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigDebug);
    }

extern "C" G_MODULE_EXPORT void on_BuildReleaseMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.runSrcManager(BuildConfigRelease);
    }

extern "C" G_MODULE_EXPORT void on_StopBuildMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    gOovGui.stopSrcManager();
    }

extern "C" G_MODULE_EXPORT void on_MakeCMakeMenuitem_activate(
	GtkWidget *button, gpointer data)
    {
    OovProcessChildArgs args;
    std::string proc = makeExeFilename("./oovCMaker");

    std::string projNameArg;
    Dialog cmakeDlg(GTK_DIALOG(gOovGui.getBuilder().getWidget("CMakeDialog")));
    if(cmakeDlg.run(true))
	{
	GtkEntry *entry = GTK_ENTRY(gOovGui.getBuilder().getWidget("CMakeProjectNameEntry"));
	projNameArg = std::string("-n") + Gui::getText(entry);
	}

    bool putInSource = Gui::messageBox("Put files in source directory?",
	    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO);
    args.addArg(proc.c_str());
    args.addArg(Project::getProjectDirectory().c_str());
    if(putInSource)
	args.addArg("-w");
    args.addArg(projNameArg.c_str());
    spawnNoWait(proc.c_str(), args.getArgv());
    std::string stat = std::string("Building CMake files for ") +
	    Project::getProjectDirectory() + ".";
    if(putInSource)
	stat += "\nCheck the source directory for CMake files.";
    else
	stat += "\nCheck the oovcde directory for CMake files.";
    Gui::messageBox(stat.c_str(), GTK_MESSAGE_INFO);
    }

static bool sDisplayClassViewRightClick = false;

extern "C" G_MODULE_EXPORT void on_ClassTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    std::string className = gOovGui.getSelectedClass();
    if(sDisplayClassViewRightClick)
	{
	gOovGui.addClass(className.c_str());
	sDisplayClassViewRightClick = false;
	}
    else
	{
	gOovGui.displayClass(className.c_str());
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
    gOovGui.displayOperation(className.c_str(), operName.c_str(), isConst);
    }

extern "C" G_MODULE_EXPORT void on_JournalTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    gOovGui.getJournal().setCurrentRecord(gOovGui.getSelectedJournalIndex());
    gtk_widget_queue_draw(gOovGui.getBuilder().getWidget("DiagramDrawingarea"));
    }

extern "C" G_MODULE_EXPORT void on_ComponentTreeview_cursor_changed(
	GtkWidget *button, gpointer data)
    {
    if(gOovGui.isProjectOpen())
	{
	gOovGui.getJournal().displayComponents();
	gOovGui.updateJournalList();

	std::string fn = gOovGui.getSelectedComponent();
	if(fn.length() > 0)
	    {
	    viewSource(fn.c_str(), 1);
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ListNotebook_switch_page(GtkNotebook *notebook,
	GtkWidget *page, guint page_num, gpointer user_data)
    {
    switch(page_num)
	{
	case 0:
	    gOovGui.clearSelectedComponent();;
	    on_ComponentTreeview_cursor_changed(nullptr, nullptr);
	    break;

	case 1:
	    on_ClassTreeview_cursor_changed(nullptr, nullptr);
	    break;

	// Operation is always related to class, so it must be initialized
	// to first operation of class.
	case 2:
//	    on_OperationsTreeview_cursor_changed(nullptr, nullptr);
	    break;

	case 3:
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
	    viewSource(fn.c_str(), line);
	    }
	}
    return FALSE;
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutmenuitem_activate(GtkWidget *widget, gpointer data)
    {
    char const * const comments =
	    "This program generates diagrams from C++ files, and allows"
	    " graphical navigation between drawings and source code.";
    gtk_show_about_dialog(nullptr, "program-name", "Oovcde",
	    "version", "Version " OOV_VERSION, "comments", comments, nullptr);
    }
