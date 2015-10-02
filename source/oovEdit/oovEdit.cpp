//============================================================================
// Name        : oovEdit.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
// Description : Simple Editor
//============================================================================

#include "FilePath.h"
#include "Version.h"
#include "Debugger.h"
#include "oovEdit.h"
#include "Project.h"
#include "Components.h"
#include "DirList.h"
#include <string.h>
#include <vector>


Editor::Editor():
    mEditFiles(mDebugger, mEditOptions), mLastSearchCaseSensitive(false)
    {
    mDebugger.setListener(*this);
    }

void Editor::init()
    {
    mEditFiles.init(mBuilder);
    mVarView.init(mBuilder, "DataTreeview", nullptr);
    setStyle();
    g_idle_add(onIdleCallback, this);
    }

void Editor::openTextFile(OovStringRef const fn)
    {
    mEditFiles.viewModule(fn, 1);
    }

void Editor::openTextFile()
    {
    OovString filename;
    PathChooser ch;
    if(ch.ChoosePath(Gui::getMainWindow(), "Open File",
            GTK_FILE_CHOOSER_ACTION_OPEN, filename))
        {
        openTextFile(filename);
        }
    }

bool Editor::saveTextFile()
    {
    bool saved = false;
    if(mEditFiles.getEditView())
        saved = mEditFiles.getEditView()->saveTextFile();
    return saved;
    }

bool Editor::saveAsTextFileWithDialog()
    {
    bool saved = false;
    if(mEditFiles.getEditView())
        saved = mEditFiles.saveAsTextFileWithDialog();
    return saved;
    }

class FindDialog:public Dialog
    {
    public:
        FindDialog(EditFiles &editFiles):
            Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("FindDialog")))
            {
            GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget("FindEntry"));
            FileEditView *view = editFiles.getEditView();
            std::string viewText;
            if(view)
                {
                viewText = view->getSelectedText();
                }
            if(viewText.length() > 0)
                {
                Gui::setText(entry, viewText);
                }
            gtk_widget_grab_focus(GTK_WIDGET(entry));
            gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
            }
    };

void Editor::findDialog()
    {
    FindDialog dialog(mEditFiles);
    bool done = false;
    while(!done)
        {
        int ret = dialog.runHideCancel();
        if(ret == GTK_RESPONSE_CANCEL || ret == GTK_RESPONSE_DELETE_EVENT)
            {
            done = true;
            }
        else
            {
            GtkToggleButton *downCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
                    "FindDownCheckbutton"));
            GtkToggleButton *caseCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
                    "CaseSensitiveCheckbutton"));
            GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget("FindEntry"));
            if(ret == GTK_RESPONSE_OK)
                {
                find(gtk_entry_get_text(entry), gtk_toggle_button_get_active(downCheck),
                    gtk_toggle_button_get_active(caseCheck));
                }
            else
                {
                GtkEntry *replaceEntry = GTK_ENTRY(getBuilder().getWidget("ReplaceEntry"));
                findAndReplace(gtk_entry_get_text(entry), gtk_toggle_button_get_active(downCheck),
                    gtk_toggle_button_get_active(caseCheck), gtk_entry_get_text(replaceEntry));
                }
            }
        }
    }


void Editor::gotoLineDialog()
    {
    Dialog dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("GoToLineDialog")));
    int ret = dialog.run(true);
    if(ret)
        {
        FileEditView *view = mEditFiles.getEditView();
        GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget("LineNumberEntry"));
        OovString lineNumStr = gtk_entry_get_text(entry);
        int lineNum;
        if(lineNumStr.getInt(0, INT_MAX, lineNum))
            {
            view->gotoLine(lineNum);
            }
        Gui::setText(entry, "");
        }
    }

void Editor::findAgain(bool forward)
    {
    if(mLastSearch.length() == 0)
        {
        findDialog();
        }
    else
        {
        find(mLastSearch, forward, mLastSearchCaseSensitive);
        }
    }


class FindFiles:public dirRecurser
    {
    public:
        ~FindFiles()
            {}
        FindFiles(char const *srchStr, bool caseSensitive, GtkTextView *view):
            mSrchStr(srchStr), mCaseSensitive(caseSensitive), mView(view),
            mNumMatches(0)
            {}
        bool recurseDirs(char const * const srcDir)
            {
            mNumMatches = 0;
            return dirRecurser::recurseDirs(srcDir);
            }
        int getNumMatches() const
            { return mNumMatches; }

    private:
        virtual bool processFile(OovStringRef const filePath) override;

    private:
        std::string mSrchStr;
        bool mCaseSensitive;
        GtkTextView *mView;
        int mNumMatches;
    };

#ifndef __linux__
static int my_strnicmp(char const *str1, char const *str2, int len)
    {
    int val = 0;
    for(int i=0; i<len; i++)
        {
        val = toupper(str1[i]) - toupper(str2[i]);
        if(val != 0)
            break;
        }
    return val;
    }

static char const *strcasestr(char const *haystack, char const *needle)
    {
    int needleLen = strlen(needle);
    char const *retp = nullptr;
    while(*haystack)
        {
        // strncasecmp, strnicmp, _strnicmp not working under mingw with some flags
        if(my_strnicmp(haystack, needle, needleLen) == 0)
            {
            retp = haystack;
            break;
            }
        haystack++;
        }
    return retp;
    }
#endif

// Return true while success.
bool FindFiles::processFile(OovStringRef const filePath)
    {
    bool success = true;
    FilePath ext(filePath, FP_File);

    if(isHeader(ext) || isSource(ext))
        {
        FILE *fp = fopen(filePath.getStr(), "r");
        if(fp)
            {
            char buf[1000];
            int lineNum = 0;
            while(fgets(buf, sizeof(buf), fp))
                {
                lineNum++;
                char const *match=NULL;
                if(mCaseSensitive)
                    {
                    match = strstr(buf, mSrchStr.c_str());
                    }
                else
                    {
                    match = strcasestr(buf, mSrchStr.c_str());
                    }
                if(match)
                    {
                    mNumMatches++;
                    OovString matchStr = filePath;
                    matchStr += ':';
                    matchStr.appendInt(lineNum);
                    matchStr += "   ";
                    matchStr += buf;
//                  matchStr += '\n';
                    Gui::appendText(mView, matchStr);
                    }
                }
            fclose(fp);
            }
        }
    return success;
    }

void Editor::findInFiles(char const * const srchStr, char const * const path,
        bool caseSensitive, GtkTextView *view)
    {
    FindFiles findFiles(srchStr, caseSensitive, view);
    findFiles.recurseDirs(path);
    OovString matchStr = "Found ";
    matchStr.appendInt(findFiles.getNumMatches());
    matchStr += " matches";
    Gui::appendText(view, matchStr);
    }

void Editor::findInFilesDialog()
    {
    FindDialog dialog(mEditFiles);
    GtkToggleButton *downCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
            "FindDownCheckbutton"));
    Gui::setVisible(GTK_WIDGET(downCheck), false);
    if(dialog.run(true))
        {
        GtkToggleButton *caseCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
                "CaseSensitiveCheckbutton"));
        mEditFiles.showInteractNotebookTab("Find");
        GtkTextView *findView = GTK_TEXT_VIEW(getBuilder().getWidget("FindTextview"));
        Gui::clear(findView);
        GtkEntry *entry = GTK_ENTRY(Builder::getBuilder()->getWidget("FindEntry"));
        findInFiles(gtk_entry_get_text(entry), Project::getSrcRootDirectory().getStr(),
                gtk_toggle_button_get_active(caseCheck), findView);
        }
    Gui::setVisible(GTK_WIDGET(downCheck), true);
    }

void Editor::gotoFileLine(std::string const &lineBuf)
    {
    size_t pos = lineBuf.rfind(':');
    if(pos != std::string::npos)
        {
        int lineNum;
        OovString numStr(lineBuf.substr(pos+1, lineBuf.length()-pos));
        if(numStr.getInt(0, INT_MAX, lineNum))
            {
            if(lineNum == 0)
                lineNum = 1;
            mEditFiles.viewFile(lineBuf.substr(0, pos), lineNum);
            }
        }
    }

/*
void Editor::setTabs(int numSpaces)
    {
    // We really want 8 space tabs, so don't change anything here.
    int charWidth = 0;
    int charHeight = 0;
    PangoLayout *layout = gtk_widget_create_pango_layout(mTextView, "A");


    PangoTabArray *tabs = pango_tab_array_new(50, true);

    layout->FontDescription = font;
    layout.GetPixelSize(out charWidth, out charHeight);
    for (int i = 0; i < tabs.Size; i++)
        {
        tabs.SetTab(i, TabAlign.Left, i * charWidth * numSpaces);
        }
    gtk_text_view_set_tabs(mTextView, tabs);
    }
*/

void Editor::setStyle()
    {
    GtkCssProvider *provider = gtk_css_provider_get_default();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);

    OovString path = Project::getBinDirectory();
    path += "OovEdit.css";
    GError *err = 0;
    gtk_css_provider_load_from_path(provider, path.getStr(), &err);
    gtk_style_context_add_provider_for_screen (screen,
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    }

void Editor::find(OovStringRef const findStr, bool forward, bool caseSensitive)
    {
    mLastSearch = findStr;
    mLastSearchCaseSensitive = caseSensitive;
    if(mEditFiles.getEditView())
        mEditFiles.getEditView()->find(findStr, forward, caseSensitive);
    }

void Editor::findAndReplace(OovStringRef const findStr, bool forward, bool caseSensitive,
        OovStringRef const replaceStr)
    {
    mLastSearch = findStr;
    mLastSearchCaseSensitive = caseSensitive;
    if(mEditFiles.getEditView())
        mEditFiles.getEditView()->findAndReplace(findStr, forward, caseSensitive, replaceStr);
    }


Editor *gEditor;

gboolean Editor::onIdleCallback(gpointer data)
    {
    return gEditor->onIdle(data);
    }

gboolean Editor::onIdle(gpointer data)
    {
    if(mDebugOut.length())
        {
        GtkTextView *view = GTK_TEXT_VIEW(getBuilder().
                getWidget("ControlTextview"));
        Gui::appendText(view, mDebugOut);
        Gui::scrollToCursor(view);
        mDebugOut.clear();
        }
    eDebuggerChangeStatus dbgStatus = mDebugger.getChangeStatus();
    if(dbgStatus != DCS_None)
        {
        idleDebugStatusChange(dbgStatus);
        }
    getEditFiles().onIdle();
    OovIpcMsg msg;
    if(mEditorIpc.getMessage(msg))
        {
        OovString cmd = msg.getArg(0);
        if(cmd[0] == EC_ViewFile)
            {
            OovString fn = msg.getArg(1);
            OovString lineNumStr = msg.getArg(2);
            int lineNum = 1;
            size_t pos = lineNumStr.findSpace();
            if(pos != std::string::npos)
                {
                lineNumStr = lineNumStr.substr(0, pos);
                }
            if(lineNumStr.getInt(0, INT_MAX, lineNum))
                {
                }
            gEditor->getEditFiles().viewModule(fn, lineNum);
            }
        }
    sleepMs(5);
    return true;
    }

static void appendTree(GuiTree &varView, GuiTreeItem &parentItem,
        DebugResult const &debResult)
    {
    OovString str = debResult.getVarName();
    if(debResult.getValue().length() > 0)
        {
        str += " : ";
        str += debResult.getValue();
        }
    GuiTreeItem item = varView.appendText(parentItem, str);
    for(auto const &childRes : debResult.getChildResults())
        {
        appendTree(varView, item, *childRes.get());
        }
    int numChildren = varView.getNumChildren(GuiTreeItem());
    OovString numPathStr;
    numPathStr.appendInt(numChildren-1);
    GuiTreePath path(numPathStr);
    varView.expandRow(path);
    }

void Editor::idleDebugStatusChange(eDebuggerChangeStatus st)
    {
    if(st == DCS_RunState)
        {
        getEditFiles().updateDebugMenu();
        if(mDebugger.getChildState() == DCS_ChildPaused)
            {
            auto const &loc = mDebugger.getStoppedLocation();
            mEditFiles.viewModule(loc.getFilename(), loc.getLine());
            mDebugger.startGetStack();
            }
        }
    else if(st == DCS_Stack)
        {
        GtkTextView *view = GTK_TEXT_VIEW(mBuilder.getWidget("StackTextview"));
        Gui::setText(view, mDebugger.getStack());
        }
    else if(st == DCS_Value)
        {
//        mVarView.appendText(GuiTreeItem(), mDebugger.getVarValue().getAsString());
        GuiTreeItem item;
        appendTree(mVarView, item, mDebugger.getVarValue());

        int numChildren = mVarView.getNumChildren(GuiTreeItem());
        OovString numPathStr;
        numPathStr.appendInt(numChildren-1);
        GuiTreePath path(numPathStr);
        mVarView.scrollToPath(path);
        }
    }

void Editor::debugSetStackFrame(OovStringRef const frameLine)
    {
    mDebugger.setStackFrame(frameLine);
    }

/*
// This indicates if a scroll occurred.
//gboolean draw(GtkWidget *widget, CairoContext *cr, gpointer user_data)
gboolean draw(GtkWidget *widget, void *cr, gpointer user_data)
    {
    gEditor->drawHighlight();
    return FALSE;
    }
*/
/*
static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
    gEditor->setNeedHighlightUpdate(true);
    }
*/

void Editor::loadSettings()
    {
    int width, height;
    mEditOptions.setProjectDir(mProjectDir);
    if(mEditOptions.getScreenSize(width, height))
        {
        gtk_window_resize(Gui::getMainWindow(), width, height);
        }
    Project::setProjectDirectory(mProjectDir);
    }

void Editor::saveSettings()
    {
    int width, height;
    gtk_window_get_size(Gui::getMainWindow(), &width, &height);
    mEditOptions.saveScreenSize(width, height);
    }

void Editor::editPreferences()
    {
    GtkComboBoxText *cb = GTK_COMBO_BOX_TEXT(mBuilder.getWidget("DebugComponent"));
    Gui::clear(cb);

    ComponentTypesFile compFile;
    compFile.read();
    bool haveNames = false;
    Gui::setSelected(cb, 0);
    std::string dbgComponent = mEditOptions.getValue(OptEditDebuggee);
    int compIndex = -1;
    int boxCount = 0;
    for(const std::string &name : compFile.getComponentNames())
        {
        if(compFile.getComponentType(name) == ComponentTypesFile::CT_Program)
            {
            FilePath fp(Project::getBuildOutputDir(BuildConfigDebug), FP_Dir);
            OovString filename = name;
            if(strcmp(name.c_str(), Project::getRootComponentName()) == 0)
                filename = Project::getRootComponentFileName();
            fp.appendFile(FilePathMakeExeFilename(filename));
            Gui::appendText(cb, fp);
            haveNames = true;
            if(fp.compare(dbgComponent) == 0)
                {
                compIndex = boxCount;
                }
            boxCount++;
            }
        }
    if(compIndex == -1)
        {
        if(dbgComponent.length() > 0)
            {
            Gui::appendText(cb, dbgComponent);
            compIndex = boxCount;
            }
        }
    if(compIndex != -1)
        {
        Gui::setSelected(cb, compIndex);
        }
    GtkEntry *debuggeeArgs = GTK_ENTRY(mBuilder.getWidget("CommandLineArgsEntry"));
    Gui::setText(debuggeeArgs, mEditOptions.getValue(OptEditDebuggeeArgs));
    GtkEntry *workDirEntry = GTK_ENTRY(mBuilder.getWidget("DebugWorkingDirEntry"));
    Gui::setText(workDirEntry, mEditOptions.getValue(OptEditDebuggerWorkingDir));

    Dialog dlg(GTK_DIALOG(mBuilder.getWidget("Preferences")));
    if(dlg.run(true))
        {
        if(haveNames)
            {
            std::string text = Gui::getText(cb);
            mEditOptions.setNameValue(OptEditDebuggee, text);
            mEditOptions.setNameValue(OptEditDebuggeeArgs, Gui::getText(debuggeeArgs));
            mEditOptions.setNameValue(OptEditDebuggerWorkingDir, Gui::getText(workDirEntry));
//          mDebugger.setDebuggee(text);
//          mDebugger.setDebuggeeArgs(mEditOptions.getValue(OptEditDebuggeeArgs));
            }
        else
            Gui::messageBox("Some components for the project must be defined as programs");
        }
    }

void Editor::setPreferencesWorkingDir()
    {
    PathChooser chooser;
    chooser.setDefaultPath(Project::getBuildOutputDir(BuildConfigDebug));
    OovString dir;
    if(chooser.ChoosePath(Gui::getMainWindow(), "Select Working Directory",
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, dir))
        {
        GtkEntry *workDirEntry = GTK_ENTRY(mBuilder.getWidget("DebugWorkingDirEntry"));
        Gui::setText(workDirEntry, dir);
        }
    }

bool Editor::checkExitSave()
    {
    bool exitOk = mEditFiles.checkExitSave();
    if(exitOk)
        {
        /// @todo - hang or crash on exit!!
        // There is a bug somewhere that disposing the mTransUnit in
        // the tokenizer while the program is exiting crashes.  Adding
        // the closeAll to close the tokenizer early works, but then
        // the program hangs sometime after this function returns.
        // So the real reason for this bug is that something is hanging
        // and not allowing GTK to exit.
        // For now, we exit here, but this is just a temporary fix.
        mEditFiles.closeAll();
        exit(0);
        }
    return exitOk;
    }

// Using pipe IPC and editor container to talk between editor and oovcde solves
// all problems by launching a single editor. No need to use special GTK stuff,
// and it works much better since only a single process is launched instead of
// trying to launch a process, then discovering it is already running.
#define SINGLE_INSTANCE 0

#if(SINGLE_INSTANCE)

// Note that this is called a few times. If command line arguments are
// given, then it is called from the open. It can also be called from
// g_application_run.
static void activateApp(GApplication *gapp)
    {
    GtkApplication *app = GTK_APPLICATION (gapp);
    GtkWidget *window = Gui::getMainWindow();
    gEditor->loadSettings();
    gtk_widget_show_all(window);
    gtk_application_add_window(app, GTK_WINDOW(window));
    }

// command-line signal
// Beware - this is called many times when starting up.   It is a bit strange,
// but this function can be called from different processes during startup.
static void commandLine(GApplication *gapp, GApplicationCommandLine *cmdline,
    gpointer user_data)
    {
    int argc;
    gchar **argv = g_application_command_line_get_arguments(cmdline, &argc);
    bool goodArgs = true;
    for(gint i = 0; i < argc && goodArgs; i++)
        {
        char const * fn = nullptr;
        int line = 1;
        for(int argi=1; argi<argc; argi++)
            {
            if(argv[argi][0] == '+')
                {
                if(argv[argi][1] == 'p')
                    {
                    OovString fname = FilePathFixFilePath(&argv[argi][2]);
                    gEditor->setProjectDir(fname);
                    }
                else
                    sscanf(&argv[argi][1], "%d", &line);
                }
            else if(argv[argi][0] == '-')
                {
                if(argv[argi][1] == 'p')
                    {
                    OovString fname = FilePathFixFilePath(&argv[argi][2]);
                    gEditor->setProjectDir(fname);
                    }
                else
                    goodArgs = false;
                }
            else
                fn = argv[argi];
            }
        if(fn)
            {
            activateApp(gapp);
            gEditor->getEditFiles().viewModule(FilePathFixFilePath(fn), line);
            }
        }
    if(goodArgs)
        {
        activateApp(gapp);
        }
    else
        {
        std::string str = "OovEdit version ";
        str += OOV_VERSION;
        str += "\n";
        str += "oovEdit: Args are: filename [args]...\n";
        str += "args are:\n";
        str += "   +<line>            line number of opened file\n";
        str += "   -p<projectDir>    directory of project files\n";
        fprintf(stderr, "%s\n", str.c_str());
        fflush(stderr);
        Gui::messageBox(str);
        }
    }

static void startupApp(GApplication *gapp)
    {
    if(gEditor->getBuilder().addFromFile("oovEdit.glade"))
        {
        gEditor->init();
        gEditor->getBuilder().connectSignals();
        }
    else
        {
        Gui::messageBox("Unable to find oovEdit.glade.");
        exit(1);
        }
    }

int main(int argc, char **argv)
    {
    GtkApplication *app = gtk_application_new("org.oovcde.oovEdit",
            G_APPLICATION_HANDLES_COMMAND_LINE);
    GApplication *gapp = G_APPLICATION(app);

    Editor editor;
    gEditor = &editor;
    g_signal_connect(app, "activate", G_CALLBACK(activateApp), NULL);
    g_signal_connect(app, "startup", G_CALLBACK(startupApp), NULL);
    g_signal_connect(app, "command-line", G_CALLBACK(commandLine), NULL);
    // For some reason, the '-' arguments quit working when upgrading Ubuntu.
    // It would cause the commandLine function to fail in some way. So
    // this cheat converts the minus to a plus, and both are handled in
    // the commandLine function.  This dirty trick does change memory that is
    // passed to main, but it seems to work.
#ifdef __linux__
    for(int i=0; i<argc; i++)
        {
        if(argv[i][0] == '-')
            argv[i][0] = '+';
        }
#endif
    int status = g_application_run(gapp, argc, argv);
    g_object_unref(app);
    return status;
    }


#else

#define CATCH_EXCEPTIONS 0

#if(CATCH_EXCEPTIONS)

#ifdef __linux__
static void dumpExceptions()
    {
    backtrace();
    }

#else

#include <DbgHelp.h>
// Requires DbgHelp.dll/dbghelp.lib
static void dumpExceptions()
    {
    unsigned int   i;
    void *stack[ 100 ];
    unsigned short frames;
    SYMBOL_INFO *symbol;
    HANDLE process;

    process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    frames = CaptureStackBackTrace(0, 100, stack, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1 );
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for( i = 0; i < frames; i++ )
        {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        printf("%i: %s - 0x%0X\n", frames-i-1, symbol->Name, symbol->Address);
        }
    free(symbol);
    #endif
    }
#endif

int main(int argc, char *argv[])
    {
    gtk_init (&argc, &argv);
    // Creating this as not a static means that all code that this initializes
    // will be initialized after statics have been initialized.
    Editor editor;
    gEditor = &editor;
    Project::setArgv0(argv[0]);
#if(CATCH_EXCEPTIONS)
    std::set_terminate(dumpExceptions);
#endif
    if(editor.getBuilder().addFromFile("oovEdit.glade"))
        {
        char const *fn = nullptr;
        int line = 0;
        bool debug = false;
        for(int argi=1; argi<argc; argi++)
            {
            if(argv[argi][0] == '+')
                {
                sscanf(&argv[argi][1], "%d", &line);
                }
            else if(argv[argi][0] == '-')
                {
                if(argv[argi][1] == 'p')
                    {
                    OovString fname = FilePathFixFilePath(&argv[argi][2]);
                    editor.setProjectDir(fname);
                    }
                if(argv[argi][1] == 'd')
                    {
                    debug = true;
                    }
                }
            else
                fn = argv[argi];
            }
        if(!debug)
            {
            // This must be before the editor.init(), or connectSignals(),
            // otherwise the pipes don't work.
            editor.startStdinListening();
            }
        editor.init();
        editor.getBuilder().connectSignals();
        GtkWidget *window = editor.getBuilder().getWidget("MainWindow");
        gEditor->loadSettings();
        if(fn)
            {
            editor.getEditFiles().viewModule(fn, line);
            }
        else
            {
            fprintf(stderr, "OovEdit version %s\n", OOV_VERSION);
            fprintf(stderr, "oovEdit: Args are: filename [args]...\n");
            fprintf(stderr, "args are:\n");
            fprintf(stderr, "   +<line>           line number of opened file\n");
            fprintf(stderr, "   -p<projectDir>    directory of project files\n");
            fprintf(stderr, "   -d                debug without pipes\n");
            }
        gtk_widget_show(window);
        gtk_main();
        }
    else
        {
        Gui::messageBox("The file oovEdit.glade must be in the executable directory.");
        }
    return 0;
    }
#endif

extern "C" G_MODULE_EXPORT void on_FileOpenImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->openTextFile();
    }

extern "C" G_MODULE_EXPORT void on_FileSaveImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->saveTextFile();
    }

extern "C" G_MODULE_EXPORT void on_FileSaveAsImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->saveAsTextFileWithDialog();
    }

extern "C" G_MODULE_EXPORT void on_FileQuitImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
#if(GTK_MINOR_VERSION >= 10)
    gtk_window_close(Gui::getMainWindow());
#endif
    }

extern "C" G_MODULE_EXPORT gboolean on_MainWindow_delete_event(GtkWidget *button, gpointer data)
    {
    gEditor->saveSettings();
    return !gEditor->checkExitSave();
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    char const * const comments =
            "This is a simple editor that is part of the Oovcde project";
    gtk_show_about_dialog(Gui::getMainWindow(), "program-name", "OovEdit",
            "version", "Version " OOV_VERSION, "comments", comments, nullptr);
    }

extern "C" G_MODULE_EXPORT void on_FindImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gEditor->findDialog();
    }

extern "C" G_MODULE_EXPORT void on_FindNextMenuItem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->findAgain(true);
    }

extern "C" G_MODULE_EXPORT void on_FindPreviousMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->findAgain(false);
    }

extern "C" G_MODULE_EXPORT void on_FindEntry_activate(GtkEditable *editable,
        gpointer user_data)
    {
    GtkWidget *dlg = gEditor->getBuilder().getWidget("FindDialog");
    gtk_widget_hide(dlg);
    g_signal_emit_by_name(dlg, "response", GTK_RESPONSE_OK);
    }

extern "C" G_MODULE_EXPORT void on_FindInFilesMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    gEditor->findInFilesDialog();
    }

extern "C" G_MODULE_EXPORT bool on_FindTextview_button_press_event(
        GtkWidget *widget, GdkEvent *event, gpointer data)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
        {
        GtkWidget *widget = gEditor->getBuilder().getWidget("FindTextview");
        std::string line = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
        size_t pos = line.find(' ');
        if(pos != std::string::npos)
            {
            line.resize(pos);
            }
        gEditor->gotoFileLine(line);
        }
    return FALSE;
    }

extern "C" G_MODULE_EXPORT gboolean on_StackTextview_button_press_event(GtkWidget *widget,
        GdkEvent *event, gpointer user_data)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
        {
        GtkWidget *widget = gEditor->getBuilder().getWidget("StackTextview");
        OovString line = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
        gEditor->debugSetStackFrame(line);
        size_t pos = line.rfind(' ');
        if(pos != std::string::npos)
            {
            pos++;
            line.erase(0, pos);
            }
        gEditor->gotoFileLine(line);
        }
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_ControlTextview_key_press_event(GtkWidget *widget,
        GdkEvent *event, gpointer data)
    {
    switch(event->key.keyval)
        {
        case GDK_KEY_Return:
            {
            std::string str = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
            gEditor->getDebugger().sendCommand(str);
            }
            break;
        }
    return false;
    }

extern "C" G_MODULE_EXPORT void on_CutImagemenuitem_activate(GtkWidget *button,
        gpointer data)
    {
    gEditor->cut();
    }

extern "C" G_MODULE_EXPORT void on_CopyImagemenuitem_activate(GtkWidget *button,
        gpointer data)
    {
    gEditor->copy();
    }

extern "C" G_MODULE_EXPORT void on_PasteImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->paste();
    }

extern "C" G_MODULE_EXPORT void on_DeleteImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->deleteSel();
    }

extern "C" G_MODULE_EXPORT void on_UndoImageMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->undo();
    }

extern "C" G_MODULE_EXPORT void on_RedoImageMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->redo();
    }

extern "C" G_MODULE_EXPORT void on_EditPreferences_activate(GtkWidget *button, gpointer data)
    {
    gEditor->editPreferences();
    }

// For some reason, the click doesn't work, but pressing enter does.
extern "C" G_MODULE_EXPORT void on_DebugWorkingDirButton_activate(GtkWidget *button, gpointer data)
    {
    gEditor->setPreferencesWorkingDir();
    }

/*
// In Glade, use GObject, notify, and enter "active" in the Detail column.
extern "C" G_MODULE_EXPORT void on_DebugWorkingDirButton_active_notify()
    {
    gEditor->setPreferencesWorkingDir();
    }
*/

extern "C" G_MODULE_EXPORT void on_GoToDeclMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->gotoDeclaration();
    }

extern "C" G_MODULE_EXPORT void on_GoToDefMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->gotoDefinition();
    }

extern "C" G_MODULE_EXPORT void on_GoToLineMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->gotoLineDialog();
    }

extern "C" G_MODULE_EXPORT void on_GoToMethodMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->goToMethod();
    }

extern "C" G_MODULE_EXPORT void on_ViewClassDiagramMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->viewClassDiagram();
    }

extern "C" G_MODULE_EXPORT void on_ViewPortionDiagramMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor->viewPortionDiagram();
    }

extern "C" G_MODULE_EXPORT void on_LineNumberEntry_activate(GtkEditable *editable,
        gpointer user_data)
    {
    GtkWidget *dlg = gEditor->getBuilder().getWidget("GoToLineDialog");
    gtk_widget_hide(dlg);
    g_signal_emit_by_name(dlg, "response", GTK_RESPONSE_OK);
    }

extern "C" G_MODULE_EXPORT void on_MainAnalyzeToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    gEditor->analyze();
    }

extern "C" G_MODULE_EXPORT void on_MainBuildToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    gEditor->build();
    }

extern "C" G_MODULE_EXPORT void on_MainStopToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    gEditor->stopAnalyze();
    }
