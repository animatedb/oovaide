//============================================================================
// Name        : oovEdit.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
// Description : Simple Editor
//============================================================================

#include "Gui.h"
#include "FilePath.h"
#include "FileEditView.h"
#include "Version.h"
#include "Debugger.h"
#include <vector>


enum TextViewIndices { TVI_Header=0, TVI_Source=1 };

class Module
    {
    public:
	Module():
	    mLastEditView(nullptr)
	    {}
	void init(Builder &builder)
	    {
	    mFileEditView[TVI_Header].init(GTK_TEXT_VIEW(builder.getWidget("Edit1Textview")));
	    mFileEditView[TVI_Source].init(GTK_TEXT_VIEW(builder.getWidget("Edit3Textview")));
	    }
	void setEditTextView(int index)
	    {
	    mLastEditView = &mFileEditView[index];
	    gtk_widget_grab_focus(GTK_WIDGET(mLastEditView->getTextView()));
	    }
	void setFocusEditTextView(GtkTextView *editTextView)
	    {
	    if(mFileEditView[TVI_Header].getTextView() == editTextView)
		mLastEditView = &mFileEditView[TVI_Header];
	    else if(mFileEditView[TVI_Source].getTextView() == editTextView)
		mLastEditView = &mFileEditView[TVI_Source];
	    else
		mLastEditView = nullptr;
	    }
	bool checkExitSave()
	    {
	    bool exitOk = true;
	    if(mFileEditView[TVI_Header].getTextView())
		exitOk = mFileEditView[TVI_Header].checkExitSave();
	    if(mFileEditView[TVI_Source].getTextView())
		exitOk = mFileEditView[TVI_Source].checkExitSave();
	    return exitOk;
	    }
	FileEditView *getEditView()
	    {
	    return mLastEditView;
	    }

    private:
	FileEditView mFileEditView[2];
	FileEditView *mLastEditView;
    };

class Editor:public DebuggerListener
    {
    public:
	Editor():
	    mLastSearchCaseSensitive(false), mDesiredLine(-1)
	    {
	    mDebugger.setListener(*this);
	    /// @todo - need to get this from somewhere
	    mDebugger.setDebuggee("oovEdit");
	    g_idle_add(onIdle, this);
	    }
	void init()
	    {
	    mModule.init(mBuilder);
	    setStyle();
	    }
	static gboolean onIdle(gpointer data);
	void setFocusEditTextView(GtkTextView *editTextView)
	    { mModule.setFocusEditTextView(editTextView); }
	void openTextFile(char const * const fn)
	    {
	    FilePath moduleName(fn, FP_File);
	    FilePath cppExt("cpp", FP_Ext);
	    FilePath hExt("h", FP_Ext);

	    if(moduleName.matchExtension(hExt.c_str()) ||
		    moduleName.matchExtension(cppExt.c_str()))
		{
		mModule.setEditTextView(TVI_Header);
		moduleName.appendExtension("h");
		mModule.getEditView()->openTextFile(moduleName.c_str());

		mModule.setEditTextView(TVI_Source);
		moduleName.appendExtension("cpp");
		mModule.getEditView()->openTextFile(moduleName.c_str());
		}
	    else
		{
		mModule.setEditTextView(TVI_Source);
		mModule.getEditView()->openTextFile(fn);
		}

	    moduleName.discardExtension();
	    gtk_window_set_title(GTK_WINDOW(mBuilder.getWidget("MainWindow")),
		    moduleName.c_str());
	    }
	void openTextFile()
	    {
	    std::string filename;
	    PathChooser ch;
	    if(ch.ChoosePath(mModule.getEditView()->getWindow(), "Open File",
		    GTK_FILE_CHOOSER_ACTION_OPEN, filename))
		{
		openTextFile(filename.c_str());
		}
	    }
	bool saveTextFile()
	    {
	    bool saved = false;
	    if(mModule.getEditView())
		saved = mModule.getEditView()->saveTextFile();
	    return saved;
	    }
	bool saveAsTextFileWithDialog()
	    {
	    bool saved = false;
	    if(mModule.getEditView())
		saved = mModule.getEditView()->saveAsTextFileWithDialog();
	    return saved;
	    }
	void findDialog()
	    {
	    GtkEntry *entry = GTK_ENTRY(getBuilder().getWidget("FindEntry"));
	    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
	    Dialog dialog(GTK_DIALOG(getBuilder().getWidget("FindDialog")));
	    dialog.run();
	    }
	void findUsingDialogInfo()
	    {
	    GtkEntry *entry = GTK_ENTRY(getBuilder().getWidget("FindEntry"));
	    GtkToggleButton *downCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
		    "FindDownCheckbutton"));
	    GtkToggleButton *caseCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
		    "CaseSensitiveCheckbutton"));
	    find(gtk_entry_get_text(entry), gtk_toggle_button_get_active(downCheck),
		    gtk_toggle_button_get_active(caseCheck));
	    }
	void findAgain(bool forward)
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
/*
	void setTabs(int numSpaces)
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
	void setStyle()
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
	void cut()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->cut();
	    }
	void copy()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->copy();
	    }
	void paste()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->paste();
	    }
	void deleteSel()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->deleteSel();
	    }
	void undo()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->undo();
	    }
	void redo()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->redo();
	    }
	bool checkExitSave()
	    {
	    return mModule.checkExitSave();
	    }
	Builder &getBuilder()
	    { return mBuilder; }
	void gotoLine(int lineNum)
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->gotoLine(lineNum);
	    }
	void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
	        gchar *text, gint len)
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->bufferInsertText(textbuffer, location, text, len);
	    }
	void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
	        GtkTextIter *end)
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->bufferDeleteRange(textbuffer, start, end);
	    }
	void drawHighlight()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->drawHighlight();
	    }
	void idleHighlight()
	    {
	    if(mModule.getEditView())
		mModule.getEditView()->idleHighlight();
	    }
	bool handleIndentKeys(GdkEvent *event)
	    {
	    bool handled = false;
	    if(mModule.getEditView())
		handled = mModule.getEditView()->handleIndentKeys(event);
	    return handled;
	    }
	Debugger &getDebugger()
	    { return mDebugger; }
	virtual void DebugOutput(char const * const str)
	    {
	    mDebugOut.append(str);
	    }
	virtual void DebugStopped(char const * const fileName, int lineNum)
	    {
	    mDesiredLine = lineNum;
	    }

    private:
	Builder mBuilder;
	Module mModule;
	Debugger mDebugger;
	std::string mLastSearch;
	bool mLastSearchCaseSensitive;
	std::string mDebugOut;
	int mDesiredLine;
	void find(char const * const findStr, bool forward, bool caseSensitive)
	    {
	    mLastSearch = findStr;
	    mLastSearchCaseSensitive = caseSensitive;
	    if(mModule.getEditView())
		mModule.getEditView()->find(findStr, forward, caseSensitive);
	    }
	void setModuleName(const char *mn)
	    {
	    gtk_window_set_title(GTK_WINDOW(mBuilder.getWidget("MainWindow")), mn);
	    }
};


Editor gEditor;

gboolean Editor::onIdle(gpointer data)
    {
    if(gEditor.mDebugOut.length())
	{
	GtkTextView *view = GTK_TEXT_VIEW(gEditor.getBuilder().
		getWidget("Edit2Textview"));
	Gui::appendText(view, gEditor.mDebugOut.c_str());
	gEditor.mDebugOut.clear();
	}
    if(gEditor.mDesiredLine != -1)
	{
	gEditor.gotoLine(gEditor.mDesiredLine);
	gEditor.mDesiredLine = -1;
	}
    return true;
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

//gboolean draw(GtkWidget *widget, CairoContext *cr, gpointer user_data)
gboolean draw(GtkWidget *widget, void *cr, gpointer user_data)
    {
    gEditor.drawHighlight();
    return FALSE;
    }
/*
static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
    gEditor.setNeedHighlightUpdate(true);
    }
*/

static gboolean OnIdle(gpointer data)
    {
    gEditor.idleHighlight();
    return TRUE;
    }

int main(int argc, char *argv[])
    {
    gtk_init (&argc, &argv);
    if(gEditor.getBuilder().addFromFile("oovEdit.glade"))
	{
	gEditor.init();
	gEditor.getBuilder().connectSignals();
	GtkWidget *window = gEditor.getBuilder().getWidget("MainWindow");
	char const * fn = nullptr;
	int line = 0;
	for(int argi=1; argi<argc; argi++)
	    {
	    if(argv[argi][0] == '+')
		{
		sscanf(&argv[argi][1], "%d", &line);
		}
	    else
		fn = argv[argi];
	    }
	if(fn)
	    {
	    gEditor.openTextFile(fn);
	    gEditor.gotoLine(line);
	    }
	else
	    {
	    fprintf(stderr, "Unable to open file %s\n", fn);
	    }
	gtk_widget_show(window);
	g_idle_add(OnIdle, NULL);
	gtk_main();
	}
    else
	{
	Gui::messageBox("The file oovEdit.glade must be in the executable directory.");
	}
    return 0;
    }

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
    return !gEditor.checkExitSave();
    }

extern "C" G_MODULE_EXPORT void on_HelpAboutImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    char const * const comments =
	    "This is a simple editor";
    gtk_show_about_dialog(nullptr, "program-name", "OovEdit",
	    "version", "Version " OOV_VERSION, "comments", comments, nullptr);
    }

extern "C" G_MODULE_EXPORT void on_FindImagemenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gEditor.findDialog();
    }

extern "C" G_MODULE_EXPORT void on_FindDialog_delete_event(GtkWidget *button, gpointer data)
    {
    gtk_widget_hide(gEditor.getBuilder().getWidget("FindDialog"));
    }

extern "C" G_MODULE_EXPORT void on_FindCancelButton_clicked(GtkWidget *button, gpointer data)
    {
    gtk_widget_hide(gEditor.getBuilder().getWidget("FindDialog"));
    }

extern "C" G_MODULE_EXPORT void on_FindOkButton_clicked(GtkWidget *button, gpointer data)
    {
    gtk_widget_hide(gEditor.getBuilder().getWidget("FindDialog"));
    gEditor.findUsingDialogInfo();
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
    gtk_widget_hide(gEditor.getBuilder().getWidget("FindDialog"));
    gEditor.findUsingDialogInfo();
    }

extern "C" G_MODULE_EXPORT void on_CutImagemenuitem_activate(GtkWidget *button, gpointer data)
    {
    gEditor.cut();
    }

extern "C" G_MODULE_EXPORT void on_CopyImagemenuitem_activate(GtkWidget *button, gpointer data)
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

extern "C" G_MODULE_EXPORT gboolean on_Edit1Textview_focus_in_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    gEditor.setFocusEditTextView(GTK_TEXT_VIEW(widget));
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_Edit2Textview_focus_in_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    gEditor.setFocusEditTextView(GTK_TEXT_VIEW(widget));
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_Edit3Textview_focus_in_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    gEditor.setFocusEditTextView(GTK_TEXT_VIEW(widget));
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_Edit1Textview_key_press_event(GtkWidget *widget,
	GdkEvent *event, gpointer data)
    {
    return gEditor.handleIndentKeys(event);
    }

extern "C" G_MODULE_EXPORT gboolean on_Edit2Textview_key_press_event(GtkWidget *widget,
	GdkEvent *event, gpointer data)
    {
    switch(event->key.keyval)
	{
	case GDK_KEY_Return:
	    std::string str = Gui::getCurrentLineText(GTK_TEXT_VIEW(widget));
	    gEditor.getDebugger().sendCommand(str.c_str());
	    break;
	}
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_Edit3Textview_key_press_event(GtkWidget *widget,
	GdkEvent *event, gpointer data)
    {
    return gEditor.handleIndentKeys(event);
    }
