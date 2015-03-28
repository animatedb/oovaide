/*
 * FileEditView.cpp
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "FileEditView.h"
#include "FilePath.h"
#include "File.h"
#include "OovString.h"
#include "Builder.h"
#include <string.h>


#include "IncludeMap.h"
#include "BuildConfigReader.h"
#include "OovProcess.h"
// This currently does not get the package command line arguments,
// but usually these won't be needed for compilation.
static void getCppArgs(OovStringRef const srcName, OovProcessChildArgs &args)
    {
    ProjectReader proj;
    proj.readOovProject(Project::getProjectDirectory(), BuildConfigAnalysis);
    OovStringVec cppArgs = proj.getCompileArgs();
    for(auto const &arg : cppArgs)
	{
	args.addArg(arg);
	}

    BuildConfigReader cfg;
    std::string incDepsPath = cfg.getIncDepsFilePath(BuildConfigAnalysis);
    IncDirDependencyMapReader incDirMap;
    incDirMap.read(incDepsPath);
    OovStringVec incDirs = incDirMap.getNestedIncludeDirsUsedBySourceFile(srcName);
    for(auto const &dir : incDirs)
	{
	std::string arg = "-I";
	arg += dir;
	args.addArg(arg);
	}
    }



void signalBufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len, gpointer user_data);
void signalBufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end, gpointer user_data);

//static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data);
//gboolean draw(GtkWidget *widget, void *cr, gpointer user_data);


#if(COMPLETION_WINDOW)
void CompletionList::init(GtkTextBuffer *textBuffer)
    {
    mTopWidget = Builder::getBuilder()->getWidget("CompletionListWindow");
    mGuiList.init(*Builder::getBuilder(), "CompletionListTreeview", "");
    mTextBuffer = textBuffer;
    }

static bool isIdentC(int key)
    {
    return (isalnum(key) || key == '_');
    }

static bool isInsertableC(int key)
    {
    bool ctrl = iscntrl(key);
    return (!ctrl && key < 0x80);
    }

static bool isListKey(int key)
    {
    return(key == GDK_KEY_Down || key == GDK_KEY_Up ||
	    key == GDK_KEY_KP_Down || key == GDK_KEY_KP_Up ||
	    key == GDK_KEY_Return);
    }

static bool isModifierKey(int key)
    {
    return(key == GDK_KEY_Shift_R || key == GDK_KEY_Shift_L);
    }

static CompletionList *sCurrentCompleteList;
void CompletionList::handleEditorKey(int key)
    {
    sCurrentCompleteList = this;
    if(key == '.' || (mLastKey == '-' && key == '>'))
	{
	mCompletionTriggerPointOffset = GuiTextBuffer::getCursorOffset(mTextBuffer);
	mGuiList.clear();
	Gui::setVisible(mTopWidget, true);
	}
    if(!isModifierKey(key))
	{
	mLastKey = key;
	}
    }


/*
 * Classes of keys:
 * 	List control keys (arrows, return)	!quit list,	!modify editbuf,	list movement,
 * 	Modifier keys (shift)			!quit list,	!modify editbuf,
 * 	Delete/backspace			!quit list, 	modify editbuf
 * 	Non-insertable (esc)			quit list,	!modify editbuf
 * 	punctuation (parens, comma)		quit list,	modify editbuf
 *	identifiers				!quit list,	modify editbuf
 */
/// @todo - Need to handle searching list
/// @todo - Need to position completion window
bool CompletionList::handleListKey(int key)
    {
    bool handled = true;
    bool doneList = true;

    if(key == GDK_KEY_BackSpace)
	{
	GtkTextIter iter = GuiTextBuffer::getCursorIter(mTextBuffer);
	gtk_text_buffer_backspace(mTextBuffer, &iter, true, true);
	doneList = (GuiTextBuffer::getCursorOffset(mTextBuffer) ==
		mCompletionTriggerPointOffset);
	}
    else if(key == GDK_KEY_Delete || key == GDK_KEY_KP_Delete)
	{
	GtkTextIter iter = GuiTextBuffer::getCursorIter(mTextBuffer);
	GtkTextIter endIter = iter;
	if(gtk_text_iter_forward_char(&endIter))
	    {
	    gtk_text_buffer_delete(mTextBuffer, &iter, &endIter);
	    }
	}
    else if(isListKey(key) || isModifierKey(key))
	{
	handled = false;
	doneList = false;
	}
    else if(isInsertableC(key))
	{
	// Punctuation and other things need to be inserted.
	char keyStr[1];
	keyStr[0] = key;
	gtk_text_buffer_insert_at_cursor(mTextBuffer, keyStr, 1);
	if(isIdentC(key))
	    {
	    doneList = false;
	    }
	}
    if(Gui::isVisible(mTopWidget))
	{
	int cursOffset = GuiTextBuffer::getCursorOffset(mTextBuffer);
	OovString str = GuiTextBuffer::getText(mTextBuffer,
		mCompletionTriggerPointOffset+1, cursOffset);
	mGuiList.setSelected(mGuiList.findLongestMatch(str));
	}
    if(doneList)
	{
	Gui::setVisible(mTopWidget, false);
	}
    return handled;
    }

void CompletionList::lostFocus()
    {
    Gui::setVisible(mTopWidget, false);
    }

void CompletionList::setList(OovStringVec const &strs)
    {
    mGuiList.clear();
    for(auto const &str : strs)
	{
	mGuiList.appendText(str);
	}
    if(strs.size())
	{
	mGuiList.setSelected(strs[0]);
	}
/*
    GtkWidget *sw = Builder::getBuilder()->getWidget("CompletionListScrolledwindow");
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sw), -1);
*/
//    gtk_widget_queue_resize(GTK_WIDGET(mGuiList.getTreeView()));
//    gtk_widget_queue_resize(sw);
//    gtk_widget_queue_resize(mTopWidget);
/*
    GtkWidget *w;
    GtkRequisition min, nat;

    w = mTopWidget;
    gtk_widget_get_preferred_size(w, &min, &nat);
    printf("top %d %d\n", min.height, nat.height);

    w = Builder::getBuilder()->getWidget("CompletionListScrolledwindow");
    gtk_widget_get_preferred_size(w, &min, &nat);
    printf("scrollw %d %d\n", min.height, nat.height);

    w = GTK_WIDGET(mGuiList.getTreeView());
    gtk_widget_get_preferred_size(w, &min, &nat);
    printf("treeview %d %d\n", min.height, nat.height);

    gtk_widget_set_size_request(w, 0, 0);
//    gtk_widget_set_preferred_size(w, 0, 0);

    printf("--\n");

    fflush(stdout);
*/
    }

void CompletionList::resize()
    {
    GtkWidget *sw = Builder::getBuilder()->getWidget("CompletionListScrolledwindow");
    Gui::resizeParentScrolledWindow(GTK_WIDGET(mGuiList.getTreeView()),
	    GTK_SCROLLED_WINDOW(sw));
    }

void CompletionList::activateSelectedItem()
    {
    OovString str = mGuiList.getSelected();
    gtk_text_buffer_insert_at_cursor(mTextBuffer, str.getStr(), str.length());
    Gui::setVisible(mTopWidget, false);
    }

#endif


void FileEditView::init(GtkTextView *textView)
    {
    mTextView = textView;
    mTextBuffer = gtk_text_view_get_buffer(mTextView);
    mIndenter.init(mTextBuffer);
    mCompleteList.init(mTextBuffer);
    gtk_widget_grab_focus(GTK_WIDGET(mTextView));
    g_signal_connect(G_OBJECT(mTextBuffer), "insert-text",
          G_CALLBACK(signalBufferInsertText), NULL);
    g_signal_connect(G_OBJECT(mTextBuffer), "delete-range",
          G_CALLBACK(signalBufferDeleteRange), NULL);
//	    g_signal_connect(G_OBJECT(mBuilder.getWidget("EditTextScrolledwindow")),
//		    "scroll-child", G_CALLBACK(scrollChild), NULL);
//    g_signal_connect(G_OBJECT(mTextView), "draw", G_CALLBACK(draw), NULL);
    }

bool FileEditView::openTextFile(OovStringRef const fn)
    {
    setFileName(fn);
    File file(fn, "rb");
    if(file.isOpen())
	{
	fseek(file.getFp(), 0, SEEK_END);
	long fileSize = ftell(file.getFp());
	fseek(file.getFp(), 0, SEEK_SET);
	std::vector<char> buf(fileSize);
	// actualCount can be less than fileSize on Windows due to /r/n
	int actualCount = fread(&buf.front(), 1, fileSize, file.getFp());
	if(actualCount > 0)
	    {
	    gtk_text_buffer_set_text(mTextBuffer, &buf.front(), actualCount);
	    gtk_text_buffer_set_modified(mTextBuffer, FALSE);
	    }
	highlightRequest();
	}
    return(file.isOpen());
    }

bool FileEditView::saveTextFile()
    {
    bool success = false;
    if(mFileName.length() == 0)
	{
	success = saveAsTextFileWithDialog();
	}
    else
	success = true;
    if(success)
	success = saveAsTextFile(mFileName);
    return success;
    }

GuiText FileEditView::getBuffer()
    {
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(mTextBuffer, &start, &end);
    return GuiText(gtk_text_buffer_get_text(mTextBuffer, &start, &end, false));
    }

void FileEditView::highlightRequest()
    {
    if(mFileName.length())
	{
//	int numArgs = 0;
//	char const * cppArgv[40];
	OovProcessChildArgs cppArgs;
	getCppArgs(mFileName, cppArgs);

	FilePath path;
	path.getAbsolutePath(mFileName, FP_File);
	mHighlighter.highlightRequest(path, cppArgs.getArgv(), cppArgs.getArgc());
	}
    }

void FileEditView::gotoLine(int lineNum)
    {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &iter, lineNum-1);
    // Moves the insert and selection_bound marks.
    gtk_text_buffer_place_cursor(mTextBuffer, &iter);
    moveToIter(iter);
    }

void FileEditView::moveToIter(GtkTextIter startIter, GtkTextIter *endIter)
    {
    GtkTextMark *mark = gtk_text_buffer_get_insert(mTextBuffer);
    if(mark)
	{
//		The solution involves creating an idle proc (which is a procedure which
//		GTK will call when it has nothing else to do until told otherwise by the
//		return value of this procedure). In that idle proc the source view is
//		scrolled constantly, until it is determined that the source view has in
//		fact scrolled to the location desired. This was accomplished by using a couple of nice functions:
//		    gtk_get_iter_location to get the location of the cursor.
//		    gtk_text_view_get_visible_rect to get the rectangle of the visible part of the document in the source view.
//		    gtk_intersect to check whether the cursor is in the visible rectangle.
//		gtk_text_buffer_get_iter_at_mark(mTextBuffer, &iter, mark);

	GtkTextIter tempEndIter = startIter;
	if(endIter)
	    tempEndIter = *endIter;
	gtk_text_buffer_select_range(mTextBuffer, &startIter, &tempEndIter);
	gtk_text_buffer_move_mark(mTextBuffer, mark, &startIter);
	Gui::scrollToCursor(mTextView);
//	gtk_text_view_scroll_to_mark(mTextView, mark, 0, TRUE, 0, 0.5);
	}
    }

bool FileEditView::saveAsTextFileWithDialog()
    {
    PathChooser ch;
    OovString filename = mFileName;
    bool saved = false;
    std::string prompt = "Save ";
    prompt += mFileName;
    prompt += " As";
    if(ch.ChoosePath(GTK_WINDOW(mTextView), prompt,
	    GTK_FILE_CHOOSER_ACTION_SAVE, filename))
	{
	saved = saveAsTextFile(filename);
	}
    return saved;
    }

bool FileEditView::saveAsTextFile(OovStringRef const fn)
    {
    size_t writeSize = -1;
    OovString tempFn = fn;
    setFileName(fn);
    tempFn += ".tmp";
    File file(tempFn, "wb");
    if(file.isOpen())
	{
	int size = gtk_text_buffer_get_char_count(mTextBuffer);
	GuiText buf = getBuffer();
	writeSize = fwrite(buf.c_str(), 1, size, file.getFp());
	file.close();
	FileDelete(fn);
	FileRename(tempFn, fn);
	gtk_text_buffer_set_modified(mTextBuffer, FALSE);
	}
    return(writeSize > 0);
    }

bool FileEditView::checkExitSave()
    {
    bool exitOk = true;
    if(gtk_text_buffer_get_modified(mTextBuffer))
	{
	std::string prompt = "Save ";
	prompt += mFileName;
	GtkDialog *dlg = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", prompt.c_str()));
	gtk_dialog_add_button(dlg, GUI_CANCEL, GTK_RESPONSE_CANCEL);
	gint result = gtk_dialog_run(dlg);
	if(result == GTK_RESPONSE_YES)
	    {
	    if(mFileName.length() > 0)
		{
		exitOk = saveTextFile();
		}
	    else
		{
		exitOk = saveAsTextFileWithDialog();
		}
	    }
	else if(result == GTK_RESPONSE_NO)
	    exitOk = true;
	else
	    exitOk = false;
	gtk_widget_destroy(GTK_WIDGET(dlg));
	}
    return exitOk;
    }

bool FileEditView::find(char const * const findStr, bool forward, bool caseSensitive)
    {
    GtkTextMark *mark = gtk_text_buffer_get_insert(mTextBuffer);
    GtkTextIter startFind;
    gtk_text_buffer_get_iter_at_mark(mTextBuffer, &startFind, mark);
    GtkTextIter startMatch;
    GtkTextIter endMatch;
    GtkTextSearchFlags flags = static_cast<GtkTextSearchFlags>(GTK_TEXT_SEARCH_TEXT_ONLY |
		GTK_TEXT_SEARCH_VISIBLE_ONLY);
    if(!caseSensitive)
	{
	flags = static_cast<GtkTextSearchFlags>(flags | GTK_TEXT_SEARCH_CASE_INSENSITIVE);
	}
    bool found = false;
    if(forward)
	{
	gtk_text_iter_forward_char(&startFind);
	found = gtk_text_iter_forward_search(&startFind, findStr, flags,
		&startMatch, &endMatch, NULL);
	}
    else
	{
	gtk_text_iter_backward_char(&startFind);
	found = gtk_text_iter_backward_search(&startFind, findStr, flags,
		&startMatch, &endMatch, NULL);
	}
    if(found)
	moveToIter(startMatch, &endMatch);
    return found;
    }

std::string FileEditView::getSelectedText()
    {
    GtkTextIter startSel;
    GtkTextIter endSel;
    std::string text;
    bool haveSel = gtk_text_buffer_get_selection_bounds(mTextBuffer, &startSel, &endSel);
    if(haveSel)
	{
	GuiText gText = gtk_text_buffer_get_text(mTextBuffer, &startSel, &endSel, true);
	text = gText;
	}
    return text;
    }

bool FileEditView::findAndReplace(char const * const findStr, bool forward,
	bool caseSensitive, char const * const replaceStr)
    {
    GtkTextIter startSel;
    GtkTextIter endSel;
    bool haveSel = gtk_text_buffer_get_selection_bounds(mTextBuffer, &startSel, &endSel);
    if(haveSel)
	{
	bool match;
	GuiText text = gtk_text_buffer_get_text(mTextBuffer, &startSel, &endSel, true);
	if(caseSensitive)
	    {
	    match = (strcmp(text.c_str(), findStr) == 0);
	    }
	else
	    {
	    match = (StringCompareNoCase(text.c_str(), findStr) == 0);
	    }
	if(match)
	    {
	    gtk_text_buffer_delete_selection(mTextBuffer, true, true);
	    gtk_text_buffer_insert_at_cursor(mTextBuffer, replaceStr,
		    strlen(replaceStr));
	    }
	}
    return find(findStr, forward, caseSensitive);
    }

void FileEditView::bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len)
    {
    if(!doingHistory())
	{
	int offset = HistoryItem::getOffset(location);
	addHistoryItem(HistoryItem(true, offset, text, len));
	}
    highlightRequest();
    }

void FileEditView::bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end)
    {
    if(!doingHistory())
	{
	int offset = HistoryItem::getOffset(start);
	GuiText str(gtk_text_buffer_get_text(textbuffer, start, end, false));
	addHistoryItem(HistoryItem(false, offset, str, str.length()));
	}
    highlightRequest();
    }

bool FileEditView::idleHighlight()
    {
    bool foundToken = false;
    eHighlightTask task = mHighlighter.highlightUpdate(mTextView, getBuffer(),
		gtk_text_buffer_get_char_count(mTextBuffer));
    if(task & HT_FindToken)
        {
        foundToken = true;
        }
    if(task & HT_ShowMembers)
        {
        OovStringVec members = mHighlighter.getShowMembers();
#if(COMPLETION_WINDOW)
        mCompleteList.setList(members);
#else
	GtkTextView *findView = GTK_TEXT_VIEW(Builder::getBuilder()->
		getWidget("FindTextview"));
	Gui::clear(findView);
	for(auto const &str : members)
	    {
	    std::string appendStr = str;
	    appendStr += '\n';
	    Gui::appendText(findView, appendStr);
	    }
#endif
        }
    return foundToken;
    }

bool FileEditView::handleIndentKeys(GdkEvent *event)
    {
    bool handled = false;
    int modKeys = (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK) & event->key.state;
#if(COMPLETION_WINDOW)
    mCompleteList.handleEditorKey(event->key.keyval);
#endif
    switch(event->key.keyval)
	{
	case '>':	// Checking for "->"
	case '.':
	    {
#if(CODE_COMPLETE)
	    int offset = getCursorOffset();
	    mHighlighter.showMembers(offset-1);
#else
	    GtkTextIter cursorIter = GuiTextBuffer::getCursorIter(mTextBuffer);
	    int offset = GuiTextBuffer::getCursorOffset(mTextBuffer);
	    offset--;
	    if(event->key.keyval == '>')
		{
		GtkTextIter prevCharIter = cursorIter;
		gtk_text_iter_backward_char(&prevCharIter);
		GuiText str(gtk_text_buffer_get_text(mTextBuffer,
			&prevCharIter, &cursorIter, false));
		if(str[0] == '-')
		    offset--;
		else
		    offset = -1;
		}
	    if(offset != -1)
		{
		mHighlighter.showMembers(offset);
		}
#endif
	    }
	    break;

	case GDK_KEY_ISO_Left_Tab:	// This is shift tab on PC
	    // On Windows, modKeys==0, on Linux, modKeys==GDK_SHIFT_MASK
	    if(event->key.state == GDK_SHIFT_MASK || modKeys == 0 ||
		    modKeys == GDK_SHIFT_MASK)
		{
		if(mIndenter.shiftTabPressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_BackSpace:
	    if(modKeys == 0)
		{
		if(mIndenter.backspacePressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_Tab:
	    if(modKeys == 0)
		{
		if(mIndenter.tabPressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_KP_Home:
	case GDK_KEY_Home:
	    if(modKeys == 0)
		{
		if(mIndenter.homePressed())
		    {
		    Gui::scrollToCursor(getTextView());
		    handled = true;
		    }
		}
	    break;
	}
    return handled;
    }

extern "C" G_MODULE_EXPORT void on_CompletionListTreeview_row_activated(GtkWidget *widget, gpointer data)
    {
    sCurrentCompleteList->activateSelectedItem();
    }

extern "C" G_MODULE_EXPORT bool on_CompletionListTreeview_key_press_event(
	GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
    return sCurrentCompleteList->handleListKey(event->key.keyval);
    }

extern "C" G_MODULE_EXPORT void on_CompletionListTreeview_focus_out_event(GtkWidget *widget, gpointer data)
    {
    sCurrentCompleteList->lostFocus();
    }

extern "C" G_MODULE_EXPORT bool on_CompletionListTreeview_draw(GtkWidget *widget, gpointer data)
    {
    sCurrentCompleteList->resize();
    return false;
    }
