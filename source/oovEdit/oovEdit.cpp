//============================================================================
// Name        : oovEdit.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
// Description : Simple Editor
//============================================================================

#include "FilePath.h"
#include "Version.h"
#include "Debugger.h"
#include "oovEdit.h"
#include <vector>



Editor::Editor():
    mLastSearchCaseSensitive(false)
    {
    mDebugger.setListener(*this);
    /// @todo - need to get this from somewhere
    mDebugger.setDebuggee("oovEdit");
    g_idle_add(onIdle, this);
    }

void Editor::init()
    {
    GtkWidget *book1 = mBuilder.getWidget("EditNotebook1");
    GtkWidget *book2 = mBuilder.getWidget("EditNotebook2");
    mEditFiles.init(GTK_NOTEBOOK(book1), GTK_NOTEBOOK(book2));
    setStyle();
    }

void Editor::openTextFile(char const * const fn)
    {
    mEditFiles.viewFile(fn, 1);
    }

void Editor::openTextFile()
    {
    std::string filename;
    PathChooser ch;
    if(ch.ChoosePath(mEditFiles.getEditView()->getWindow(), "Open File",
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

void Editor::findDialog()
    {
    GtkEntry *entry = GTK_ENTRY(getBuilder().getWidget("FindEntry"));
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    Dialog dialog(GTK_DIALOG(getBuilder().getWidget("FindDialog")));
    dialog.run();
    }

void Editor::findUsingDialogInfo()
    {
    GtkEntry *entry = GTK_ENTRY(getBuilder().getWidget("FindEntry"));
    GtkToggleButton *downCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
	    "FindDownCheckbutton"));
    GtkToggleButton *caseCheck = GTK_TOGGLE_BUTTON(getBuilder().getWidget(
	    "CaseSensitiveCheckbutton"));
    find(gtk_entry_get_text(entry), gtk_toggle_button_get_active(downCheck),
	    gtk_toggle_button_get_active(caseCheck));
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


Editor gEditor;

gboolean Editor::onIdle(gpointer data)
    {
    if(gEditor.mDebugOut.length())
	{
	GtkTextView *view = GTK_TEXT_VIEW(gEditor.getBuilder().
		getWidget("ControlTextview"));
	Gui::appendText(view, gEditor.mDebugOut.c_str());
	gEditor.mDebugOut.clear();
	}
    gEditor.getEditFiles().onIdle();
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
	    gEditor.getEditFiles().viewFile(fn, line);
	    }
	else
	    {
	    fprintf(stderr, "Unable to open file %s\n", fn);
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

extern "C" G_MODULE_EXPORT gboolean on_ControlTextview_key_press_event(GtkWidget *widget,
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

