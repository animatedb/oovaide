/*
 * FileEditView.cpp
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "FileEditView.h"
#include "FilePath.h"


void signalBufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len, gpointer user_data);
void signalBufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end, gpointer user_data);

//static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data);
//gboolean draw(GtkWidget *widget, CairoContext *cr, gpointer user_data);
gboolean draw(GtkWidget *widget, void *cr, gpointer user_data);



void FileEditView::init(GtkTextView *textView)
    {
    mTextView = textView;
    mTextBuffer = gtk_text_view_get_buffer(mTextView);
    mIndenter.init(mTextBuffer);
    gtk_widget_grab_focus(GTK_WIDGET(mTextView));
    g_signal_connect(G_OBJECT(mTextBuffer), "insert-text",
          G_CALLBACK(signalBufferInsertText), NULL);
    g_signal_connect(G_OBJECT(mTextBuffer), "delete-range",
          G_CALLBACK(signalBufferDeleteRange), NULL);
//	    g_signal_connect(G_OBJECT(mBuilder.getWidget("EditTextScrolledwindow")),
//		    "scroll-child", G_CALLBACK(scrollChild), NULL);
    g_signal_connect(G_OBJECT(mTextView), "draw", G_CALLBACK(draw), NULL);
    }

bool FileEditView::openTextFile(char const * const fn)
    {
    setFileName(fn);
    FILE *fp = fopen(fn, "rb");
    if(fp)
	{
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	std::vector<char> buf(fileSize);
	// actualCount can be less than fileSize on Windows due to /r/n
	int actualCount = fread(&buf.front(), 1, fileSize, fp);
	if(actualCount > 0)
	    {
	    gtk_text_buffer_set_text(mTextBuffer, &buf.front(), actualCount);
	    gtk_text_buffer_set_modified(mTextBuffer, FALSE);
	    }
	fclose(fp);
	setNeedHighlightUpdate(HS_ExternalChange);
	}
    return(fp != nullptr);
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
	success = saveAsTextFile(mFileName.c_str());
    return success;
    }

GuiText FileEditView::getBuffer()
    {
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(mTextBuffer, &start, &end);
    return GuiText(gtk_text_buffer_get_text(mTextBuffer, &start, &end, false));
    }

void FileEditView::highlight()
    {
    if(mFileName.length())
	{
	int numArgs = 0;
	char const * cppArgv[40];
//	    cppArgv[numArgs++] = "";
//	    cppArgv[numArgs++] = mFileName.c_str();
	FilePath path;
	path.getAbsolutePath(mFileName.c_str(), FP_File);
	mHighlighter.highlight(mTextView, path.c_str(), getBuffer().c_str(),
		gtk_text_buffer_get_char_count(mTextBuffer), cppArgv, numArgs);
	}
    }

void FileEditView::gotoLine(int lineNum)
    {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &iter, lineNum-1);
    gtk_text_buffer_place_cursor(mTextBuffer, &iter);
//	    gtk_text_view_scroll_to_iter(mTextView, &iter, 0, FALSE, 0.5, 0);
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
	gtk_text_view_scroll_to_mark(mTextView, mark, 0, TRUE, 0, 0.5);
	}
    }

bool FileEditView::saveAsTextFileWithDialog()
    {
    PathChooser ch;
    std::string filename = mFileName;
    bool saved = false;
    std::string prompt = "Save ";
    prompt += mFileName;
    prompt += " As";
    if(ch.ChoosePath(GTK_WINDOW(mTextView), prompt.c_str(),
	    GTK_FILE_CHOOSER_ACTION_SAVE, filename))
	{
	saved = saveAsTextFile(filename.c_str());
	}
    return saved;
    }

bool FileEditView::saveAsTextFile(char const * const fn)
    {
    size_t writeSize = -1;
    std::string tempFn = fn;
    setFileName(fn);
    tempFn += ".tmp";
    FILE *fp = fopen(tempFn.c_str(), "wb");
    if(fp)
	{
	int size = gtk_text_buffer_get_char_count(mTextBuffer);
	GuiText buf = getBuffer();
	writeSize = fwrite(buf.c_str(), 1, size, fp);
	fclose(fp);
	deleteFile(fn);
	renameFile(tempFn.c_str(), fn);
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
	GtkWidget *button = gtk_dialog_add_button(dlg, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
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

void FileEditView::find(char const * const findStr, bool forward, bool caseSensitive)
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
    }

void FileEditView::bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len)
    {
    if(!doingHistory())
	{
	int offset = HistoryItem::getOffset(location);
	addHistoryItem(HistoryItem(true, offset, text, len));
	}
    setNeedHighlightUpdate(HS_ExternalChange);
    }

void FileEditView::bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end)
    {
    if(!doingHistory())
	{
	int offset = HistoryItem::getOffset(start);
	GuiText str(gtk_text_buffer_get_text(textbuffer, start, end, false));
	addHistoryItem(HistoryItem(false, offset, str.c_str(), str.length()));
	}
    setNeedHighlightUpdate(HS_ExternalChange);
    }

void FileEditView::drawHighlight()
    {
    if(getHighlightUpdate() == HS_DrawHighlight)
	{
	setNeedHighlightUpdate(HS_HighlightDone);
	}
    else if(getHighlightUpdate() != HS_HighlightDone)
	{
	setNeedHighlightUpdate(HS_ExternalChange);
	}
    }

void FileEditView::idleHighlight()
    {
    if(getHighlightUpdate() == HS_ExternalChange)
	{
	highlight();
	setNeedHighlightUpdate(HS_DrawHighlight);
	}
    }

bool FileEditView::handleIndentKeys(GdkEvent *event)
    {
    bool handled = false;
    switch(event->key.keyval)
	{
	case GDK_KEY_ISO_Left_Tab:	// This is shift tab on PC
	    if(event->key.state == GDK_SHIFT_MASK || event->key.state == 0)
		{
		if(mIndenter.shiftTabPressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_BackSpace:
	    if(event->key.state == 0)
		{
		if(mIndenter.backspacePressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_Tab:
	    if(event->key.state == 0)
		{
		if(mIndenter.tabPressed())
		    handled = true;
		}
	    break;

	case GDK_KEY_KP_Home:
	case GDK_KEY_Home:
	    if(event->key.state == 0)
		{
		if(mIndenter.homePressed())
		    handled = true;
		}
	    break;
	}
    return handled;
    }

