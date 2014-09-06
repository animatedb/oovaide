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
    g_idle_add(onIdle, this);
    }

void Editor::openTextFile(char const * const fn)
    {
    mEditFiles.viewModule(fn, 1);
    }

void Editor::openTextFile()
    {
    std::string filename;
    PathChooser ch;
    if(ch.ChoosePath(GTK_WINDOW(mBuilder.getWidget("MainWindow")), "Open File",
	    GTK_FILE_CHOOSER_ACTION_OPEN, filename))
	{
	openTextFile(filename.c_str());
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
	saved = mEditFiles.getEditView()->saveAsTextFileWithDialog();
    return saved;
    }

class FindDialog:public Dialog
    {
    public:
	FindDialog(EditFiles &editFiles):
	    Dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("FindDialog")),
		    Gui::getWindow(Builder::getBuilder()->getWidget("MainWindow")))
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
		Gui::setText(entry, viewText.c_str());
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
    Dialog dialog(GTK_DIALOG(Builder::getBuilder()->getWidget("GoToLineDialog")),
	    Gui::getWindow(Builder::getBuilder()->getWidget("MainWindow")));
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
	find(mLastSearch.c_str(), forward, mLastSearchCaseSensitive);
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
	virtual bool processFile(const std::string &filePath);

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
bool FindFiles::processFile(const std::string &filePath)
    {
    bool success = true;
    FilePath ext(filePath, FP_File);

    if(isHeader(ext.c_str()) || isSource(ext.c_str()))
	{
	FILE *fp = fopen(filePath.c_str(), "r");
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
//		    matchStr += '\n';
		    Gui::appendText(mView, matchStr.c_str());
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
    Gui::appendText(view, matchStr.c_str());
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
	findInFiles(gtk_entry_get_text(entry), Project::getSrcRootDirectory().c_str(),
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
	OovString numStr(lineBuf.substr(pos+1, lineBuf.length()-pos).c_str());
	if(numStr.getInt(0, INT_MAX, lineNum))
	    {
	    if(lineNum == 0)
		lineNum = 1;
	    mEditFiles.viewFile(lineBuf.substr(0, pos).c_str(), lineNum);
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

    const gchar *styleFile = "OovEdit.css";
    GError *err = 0;
    gtk_css_provider_load_from_path(provider, styleFile, &err);
    gtk_style_context_add_provider_for_screen (screen,
	GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    }

void Editor::find(char const * const findStr, bool forward, bool caseSensitive)
    {
    mLastSearch = findStr;
    mLastSearchCaseSensitive = caseSensitive;
    if(mEditFiles.getEditView())
	mEditFiles.getEditView()->find(findStr, forward, caseSensitive);
    }

void Editor::findAndReplace(char const * const findStr, bool forward, bool caseSensitive,
	char const * const replaceStr)
    {
    mLastSearch = findStr;
    mLastSearchCaseSensitive = caseSensitive;
    if(mEditFiles.getEditView())
	mEditFiles.getEditView()->findAndReplace(findStr, forward, caseSensitive, replaceStr);
    }


Editor gEditor;

gboolean Editor::onIdle(gpointer data)
    {
    if(gEditor.mDebugOut.length())
	{
	GtkTextView *view = GTK_TEXT_VIEW(gEditor.getBuilder().
		getWidget("ControlTextview"));
	Gui::appendText(view, gEditor.mDebugOut.c_str());
	Gui::scrollToCursor(view);
	gEditor.mDebugOut.clear();
	}
    Debugger::eChangeStatus dbgStatus = gEditor.mDebugger.getChangeStatus();
    if(dbgStatus != Debugger::CS_None)
	{
	gEditor.idleDebugStatusChange(dbgStatus);
	}
    gEditor.getEditFiles().onIdle();
    sleepMs(5);
    return true;
    }

void Editor::idleDebugStatusChange(Debugger::eChangeStatus st)
    {
    if(st == Debugger::CS_RunState)
	{
	getEditFiles().updateDebugMenu();
	if(mDebugger.getChildState() == GCS_GdbChildPaused)
	    {
	    auto const &loc = mDebugger.getStoppedLocation();
	    mEditFiles.viewModule(loc.getFilename().c_str(), loc.getLine());
	    mDebugger.startGetStack();
	    }
	}
    else if(st == Debugger::CS_Stack)
	{
	GtkTextView *view = GTK_TEXT_VIEW(mBuilder.getWidget("StackTextview"));
	Gui::setText(view, mDebugger.getStack().c_str());
	}
    else if(st == Debugger::CS_Value)
	{
	mVarView.appendText(GuiTreeItem(), mDebugger.getVarValue().c_str());
	}
    }

void Editor::debugSetStackFrame(char const * const frameLine)
    {
    mDebugger.setStackFrame(frameLine);
    }

void signalBufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len, gpointer user_data)
    {
    gEditor.bufferInsertText(textbuffer, location, text, len);
    }

void signalBufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end, gpointer user_data)
    {
    gEditor.bufferDeleteRange(textbuffer, start, end);
    }

/*
// This indicates if a scroll occurred.
//gboolean draw(GtkWidget *widget, CairoContext *cr, gpointer user_data)
gboolean draw(GtkWidget *widget, void *cr, gpointer user_data)
    {
    gEditor.drawHighlight();
    return FALSE;
    }
*/
/*
static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
    gEditor.setNeedHighlightUpdate(true);
    }
*/

void Editor::loadSettings()
    {
    int width, height;
    mEditOptions.setProjectDir(mProjectDir);
    if(mEditOptions.getScreenSize(width, height))
	{
	gtk_window_resize(GTK_WINDOW(mBuilder.getWidget("MainWindow")), width, height);
	}
    Project::setProjectDirectory(mProjectDir.c_str());
    }

void Editor::saveSettings()
    {
    int width, height;
    gtk_window_get_size(GTK_WINDOW(mBuilder.getWidget("MainWindow")), &width, &height);
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
	if(compFile.getComponentType(name.c_str()) == ComponentTypesFile::CT_Program)
	    {
	    FilePath fp(Project::getOutputDir(BuildConfigDebug), FP_Dir);
	    fp.appendFile(makeExeFilename(name.c_str()).c_str());
	    Gui::appendText(cb, fp.c_str());
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
	    Gui::appendText(cb, dbgComponent.c_str());
	    compIndex = boxCount;
	    }
	}
    if(compIndex != -1)
	{
	Gui::setSelected(cb, compIndex);
	}
    GtkEntry *debuggeeArgs = GTK_ENTRY(mBuilder.getWidget("CommandLineArgsEntry"));
    Gui::setText(debuggeeArgs, mEditOptions.getValue(OptEditDebuggeeArgs).c_str());
    GtkEntry *workDirEntry = GTK_ENTRY(mBuilder.getWidget("DebugWorkingDirEntry"));
    Gui::setText(workDirEntry, mEditOptions.getValue(OptEditDebuggerWorkingDir).c_str());

    Dialog dlg(GTK_DIALOG(mBuilder.getWidget("Preferences")),
	    GTK_WINDOW(mBuilder.getWidget("MainWindow")));
    if(dlg.run(true))
	{
	if(haveNames)
	    {
	    std::string text = Gui::getText(cb);
	    mEditOptions.setNameValue(OptEditDebuggee, text.c_str());
	    mEditOptions.setNameValue(OptEditDebuggeeArgs, Gui::getText(debuggeeArgs));
	    mEditOptions.setNameValue(OptEditDebuggerWorkingDir, Gui::getText(workDirEntry));
//	    mDebugger.setDebuggee(text.c_str());
//	    mDebugger.setDebuggeeArgs(mEditOptions.getValue(OptEditDebuggeeArgs).c_str());
	    }
	else
	    Gui::messageBox("Some components for the project must be defined as programs");
	}
    }

void Editor::setPreferencesWorkingDir()
    {
    PathChooser chooser;
    chooser.setDefaultPath(Project::getOutputDir(BuildConfigDebug).c_str());
    std::string dir;
    if(chooser.ChoosePath(GTK_WINDOW(mBuilder.getWidget("MainWindow")),
	    "Select Working Directory",
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, dir))
	{
	GtkEntry *workDirEntry = GTK_ENTRY(mBuilder.getWidget("DebugWorkingDirEntry"));
	Gui::setText(workDirEntry, dir.c_str());
	}
    }


#define SINGLE_INSTANCE 1
#if(SINGLE_INSTANCE)

// Note that this is called from a few places. If command line arguments are
// given, then it is called from the open, otherwise it is called from
// g_application_run
static void activateApp(GApplication *gapp)
    {
    GtkApplication *app = GTK_APPLICATION (gapp);
    GtkWidget *window = gEditor.getBuilder().getWidget("MainWindow");
    gEditor.loadSettings();
    gtk_widget_show_all(window);
    gtk_application_add_window(app, GTK_WINDOW(window));
    }

// command-line signal
// Beware - this is called many times when starting up.
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
		    std::string fname = fixFilePath(&argv[argi][2]);
		    gEditor.setProjectDir(fname.c_str());
		    }
		else
		    sscanf(&argv[argi][1], "%d", &line);
		}
	    else if(argv[argi][0] == '-')
		{
		if(argv[argi][1] == 'p')
		    {
		    std::string fname = fixFilePath(&argv[argi][2]);
		    gEditor.setProjectDir(fname.c_str());
		    }
		else
		    goodArgs = false;
		}
	    else
		fn = argv[argi];
	    }
	if(fn)
	    {
	    gEditor.getEditFiles().viewModule(fixFilePath(fn).c_str(), line);
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
	Gui::messageBox(str.c_str());
	}
    }

static void startupApp(GApplication *gapp)
    {
    if(gEditor.getBuilder().addFromFile("oovEdit.glade"))
	{
	gEditor.init();
	gEditor.getBuilder().connectSignals();
	}
    }

int main(int argc, char **argv)
    {
    GtkApplication *app = gtk_application_new("org.oovcde.oovEdit",
	    G_APPLICATION_HANDLES_COMMAND_LINE);
    GApplication *gapp = G_APPLICATION(app);

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

int main(int argc, char *argv[])
    {
    gtk_init (&argc, &argv);
    if(gEditor.getBuilder().addFromFile("oovEdit.glade"))
	{
	gEditor.init();
	gEditor.getBuilder().connectSignals();
	GtkWidget *window = gEditor.getBuilder().getWidget("MainWindow");
	char const * fn = nullptr;
        char const *proj = nullptr;
	int line = 0;
	for(int argi=1; argi<argc; argi++)
	    {
	    if(argv[argi][0] == '+')
		{
		sscanf(&argv[argi][1], "%d", &line);
		}
	    else if(argv[argi][0] == '-')
		{
		proj = &argv[argi][2];
		}
	    else
		fn = argv[argi];
	    }
	if(fn)
	    {
	    gEditor.getEditFiles().viewModule(fn, line);
	    }
	else
	    {
            fprintf(stderr, "OovEdit version %s\n", OOV_VERSION);
            fprintf(stderr, "oovEdit: Args are: filename [args]...\n");
            fprintf(stderr, "args are:\n");
            fprintf(stderr, "   +<line>            line number of opened file\n");
            fprintf(stderr, "   -p<projectDir>    directory of project files\n");
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
    gEditor.openTextFile();
    }

extern "C" G_MODULE_EXPORT void on_FileSaveImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.saveTextFile();
    }

extern "C" G_MODULE_EXPORT void on_FileSaveAsImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.saveAsTextFileWithDialog();
    }

extern "C" G_MODULE_EXPORT bool on_FileQuitImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    return !gEditor.checkExitSave();
    }

extern "C" G_MODULE_EXPORT gboolean on_MainWindow_delete_event(GtkWidget *button, gpointer data)
    {
    gEditor.saveSettings();
    return !gEditor.checkExitSave();
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    char const * const comments =
	    "This is a simple editor that is part of the Oovcde project";
    gtk_show_about_dialog(nullptr, "program-name", "OovEdit",
	    "version", "Version " OOV_VERSION, "comments", comments, nullptr);
    }

extern "C" G_MODULE_EXPORT void on_FindImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gEditor.findDialog();
    }

extern "C" G_MODULE_EXPORT void on_FindNextMenuItem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.findAgain(true);
    }

extern "C" G_MODULE_EXPORT void on_FindPreviousMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.findAgain(false);
    }

extern "C" G_MODULE_EXPORT void on_FindEntry_activate(GtkEditable *editable,
        gpointer user_data)
    {
    GtkWidget *dlg = gEditor.getBuilder().getWidget("FindDialog");
    gtk_widget_hide(dlg);
    g_signal_emit_by_name(dlg, "response", GTK_RESPONSE_OK);
    }

extern "C" G_MODULE_EXPORT void on_FindInFilesMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gEditor.findInFilesDialog();
    }

extern "C" G_MODULE_EXPORT bool on_FindTextview_button_press_event(
	GtkWidget *widget, GdkEvent *event, gpointer data)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
	{
	GtkWidget *widget = gEditor.getBuilder().getWidget("FindTextview");
	std::string line = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
	size_t pos = line.find(' ');
	if(pos != std::string::npos)
	    {
	    line.resize(pos);
	    }
	gEditor.gotoFileLine(line);
	}
    return FALSE;
    }

extern "C" G_MODULE_EXPORT gboolean on_StackTextview_button_press_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    GdkEventButton *buttEvent = reinterpret_cast<GdkEventButton *>(event);
    if(buttEvent->type == GDK_2BUTTON_PRESS)
	{
	GtkWidget *widget = gEditor.getBuilder().getWidget("StackTextview");
	std::string line = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
	gEditor.debugSetStackFrame(line.c_str());
	size_t pos = line.rfind(' ');
	if(pos != std::string::npos)
	    {
	    pos++;
	    line.erase(0, pos);
	    }
	gEditor.gotoFileLine(line);
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
	    gEditor.getDebugger().sendCommand(str.c_str());
	    }
	    break;
	}
    return false;
    }

extern "C" G_MODULE_EXPORT void on_CutImagemenuitem_activate(GtkWidget *button,
	gpointer data)
    {
    gEditor.cut();
    }

extern "C" G_MODULE_EXPORT void on_CopyImagemenuitem_activate(GtkWidget *button,
	gpointer data)
    {
    gEditor.copy();
    }

extern "C" G_MODULE_EXPORT void on_PasteImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.paste();
    }

extern "C" G_MODULE_EXPORT void on_DeleteImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.deleteSel();
    }

extern "C" G_MODULE_EXPORT void on_UndoImageMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.undo();
    }

extern "C" G_MODULE_EXPORT void on_RedoImageMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.redo();
    }

extern "C" G_MODULE_EXPORT void on_EditPreferences_activate(GtkWidget *button, gpointer data)
    {
    gEditor.editPreferences();
    }

extern "C" G_MODULE_EXPORT void on_DebugWorkingDirButton_activate(GtkWidget *button, gpointer data)
    {
    gEditor.setPreferencesWorkingDir();
    }

extern "C" G_MODULE_EXPORT void on_GoToDeclMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.gotoDeclaration();
    }

extern "C" G_MODULE_EXPORT void on_GoToDefMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.gotoDefinition();
    }

extern "C" G_MODULE_EXPORT void on_GoToLineMenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.gotoLineDialog();
    }

extern "C" G_MODULE_EXPORT void on_LineNumberEntry_activate(GtkEditable *editable,
        gpointer user_data)
    {
    GtkWidget *dlg = gEditor.getBuilder().getWidget("GoToLineDialog");
    gtk_widget_hide(dlg);
    g_signal_emit_by_name(dlg, "response", GTK_RESPONSE_OK);
    }
