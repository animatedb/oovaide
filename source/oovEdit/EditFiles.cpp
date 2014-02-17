/*
 * Module.cpp
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "EditFiles.h"
#include "FilePath.h"
#include <algorithm>


static EditFiles *sEditFiles;

EditFiles::EditFiles():
    mFocusEditViewIndex(-1)
    {
    sEditFiles = this;
    }

void EditFiles::init(GtkNotebook *headerBook, GtkNotebook *srcBook)
    {
    // create scrolled window, with text view
    mHeaderBook = headerBook;
    mSourceBook = srcBook;
    mDebuggerCurrentLocationCursor = gdk_cursor_new(GDK_SB_RIGHT_ARROW);
    mDebuggerBreakpointCursor = gdk_cursor_new(GDK_DOT);
    }

void EditFiles::onIdle()
    {
    idleHighlight();
    for(auto &fv : mFileViews)
	{
	if(fv.mDesiredLine != -1)
	    {
	    fv.mFileView.gotoLine(fv.mDesiredLine);
	    fv.mDesiredLine = -1;
	    }
	}
    }

void EditFiles::gotoLine(int lineNum)
    {
    ScrolledFileView *fv = getScrolledFileView();
    if(fv)
	{
	fv->mDesiredLine = lineNum;
	}
    }

struct LineInfo
    {
    LineInfo(int line, int y):
	lineNum(line), yPos(y)
	{}
    int lineNum;
    int yPos;
    };

static std::vector<LineInfo> getLinesInfo(GtkTextView* textView, int height)
    {
    std::vector<LineInfo> linesInfo;

    int y1 = 0;
    int y2 = height;
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_LEFT,
	    0, y1, NULL, &y1);
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_LEFT,
	    0, y2, NULL, &y2);

    GtkTextIter iter;
    gtk_text_view_get_line_at_y(textView, &iter, y1, NULL);
    while(!gtk_text_iter_is_end(&iter))
	{
	int y, height;
	gtk_text_view_get_line_yrange(textView, &iter, &y, &height);
	int line = gtk_text_iter_get_line(&iter);
	linesInfo.push_back(LineInfo(line+1, y));
	if((y + height) >= y2)
	    {
	    break;
	    }
	gtk_text_iter_forward_line(&iter);
	}
    if(linesInfo.size() == 0)
	{
	linesInfo.push_back(LineInfo(1, 1));
	}
    return linesInfo;
    }

void EditFiles::drawMargin(GtkWidget *widget, cairo_t *cr)
    {
    GtkTextView *textView = GTK_TEXT_VIEW(widget);
    GdkWindow *marginWindow;

    int beforeLineSepWidth=1;
    int marginLineWidth=1;
    int afterLineSepWidth=2;

    marginWindow = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_LEFT);
    int height;
    gdk_window_get_geometry(marginWindow, NULL, NULL, NULL, &height);
    std::vector<LineInfo> linesInfo = getLinesInfo(textView, height);

    PangoLayout *layout = gtk_widget_create_pango_layout(widget, "");
    char str[8];
    snprintf(str, sizeof(str), "%d",
	    gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(textView)));
    pango_layout_set_text(layout, str, -1);
    gint layoutWidth = 0;
    pango_layout_get_pixel_size(layout, &layoutWidth, NULL);

    gtk_text_view_set_border_window_size(textView,
	    GTK_TEXT_WINDOW_LEFT,
	    layoutWidth + beforeLineSepWidth + marginLineWidth + afterLineSepWidth);

    pango_layout_set_width(layout, layoutWidth);
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

    GtkStyle *widgetStyle = gtk_widget_get_style(widget);
    PangoAttrList *attrList = pango_attr_list_new();
    PangoAttribute *attr = pango_attr_foreground_new(
	    widgetStyle->text_aa->red, widgetStyle->text_aa->green,
	    widgetStyle->text_aa->blue);
    attr->start_index = 0;
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert(attrList, attr);
    pango_layout_set_attributes(layout, attrList);

    for(auto const & lineInfo : linesInfo)
	{
	int pos;
	gtk_text_view_buffer_to_window_coords (textView,
	    GTK_TEXT_WINDOW_LEFT, 0, lineInfo.yPos, NULL, &pos);
	snprintf(str, sizeof(str), "%d", lineInfo.lineNum);
	pango_layout_set_text(layout, str, -1);
	gtk_render_layout(gtk_widget_get_style_context(widget), cr,
		layoutWidth + beforeLineSepWidth, pos, layout);
	}

    cairo_rectangle(cr, layoutWidth + beforeLineSepWidth, 0,
	    marginLineWidth, height);
    cairo_fill(cr);
    g_object_unref (G_OBJECT (layout));
    }

extern "C" G_MODULE_EXPORT gboolean on_EditFiles_focus_in_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles)
	sEditFiles->setFocusEditTextView(GTK_TEXT_VIEW(widget));
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_EditFiles_key_press_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    bool handled = false;
    if(sEditFiles)
	handled = sEditFiles->handleKeyPress(event);
    return handled;
    }

extern "C" G_MODULE_EXPORT gboolean onMarginDraw(GtkWidget *widget,
	cairo_t *cr, gpointer user_data)
    {
    if(sEditFiles)
	sEditFiles->drawMargin(widget, cr);
    return false;
    }

void EditFiles::addFile(char const * const fn, bool useMainView, int lineNum)
    {
    auto const &iter = std::find_if(mFileViews.begin(), mFileViews.end(),
	    [fn](ScrolledFileView const &fv) -> bool
		{ return fv.mFilename.compare(fn) == 0; });
    if(iter == mFileViews.end())
	{
	GtkNotebook *book = nullptr;
	if(useMainView)
	    book = mSourceBook;
	else
	    book = mHeaderBook;
	if(book)
	    {
	    FilePath fp(fn, FP_File);
	    GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
	    GtkWidget *editView = gtk_text_view_new();
	    gtk_container_add(GTK_CONTAINER(scrolled), editView);
	    Gui::appendPage(book, scrolled, gtk_label_new(fp.getName().c_str()));
	    gtk_widget_show_all(scrolled);

	    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(editView),
		    GTK_TEXT_WINDOW_LEFT, 10);	// need to get text size
	    g_signal_connect(editView, "draw",
		    G_CALLBACK(onMarginDraw), NULL);
	    // yad
//	gtk_text_view_get_window(GTK_TEXT_WINDOW_LEFT);
//	    gdk_window_set_cursor(window, GDK_DOT);
//	    gdk_window_set_cursor(window, GDK_SB_RIGHT_ARROW);

	    ScrolledFileView scrolledView;
	    scrolledView.mFileView.init(GTK_TEXT_VIEW(editView));
	    scrolledView.mFileView.openTextFile(fp.c_str());
	    scrolledView.mScrolled = GTK_SCROLLED_WINDOW(scrolled);
	    scrolledView.mFilename = fn;
	    g_signal_connect(editView, "focus_in_event",
		    G_CALLBACK(on_EditFiles_focus_in_event), NULL);
	    g_signal_connect(editView, "key_press_event",
		    G_CALLBACK(on_EditFiles_key_press_event), NULL);
	    mFileViews.push_back(scrolledView);
	    Gui::setCurrentPage(book, Gui::getNumPages(book)-1);
	    mFileViews[mFileViews.size()-1].mDesiredLine = lineNum;
	    }
	}
    }

void EditFiles::viewFile(char const * const fn, int lineNum)
    {
    FilePath moduleName(fn, FP_File);
    FilePath cppExt("cpp", FP_Ext);
    FilePath hExt("h", FP_Ext);
    bool header = moduleName.matchExtension(hExt.c_str());
    bool source = moduleName.matchExtension(cppExt.c_str());

    if(header || source)
	{
	moduleName.appendExtension("h");
	if(header)
	    addFile(moduleName.c_str(), false, lineNum);
	else
	    addFile(moduleName.c_str(), false, 1);

	moduleName.appendExtension("cpp");
	if(source)
	    addFile(moduleName.c_str(), true, lineNum);
	else
	    addFile(moduleName.c_str(), true, 1);
	}
    else
	{
	addFile(fn, true, lineNum);
	}
    }

void EditFiles::setFocusEditTextView(GtkTextView *editTextView)
    {
    mFocusEditViewIndex = -1;
    for(size_t i=0; i<mFileViews.size(); i++)
	{
	if(mFileViews[i].mFileView.getTextView() == editTextView)
	    {
	    mFocusEditViewIndex = i;
	    break;
	    }
	}
    }

bool EditFiles::handleKeyPress(GdkEvent *event)
    {
    bool handled = false;
    if(getEditView())
	handled = getEditView()->handleIndentKeys(event);
    return handled;
    }

bool EditFiles::checkExitSave()
    {
    bool exitOk = true;
    for(auto &fileView : mFileViews)
	{
	exitOk = fileView.mFileView.checkExitSave();
	if(!exitOk)
	    break;
	}
    return exitOk;
    }

