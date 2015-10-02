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
#include "ComplexityView.h"
#include "DuplicatesView.h"
#include "StaticAnalysis.h"
#include "OovLibrary.h"
#include <stdlib.h>


static oovGui *gOovGui;
static OptionsDialog *sOptionsDialog;


WindowBuildListener::~WindowBuildListener()
    {}

void WindowBuildListener::initListener(Builder &builder)
    {
    // "DrawingStatusPaned"
    if(!mStatusTextView)
        {
        mStatusTextView = GTK_TEXT_VIEW(builder.getWidget("StatusTextview"));
        mErrHighlightTag.setForegroundColor(getBuffer(), "err", "red");
        }
    Gui::clear(mStatusTextView);
    }

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
    GtkTextBuffer *buf = getBuffer();
    bool atEnd = GuiTextBuffer::isCursorAtEnd(buf);
    if(tempStdStr.length())
        {
        // Highlight the error output.
        int startOffset = GuiTextIter::getIterOffset(GuiTextBuffer::getEndIter(buf));
        Gui::appendText(mStatusTextView, tempStdStr);

        size_t pos = 0;
        while(pos != std::string::npos)
            {
            pos = tempStdStr.find("error:", pos);
            if(pos != std::string::npos)
                {
                GtkTextIter startIter = GuiTextBuffer::getIterAtOffset(buf, startOffset + pos);
                GtkTextIter endIter = GuiTextBuffer::getIterAtOffset(buf, startOffset + pos + 6);
                int endOffset = GuiTextIter::getIterOffset(endIter);

                GuiTextBuffer::getText(buf, startOffset + pos, endOffset);
                mErrHighlightTag.applyTag(buf, &startIter, &endIter);
                pos++;
                }
            }
        }

    complete = mComplete;
    if(mComplete)
        {
        Gui::appendText(mStatusTextView, "\nComplete\n");
        buf = getBuffer();
        appended = true;
        mComplete = false;
        }
    if(atEnd)
        GuiTextBuffer::moveCursorToEnd(buf);
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

WindowProjectStatusListener::~WindowProjectStatusListener()
    {
    }


void oovGui::init()
    {
    mProject.setBackgroundProcessListener(&mWindowBuildListener);
    mProject.setStatusListener(&mProjectStatusListener);
    mWindowBuildListener.initListener(mBuilder);
    mBuilder.connectSignals();
    mContexts.init(mProjectStatusListener);
    OovError::setListener(this);
    g_idle_add(onIdle, this);
    updateMenuEnables(ProjectStatus());
    }

oovGui::~oovGui()
    {
    mProject.stopAndWaitForBackgroundComplete();
    mContexts.stopAndWaitForBackgroundComplete();
    clearAnalysis();
    g_idle_remove_by_data(this);
    }

void oovGui::errorListener(OovStringRef str, OovErrorTypes et)
    {
    if(et == ET_Error)
        {
        mWindowBuildListener.onStdErr(str, str.numBytes());
        }
    else
        {
        mWindowBuildListener.onStdOut(str, str.numBytes());
        }
    }

void oovGui::clearAnalysis()
    {
    mContexts.clear();
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
/*
    if(gBuildOptions.read())
        {
        gGuiOptions.read();
*/
        mContexts.updateContextAfterProjectLoaded();
        if(sOptionsDialog)
            {
            sOptionsDialog->updateBuildConfig();
            }
//        }
    }

gboolean oovGui::onBackgroundIdle(gpointer data)
    {
    bool complete;
    bool didSomething = mWindowBuildListener.onBackgroundProcessIdle(complete);
    if(complete)
        {
        clearAnalysis();
        mContexts.updateContextAfterAnalysisCompletes();
        didSomething = true;
        }
    EditorContainerCommands editorMsg;
    if(mContexts.handleEditorMessages(editorMsg))
        {
        switch(editorMsg)
            {
            case ECC_RunAnalysis:
                gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
                break;

            case ECC_Build:
                gOovGui->runSrcManager(BuildConfigDebug, OovProject::SM_Build);
                break;

            case ECC_StopAnalysis:
                gOovGui->stopSrcManager();
                break;

            default:
                break;
            }
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
            builder->getWidget("OpenDrawingMenuitem"), open);
    gtk_widget_set_sensitive(
            builder->getWidget("SaveDrawingMenuitem"), open);
    gtk_widget_set_sensitive(
            builder->getWidget("SaveDrawingAsMenuitem"), open);
    gtk_widget_set_sensitive(
            builder->getWidget("ExportDrawingAsMenuitem"), open);

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
            builder->getWidget("MethodUsageMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
            builder->getWidget("DuplicatesMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
            builder->getWidget("ProjectStatsMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
            builder->getWidget("LineStatsMenuitem"), idle && analRdy);
    gtk_widget_set_sensitive(
            builder->getWidget("CreateDatabaseMenuitem"), idle && analRdy);

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

std::string oovGui::getDiagramName(OovStringRef ext)
    {
    const JournalRecord *rec = mContexts.getCurrentJournalRecord();
    FilePath fn;
    if(rec)
        {
        fn.setPath(rec->getDiagramName(), FP_File);
        fn.appendExtension(ext);
        }
    else
        {
        fn.setPath("Unknown.svg", FP_File);
        }
    return fn;
    }

void oovGui::setDiagramName(OovStringRef name)
    {
    JournalRecord *rec = mContexts.getCurrentJournalRecord();
    if(rec)
        {
        rec->setDiagramName(name);
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
#ifdef __linux__
    pid_t pid=fork();
    if(!pid)
        {
        char const *prog = "/usr/bin/xdg-open";
        char const *args[3];
        args[0] = prog;
        args[1] = fileName.getStr();
        args[2] = nullptr;
//printf("FF %s\n", fp.c_str());
        execvp(prog, const_cast<char**>(args));
        }
#else
    FilePath fpTest(fileName, FP_File);
    ShellExecute(NULL, "open", fpTest.getAsWindowsPath().getStr(),
        NULL, NULL, SW_SHOWNORMAL);
#endif
    }

///
static void displayHelpFile(OovStringRef const fileName)
    {
    FilePath fullFn;
    static char const *dirs[] = { "help", "..\\..\\web\\userguide" };
    bool success = true;
    for(auto const dir : dirs)
        {
        fullFn.setPath(dir, FP_Dir);
        fullFn.appendFile(fileName);
        if(FileIsFileOnDisk(fullFn, success))
            {
            break;
            }
        }
    if(!FileIsFileOnDisk(fullFn, success))
        {
        fullFn.setPath("http://oovcde.sourceforge.net/userguide", FP_Dir);
        fullFn.appendFile(fileName);
        }
    displayBrowserFile(fullFn);
    }

static void displayWriteError(OovStringRef fn)
    {
    OovString str = "Unable to write ";
    str += fn;
    str += " to project directory";
    Gui::messageBox(str);
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
        displayWriteError(fn);
        }
    }

void oovGui::makeMemberUseFile()
    {
    std::string fn;
    if(createMemberVarUsageStaticAnalysisFile(Gui::getMainWindow(),
            mProject.getModelData(), fn))
        {
        displayBrowserFile(fn);
        }
    else
        {
        displayWriteError(fn);
        }
    }

void oovGui::makeMethodUseFile()
    {
    std::string fn;
    if(createMethodUsageStaticAnalysisFile(Gui::getMainWindow(),
            mProject.getModelData(), fn))
        {
        displayBrowserFile(fn);
        }
    else
        {
        displayWriteError(fn);
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
        displayWriteError(fn);
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

void oovGui::showProjectSettingsDialog()
    {
    ProjectSettingsDialog dlg(Gui::getMainWindow(),
        gOovGui->getProject().getProjectOptions(),
        gOovGui->getProject().getGuiOptions(), false);
    if(dlg.runDialog())
        {
        if(gOovGui->canStartAnalysis())
            {
            gOovGui->getProject().getProjectOptions().setNameValue(OptProjectExcludeDirs,
                    dlg.getExcludeDirs().getAsString(';'));
            gOovGui->getProject().getProjectOptions().writeFile();
            runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
            }
        }
    }


class OptionsDialogUpdate:public OptionsDialog
    {
    public:
        OptionsDialogUpdate(ProjectReader &options, GuiOptions &guiOptions):
            OptionsDialog(options, guiOptions)
            {}
        virtual ~OptionsDialogUpdate();
        virtual void updateOptions() override
            {
            gOovGui->cppArgOptionsChangedUpdateDrawings();
            gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
            }
        virtual void buildConfig(OovStringRef const name) override
            {
            gOovGui->runSrcManager(name, OovProject::SM_Build);
            }
    };

OptionsDialogUpdate::~OptionsDialogUpdate()
    {
    }

int main(int argc, char *argv[])
    {
    gtk_init(&argc, &argv);
    Builder builder;

    Project::setArgv0(argv[0]);
    if(builder.addFromFile("oovcdeLayout.glade"))
        {
        // Creating this as not a static means that all code that this initializes
        // will be initialized after statics have been initialized.
        // The oovGui constructs dialogs, which must be done after loading the glade file.
        oovGui oovGui(builder);
        gOovGui = &oovGui;
        gOovGui->init();
        OptionsDialogUpdate optionsDlg(gOovGui->getProject().getProjectOptions(),
                gOovGui->getProject().getGuiOptions());
        sOptionsDialog = &optionsDlg;
        BuildSettingsDialog buildDlg;

        GtkWidget *window = gOovGui->getBuilder().getWidget("MainWindow");
        gtk_widget_show(window);
        gtk_main();
        }
    else
        {
        Gui::messageBox("Unable to find oovcdeLayout.glade.");
        }
    return 0;
    }

extern "C" G_MODULE_EXPORT void on_NewProjectMenuitem_activate(
        GtkWidget *button, gpointer data)
    {
    ProjectSettingsDialog dlg(Gui::getMainWindow(),
            gOovGui->getProject().getProjectOptions(),
            gOovGui->getProject().getGuiOptions(), true);
    if(dlg.runDialog())
        {
        if(gOovGui->canStartAnalysis())
            {
            OovProject::eNewProjectStatus projStatus;
            if(gOovGui->getProject().newProject(
                    dlg.getProjectDir(), dlg.getExcludeDirs(), projStatus))
                {
                switch(projStatus)
                    {
                    case OovProject::NP_CreatedProject:
                        gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
                        gtk_widget_hide(gOovGui->getBuilder().getWidget("NewProjectDialog"));
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
    if(ch.ChoosePath(Gui::getMainWindow(), "Open OOVCDE Project Directory",
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, projectDir))
        {
        if(gOovGui->canStartAnalysis())
            {
            bool openedProject = false;
            if(gOovGui->getProject().openProject(projectDir, openedProject))
                {
                if(openedProject)
                    {
                    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
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
    gOovGui->showProjectSettingsDialog();
    }

////// New Module Dialog ////////////

extern "C" G_MODULE_EXPORT void on_NewModule_ModuleEntry_changed(
        GtkWidget * widget, gpointer /*data*/)
    {
    OovString module = Gui::getText(GTK_ENTRY(widget));
    OovString interfaceName = module + ".h";
    OovString implementationName = module + ".cpp";
    Gui::setText(GTK_ENTRY(gOovGui->getBuilder().getWidget(
            "NewModule_InterfaceEntry")), interfaceName);
    Gui::setText(GTK_ENTRY(gOovGui->getBuilder().getWidget(
            "NewModule_ImplementationEntry")), implementationName);
    }

// In glade, set the callback for the dialog's GtkWidget "delete-event" to
// gtk_widget_hide_on_delete for the title bar close button to work.
extern "C" G_MODULE_EXPORT void on_NewModuleCancelButton_clicked(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gtk_widget_hide(gOovGui->getBuilder().getWidget("NewModuleDialog"));
    }

extern "C" G_MODULE_EXPORT void on_NewModuleOkButton_clicked(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gtk_widget_hide(gOovGui->getBuilder().getWidget("NewModuleDialog"));
    OovString basePath = Project::getSrcRootDirectory();
    OovString interfaceName = Gui::getText(GTK_ENTRY(gOovGui->getBuilder().getWidget(
            "NewModule_InterfaceEntry")));
    OovString implementationName = Gui::getText(GTK_ENTRY(gOovGui->getBuilder().getWidget(
            "NewModule_ImplementationEntry")));
    OovString compName = Gui::getText(GTK_COMBO_BOX_TEXT(gOovGui->getBuilder().getWidget(
            "NewModule_ComponentComboboxtext")));

    FilePath compDir(basePath, FP_Dir);
    if(compName != Project::getRootComponentName())
        compDir.appendDir(compName);
    // Create the new component directory if it doesn't exist.
    FileEnsurePathExists(compDir);

    bool success = true;
    if(!FileIsFileOnDisk(interfaceName, success))
        {
        FilePath tempInt(compDir, FP_Dir);
        tempInt.appendFile(interfaceName);
        File intFile(tempInt, "w");
        fprintf(intFile.getFp(), "// %s", interfaceName.c_str());
        }
    else
        Gui::messageBox("Interface already exists", GTK_MESSAGE_INFO);

    if(!FileIsFileOnDisk(implementationName, success))
        {
        FilePath tempImp(compDir, FP_Dir);
        tempImp.appendFile(implementationName);
        File impFile(tempImp, "w");
        fprintf(impFile.getFp(), "// %s", implementationName.c_str());
        impFile.close();
        viewSource(gOovGui->getProject().getGuiOptions(), tempImp, 1);
        }
    else
        Gui::messageBox("Implementation already exists", GTK_MESSAGE_INFO);

    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
    }

extern "C" G_MODULE_EXPORT void on_NewModuleMenuitem_activate(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    Dialog dlg(GTK_DIALOG(gOovGui->getBuilder().getWidget("NewModuleDialog")));
    ComponentTypesFile componentsFile;
    Gui::clear(GTK_COMBO_BOX_TEXT(gOovGui->getBuilder().getWidget(
            "NewModule_ComponentComboboxtext")));
    if(componentsFile.read())
        {
        OovStringVec names = componentsFile.getComponentNames();
        if(names.size() == 0)
            {
            Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui->getBuilder().getWidget(
                    "NewModule_ComponentComboboxtext")), Project::getRootComponentName());
            }
        for(auto const &name : names)
            {
            Gui::appendText(GTK_COMBO_BOX_TEXT(gOovGui->getBuilder().getWidget(
                    "NewModule_ComponentComboboxtext")), name);
            }
        Gui::setSelected(GTK_COMBO_BOX(gOovGui->getBuilder().getWidget(
                "NewModule_ComponentComboboxtext")), 0);
        }
    dlg.run();
    }

class DrawingFile:public File
    {
    public:
        DrawingFile(OovStringRef fn, bool write)
            {
            FilePath fileName(fn, FP_File);
            if(write)
                {
                open(fileName, "w");
                }
            else
                {
                open(fileName, "r");
                }
            }
    };

extern "C" G_MODULE_EXPORT void on_OpenDrawingMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    PathChooser ch;
    OovString fn = gOovGui->getDiagramName("oov");
    if(ch.ChoosePath(Gui::getMainWindow(), "Open Drawing (.OOV)",
            GTK_FILE_CHOOSER_ACTION_OPEN, fn))
        {
        DrawingFile drawFile(fn, false);
        if(gOovGui->loadFile(drawFile))
            {
            gOovGui->setDiagramName(fn);
            }
        else
            {
            OovString str = "Unable to open drawing: ";
            str += fn;
            Gui::messageBox(str);
            }
        }
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    OovString fn = gOovGui->getDiagramName("oov");
    DrawingFile drawFile(fn, true);
    if(!gOovGui->saveFile(drawFile))
        {
        OovString str = "Unable to save drawing: ";
        str += fn;
        Gui::messageBox(str);
        }
    }

extern "C" G_MODULE_EXPORT void on_SaveDrawingAsMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    PathChooser ch;
    FilePath fn(gOovGui->getDiagramName("oov"), FP_File);
    ch.setDefaultPath(fn);
    if(ch.ChoosePath(Gui::getMainWindow(), "Save Drawing (.OOV)",
            GTK_FILE_CHOOSER_ACTION_SAVE, fn))
        {
        if(!fn.hasExtension())
            {
            fn.appendExtension("oov");
            }
        gOovGui->setDiagramName(fn);
        DrawingFile drawFile(fn, true);
        if(!gOovGui->saveFile(drawFile))
            {
            OovString str = "Unable to save drawing: ";
            str += fn;
            Gui::messageBox(str);
            }
        }
    }

extern "C" G_MODULE_EXPORT void on_ExportDrawingAsMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    PathChooser ch;
    FilePath fn(gOovGui->getDiagramName("svg"), FP_File);
    ch.setDefaultPath(fn);
    if(ch.ChoosePath(Gui::getMainWindow(), "Export Drawing (.SVG)",
            GTK_FILE_CHOOSER_ACTION_SAVE, fn))
        {
        if(!fn.hasExtension())
            {
            fn.appendExtension("svg");
            }
        DrawingFile svg(fn, true);
        if(!gOovGui->exportFile(svg))
            {
            OovString str = "Unable to export drawing: ";
            str += fn;
            Gui::messageBox(str);
            }
        }
    }


extern "C" G_MODULE_EXPORT void on_BuildAnalyzeMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_Analyze);
    }

extern "C" G_MODULE_EXPORT void on_ComplexityMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->makeComplexityFile();
    }

extern "C" G_MODULE_EXPORT void on_MemberUsageMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->makeMemberUseFile();
    }

extern "C" G_MODULE_EXPORT void on_MethodUsageMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->makeMethodUseFile();
    }

extern "C" G_MODULE_EXPORT void on_DuplicatesMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->makeDuplicatesFile();
    }

extern "C" G_MODULE_EXPORT void on_BuildDebugMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->runSrcManager(BuildConfigDebug, OovProject::SM_Build);
    }

extern "C" G_MODULE_EXPORT void on_BuildReleaseMenuitem_activate(
        GtkWidget *button, gpointer data)
    {
    gOovGui->runSrcManager(BuildConfigRelease, OovProject::SM_Build);
    }

extern "C" G_MODULE_EXPORT void on_StopBuildMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->stopSrcManager();
    }

extern "C" G_MODULE_EXPORT void on_InstrumentMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_CovInstr);
    }

extern "C" G_MODULE_EXPORT void on_CoverageBuildMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_CovBuild);
    }

extern "C" G_MODULE_EXPORT void on_CoverageStatsMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    gOovGui->runSrcManager(BuildConfigAnalysis, OovProject::SM_CovStats);
    }

extern "C" G_MODULE_EXPORT void on_MakeCMakeMenuitem_activate(
        GtkWidget * /*button*/, gpointer /*data*/)
    {
    OovProcessChildArgs args;
    OovString proc = FilePathMakeExeFilename("./oovCMaker");

    OovString projNameArg;
    Dialog cmakeDlg(GTK_DIALOG(gOovGui->getBuilder().getWidget("CMakeDialog")));
    if(cmakeDlg.run(true))
        {
        GtkEntry *entry = GTK_ENTRY(gOovGui->getBuilder().getWidget("CMakeProjectNameEntry"));
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

extern "C" G_MODULE_EXPORT gboolean on_StatusTextview_button_press_event(
        GtkWidget * /*widget*/, GdkEventExpose *event, gpointer /*user_data*/)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
        {
        std::string fn;
        unsigned int line = gOovGui->getStatusSourceFile(fn);
        if(fn.length() > 0)
            {
            viewSource(gOovGui->getProject().getGuiOptions(), fn, line);
            }
        }
    return FALSE;
    }

extern "C" G_MODULE_EXPORT void on_ProjectStatsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gOovGui->displayProjectStats();
    }

extern "C" G_MODULE_EXPORT void on_LineStatsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gOovGui->makeLineStats();
    }


// This is the C style interface to the Oov database writer run-time library.
extern "C"
{
struct DatabaseWriterInterface
    {
    bool (*OpenDb)(char const *projectDir, void const *modelData);
    bool (*WriteDb)(int passIndex, int &typeIndex, int maxTypesPerTransaction);
    char const *(*GetLastError)();
    void (*CloseDb)();
    };
};
class DatabaseWriter:public DatabaseWriterInterface, public OovLibrary
    {
    public:
        void writeDatabase(ModelData *modelData)
            {
            // Linux is interesting here.  If the project directory (a static
            // string) is read after the library is opened, then the string
            // is empty in this calling executable code.  So this must be
            // read before the library is opened, then passed to the library.
            OovString projDir = Project::getProjectDirectory();
#ifdef __linux__
            FilePath reportLibName(Project::getBinDirectory(), FP_Dir);
            reportLibName.appendFile("libOovDbWriter.so");
#else
            OovStringRef reportLibName = "oovDbWriter.dll";
#endif
            bool success = OovLibrary::open(reportLibName.getStr());
            if(success)
                {
                loadSymbols();
                bool success = OpenDb(projDir.getStr(), modelData);
                if(success)
                    {
                    TaskBusyDialog progressDlg;
                    progressDlg.setParentWindow(Gui::getMainWindow());
                    bool keepGoing = true;
                    size_t totalTypes = modelData->mTypes.size();
                    for(int pass=0; pass<2 && success && keepGoing; pass++)
                        {
                        int typeIndex = 0;
                        OovString str = "Adding Data - Pass ";
                        str.appendInt(pass+1);
                        str += " of 2";
                        progressDlg.startTask(str.getStr(), totalTypes);
                        while(typeIndex < static_cast<int>(totalTypes) && success && keepGoing)
                            {
                            success = WriteDb(pass, typeIndex, 40);
                            if(success)
                                {
                                keepGoing = progressDlg.updateProgressIteration(typeIndex, nullptr, true);
                                typeIndex++;
                                }
                            }
                        progressDlg.endTask();
                        }
                    CloseDb();
                    }
                if(!success)
                    {
                    Gui::messageBox(GetLastError());
                    }
                }
            else
                {
                OovString errStr = "Unable to find ";
                errStr += reportLibName;
                Gui::messageBox(errStr.getStr());
                }
            }

    private:
        void loadSymbols()
            {
            loadModuleSymbol("OpenDb", (OovProcPtr*)&OpenDb);
            loadModuleSymbol("WriteDb", (OovProcPtr*)&WriteDb);
            loadModuleSymbol("GetLastError", (OovProcPtr*)&GetLastError);
            loadModuleSymbol("CloseDb", (OovProcPtr*)&CloseDb);
            }
    };

extern "C" G_MODULE_EXPORT void on_CreateDatabaseMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    OovString str = "This creates an sqlite database. "
            "It currently does not support appending to the database."
            "The OovReports.db file will be created in the project output directory.\n";
#ifdef __linux__
    str += " The libsqlite3.so must be available in the normal search locations.\n";
#else
    str += " The sqlite3.dll must be available in the bin directory or path.\n";
#endif
    str += " Continue?";
    bool success = Gui::messageBox(str, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO);
    if(success)
        {
        DatabaseWriter writer;
        writer.writeDatabase(&gOovGui->getProject().getModelData());
        }
    }

extern "C" G_MODULE_EXPORT gboolean on_MainWindow_delete_event(GtkWidget *button, gpointer data)
    {
    bool ok = gOovGui->okToExit();
    if(!ok)
        {
        Gui::messageBox("Please close the editor first");
        }
    return !ok;
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutmenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    OovStringRef const comments =
            "This program generates diagrams from C++ files, and allows"
            " graphical navigation between drawings and source code.";
    gtk_show_about_dialog(Gui::getMainWindow(), "program-name", "Oovcde",
            "version", "Version " OOV_VERSION, "comments", comments.getStr(), nullptr);
    }

extern "C" G_MODULE_EXPORT void on_HelpContentsMenuitem_activate(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    displayHelpFile("oovcdeuserguide.html");
    }

extern "C" G_MODULE_EXPORT void on_BuildArgsHelp_clicked(
    GtkWidget * /*widget*/, gpointer /*data*/)
    {
    displayHelpFile("oovcdebuildhelp.html");
    }
