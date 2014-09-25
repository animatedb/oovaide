/*
 * Module.cpp
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "EditFiles.h"
#include "FilePath.h"
#include "Project.h"
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265
#endif

#define DBG_EDITF 0
#if(DBG_EDITF)
#include "Debug.h"
DebugFile sDbgFile("DbgEditFiles.txt");
#endif


static EditFiles *sEditFiles;


EditFiles::EditFiles(Debugger &debugger, EditOptions &editOptions):
    mEditOptions(editOptions),
    mHeaderBook(nullptr), mSourceBook(nullptr), mBuilder(nullptr),
    mFocusEditViewIndex(-1), mDebugger(debugger)
    {
    sEditFiles = this;
    }

void EditFiles::init(Builder &builder)
    {
    mBuilder = &builder;
    mHeaderBook = GTK_NOTEBOOK(builder.getWidget("EditNotebook1"));
    mSourceBook = GTK_NOTEBOOK(builder.getWidget("EditNotebook2"));
    updateDebugMenu();
    }

void EditFiles::onIdle()
    {
    idleHighlight();
    for(auto &fv : mFileViews)
	{
	if(fv->mDesiredLine != -1)
	    {
	    fv->mFileView.gotoLine(fv->mDesiredLine);
	    gtk_widget_grab_focus(GTK_WIDGET(fv->mFileView.getTextView()));
	    fv->mDesiredLine = -1;
	    }
	}
    }

FileEditView *EditFiles::getEditView()
    {
    if(mFocusEditViewIndex < mFileViews.size())
	return &mFileViews[mFocusEditViewIndex]->mFileView;
    else
	return nullptr;
    }

std::string EditFiles::getEditViewSelectedText()
    {
    FileEditView *view = getEditView();
    std::string text;
    if(view)
	{
	text = view->getSelectedText();
	}
    return text;
    }

void EditFiles::updateDebugMenu()
    {
    GdbChildStates state = mDebugger.getChildState();
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugGo")),
	    state != GCS_GdbChildRunning);
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugStepOver")),
	    state != GCS_GdbChildRunning);
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugStepInto")),
	    state != GCS_GdbChildRunning);
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugPause")),
	    state == GCS_GdbChildRunning);
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugStop")), true);
    Gui::setEnabled(GTK_BUTTON(mBuilder->getWidget("DebugViewVariable")),
	    state != GCS_GdbChildRunning);
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

void EditFiles::drawLeftMargin(GtkWidget *widget, cairo_t *cr, int &width, int &pixPerChar)
    {
    GtkTextView *textView = GTK_TEXT_VIEW(widget);
    GdkWindow *marginWindow;

    int beforeLineSepWidth=1;
    int marginLineWidth=1;
    int afterLineSepWidth=2;

    marginWindow = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_LEFT);
    int marginWindowHeight;
    gdk_window_get_geometry(marginWindow, NULL, NULL, NULL, &marginWindowHeight);
    std::vector<LineInfo> linesInfo = getLinesInfo(textView, marginWindowHeight);
    int lineHeight = linesInfo[1].yPos - linesInfo[0].yPos;

    PangoLayout *layout = gtk_widget_create_pango_layout(widget, "");
    char str[8];
    snprintf(str, sizeof(str), "%d",
	    gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(textView)));
    pango_layout_set_text(layout, str, -1);
    gint layoutWidth = 0;
    pango_layout_get_pixel_size(layout, &layoutWidth, NULL);
    int len = strlen(str);
    if(len > 0)
	pixPerChar = layoutWidth / len;
    else
	pixPerChar = 10;

    gtk_text_view_set_border_window_size(textView,
	    GTK_TEXT_WINDOW_LEFT,
	    layoutWidth + beforeLineSepWidth + marginLineWidth + afterLineSepWidth);
    width = layoutWidth;

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

    DebuggerLocation dbgLoc = mDebugger.getStoppedLocation();
    auto fvIter = std::find_if(mFileViews.begin(), mFileViews.end(),
	    [textView](std::unique_ptr<ScrolledFileView> const &fv) -> bool
		{ return(fv->getTextView() == textView); });
    if(fvIter != mFileViews.end())
	{
	DebuggerLocation thisFileLoc((*fvIter)->getFilename().c_str());
	int full = lineHeight*.8;
	int half = full/2;
	for(auto const & lineInfo : linesInfo)
	    {
	    int pos;
	    thisFileLoc.setLine(lineInfo.lineNum);
	    gtk_text_view_buffer_to_window_coords (textView,
		GTK_TEXT_WINDOW_LEFT, 0, lineInfo.yPos, NULL, &pos);
	    snprintf(str, sizeof(str), "%d", lineInfo.lineNum);
	    pango_layout_set_text(layout, str, -1);
	    gtk_render_layout(gtk_widget_get_style_context(widget), cr,
		    layoutWidth + beforeLineSepWidth, pos, layout);

	    int centerY = pos + lineHeight/2;
	    if(mDebugger.getBreakpoints().anyLocationsMatch(thisFileLoc))
		{
		cairo_set_source_rgb(cr, 128/255.0, 128/255.0, 0/255.0);
		cairo_arc(cr, half+1, centerY, half, 0, 2*M_PI);
		cairo_fill(cr);
		}
	    if(dbgLoc == thisFileLoc)
		{
		cairo_set_source_rgb(cr, 0/255.0, 0/255.0, 255.0/255.0);
		cairo_move_to(cr, 0, centerY);
		cairo_line_to(cr, full, centerY);

		cairo_move_to(cr, full, centerY);
		cairo_line_to(cr, half, centerY-half);

		cairo_move_to(cr, full, centerY);
		cairo_line_to(cr, half, centerY+half);

		cairo_stroke(cr);
		}
	    }
	cairo_set_source_rgb(cr, 0/255.0, 0/255.0, 0/255.0);
	cairo_rectangle(cr, layoutWidth + beforeLineSepWidth, 0,
		marginLineWidth, marginWindowHeight);
	cairo_fill(cr);
	g_object_unref (G_OBJECT (layout));
	}
    else
	width = 0;
    }

void EditFiles::drawRightMargin(GtkWidget *widget, cairo_t *cr, int leftMargin, int pixPerChar)
    {
    GtkTextView *textView = GTK_TEXT_VIEW(widget);
    GdkWindow *window = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_WIDGET);
    int marginWindowHeight;
    gdk_window_get_geometry(window, NULL, NULL, NULL, &marginWindowHeight);

    GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(
	    gtk_widget_get_parent(GTK_WIDGET(textView)));
    GtkAdjustment *adjust = gtk_scrolled_window_get_hadjustment(scrolledWindow);
    int scrollAdjust = (int)gtk_adjustment_get_value(adjust);
//    cairo_text_extents_t extents;
//    cairo_text_extents(cr, "5555555555", &extents);

    cairo_set_source_rgb(cr, 128/255.0, 128/255.0, 128/255.0);
    const int marginLineWidth = 1;
    cairo_rectangle(cr, (leftMargin + pixPerChar * 80) - scrollAdjust, 0,
	    marginLineWidth, marginWindowHeight);
    cairo_fill(cr);
    }

static void displayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("EditPopupMenu");
    GdbChildStates state = sEditFiles->getDebugger().getChildState();
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget("DebugViewVariable")),
	    state != GCS_GdbChildRunning);
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

extern "C" G_MODULE_EXPORT gboolean on_EditFiles_focus_in_event(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles)
	sEditFiles->setFocusEditTextView(GTK_TEXT_VIEW(widget));
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_EditFiles_button_press_event(GtkWidget *widget,
        GdkEvent  *event, gpointer user_data)
    {
    bool handled = false;
    GdkEventButton *eventBut = reinterpret_cast<GdkEventButton*>(event);
    if(eventBut->button == 3)	// Right button
	{
	displayContextMenu(eventBut->button, eventBut->time, (gpointer)event);
	handled = true;
	}
    return handled;
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
	{
	int width;
	int pixPerChar;
	sEditFiles->drawLeftMargin(widget, cr, width, pixPerChar);
	sEditFiles->drawRightMargin(widget, cr, width, pixPerChar);
	}
    return false;
    }

void EditFiles::removeNotebookPage(GtkWidget *pageWidget)
    {
    GtkNotebook *book = mHeaderBook;
    int page = gtk_notebook_page_num(book, pageWidget);
    if(page == -1)
	{
	book = mSourceBook;
	page = gtk_notebook_page_num(book, pageWidget);
	}
    auto const &iter = std::find_if(mFileViews.begin(), mFileViews.end(),
	    [pageWidget](std::unique_ptr<ScrolledFileView> const &fv) -> bool
		{ return fv->getViewTopParent() == pageWidget; });
    mFileViews.erase(iter);
    gtk_notebook_remove_page(book, page);
    }

extern "C" G_MODULE_EXPORT gboolean onTabLabelCloseClicked(GtkButton *button,
	gpointer user_data)
    {
    GtkWidget *pageWidget = GTK_WIDGET(user_data);
    sEditFiles->removeNotebookPage(pageWidget);
    return true;
    }

static GtkWidget *newTabLabel(char const * const tabText, GtkWidget *viewTopParent)
    {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(box), gtk_label_new(tabText));
    gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    GtkButton *button = GTK_BUTTON(gtk_button_new());
    gtk_button_set_relief(button, GTK_RELIEF_NORMAL);
    gtk_button_set_focus_on_click(button, false);
    gtk_button_set_image(button,
	    gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_SMALL_TOOLBAR)); //GTK_ICON_SIZE_MENU));

    char const * const data = "* {\n"
	"-GtkButton-default-border : 0px;\n"
	"-GtkButton-default-outside-border : 0px;\n"
	"-GtkButton-inner-border: 0px;\n"
	"-GtkWidget-focus-line-width : 0px;\n"
	"-GtkWidget-focus-padding : 0px;\n"
	"padding: 0px;\n"
	"}";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, data, -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(button));
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
//    gtk_box_pack_start(box, button, False, False, 0);
    gtk_container_add(GTK_CONTAINER(box), GTK_WIDGET(button));

    g_signal_connect(button, "clicked", G_CALLBACK(onTabLabelCloseClicked), viewTopParent);
    gtk_widget_show_all(box);
    return box;

    }

static bool putInMainWindow(char const * const fn)
    {
    FilePath hExt("h", FP_Ext);
    FilePath fileName(fn, FP_File);
    bool header = fileName.matchExtension(hExt.c_str());
    return(!header);
    }

void EditFiles::viewFile(char const * const fn, int lineNum)
    {
    FilePath fp;

    fp.getAbsolutePath(fn, FP_File);
    auto iter = std::find_if(mFileViews.begin(), mFileViews.end(),
	[fp](std::unique_ptr<ScrolledFileView> const &fv) -> bool
	{ return fv->mFilename.comparePaths(fp.c_str()) == 0; });
    if(iter == mFileViews.end())
	{
	GtkNotebook *book = nullptr;
	if(putInMainWindow(fn))
	    book = mSourceBook;
	else
	    book = mHeaderBook;
	if(book)
	    {
	    GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
	    GtkWidget *editView = gtk_text_view_new();

	    /// @todo - use make_unique when supported.
	    ScrolledFileView *scrolledView = new ScrolledFileView();
	    scrolledView->mFileView.init(GTK_TEXT_VIEW(editView));
	    scrolledView->mFileView.openTextFile(fp.c_str());
	    scrolledView->mScrolled = GTK_SCROLLED_WINDOW(scrolled);
	    scrolledView->mFilename = fp;

	    gtk_container_add(GTK_CONTAINER(scrolled), editView);
	    Gui::appendPage(book, scrolled,
		    newTabLabel(fp.getName().c_str(), scrolledView->getViewTopParent()));
	    gtk_widget_show_all(scrolled);
	    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(editView),
		    GTK_TEXT_WINDOW_LEFT, 10);	// need to get text size
	    g_signal_connect(editView, "draw", G_CALLBACK(onMarginDraw), NULL);

	    g_signal_connect(editView, "focus_in_event",
		    G_CALLBACK(on_EditFiles_focus_in_event), NULL);
	    g_signal_connect(editView, "key_press_event",
		    G_CALLBACK(on_EditFiles_key_press_event), NULL);
	    g_signal_connect(editView, "button_press_event",
		    G_CALLBACK(on_EditFiles_button_press_event), NULL);
	    mFileViews.push_back(std::unique_ptr<ScrolledFileView>(scrolledView));
	    iter = mFileViews.end()-1;
	    }
	}
    GtkNotebook *notebook = (*iter)->getBook();
    if(notebook)
	{
	int pageIndex = getPageNumber(notebook, (*iter)->getTextView());
	Gui::setCurrentPage(notebook, pageIndex);
	// focus is set after the screen is displayed by onIdle
//	gtk_widget_grab_focus(GTK_WIDGET((*iter).getTextView()));
#if(DBG_EDITF)
	sDbgFile.printflush("ViewFile %d\n", pageIndex);
#endif
	(*iter)->mDesiredLine = lineNum;
	}
    }

int EditFiles::getPageNumber(GtkNotebook *notebook, GtkTextView const *view) const
    {
    int numPages = Gui::getNumPages(notebook);
    int pageNum = -1;
    // Parent is the scrolled widget.
    GtkWidget *page = gtk_widget_get_parent(GTK_WIDGET(view));
    for(int i=0; i<numPages; i++)
	{
	if(Gui::getNthPage(notebook, i) == page)
	    {
	    pageNum = i;
	    break;
	    }
	}
    return pageNum;
    }

void EditFiles::idleHighlight()
    {
    timeval curTime;
    gettimeofday(&curTime, NULL);
    if(curTime.tv_sec != mLastHightlightIdleUpdate.tv_sec /*||
	    abs(curTime.tv_usec - mLastHightlightIdleUpdate.tv_usec) > 300*/)
	{
	for(auto &view : mFileViews)
	    view->getFileEditView().idleHighlight();
	mLastHightlightIdleUpdate = curTime;
	}
    }

void EditFiles::viewModule(char const * const fn, int lineNum)
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
	    viewFile(moduleName.c_str(), lineNum);
	else
	    viewFile(moduleName.c_str(), 1);

	moduleName.appendExtension("cpp");
	if(source)
	    viewFile(moduleName.c_str(), lineNum);
	else
	    viewFile(moduleName.c_str(), 1);
	}
    else
	{
	viewFile(fn, lineNum);
	}
    }

void EditFiles::setFocusEditTextView(GtkTextView *editTextView)
    {
    mFocusEditViewIndex = -1;
    for(size_t i=0; i<mFileViews.size(); i++)
	{
	if(mFileViews[i]->mFileView.getTextView() == editTextView)
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
	exitOk = fileView->mFileView.checkExitSave();
	if(!exitOk)
	    break;
	}
    return exitOk;
    }

bool EditFiles::checkDebugger()
    {
    bool ok = false;

    NameValueFile projFile(Project::getProjectFilePath().c_str());
    projFile.readFile();
    getDebugger().setDebuggerFilePath(projFile.getValue(OptToolDebuggerPath).c_str());
    getDebugger().setDebuggee(mEditOptions.getValue(OptEditDebuggee).c_str());
    getDebugger().setDebuggeeArgs(mEditOptions.getValue(OptEditDebuggeeArgs).c_str());
    getDebugger().setWorkingDir(mEditOptions.getValue(OptEditDebuggerWorkingDir).c_str());
//Gui::messageBox("Debugging is not recommended. It is very unstable.");
    std::string debugger = getDebugger().getDebuggerFilePath();
    std::string debuggee = getDebugger().getDebuggeeFilePath();
    if(debugger.length() > 0)
	{
	if(debuggee.length() > 0)
	    {
// The debugger could be on the path.
//	    if(fileExists(debugger.c_str()))
		{
		if(fileExists(debuggee.c_str()))
		    {
		    ok = true;
		    }
		else
		    Gui::messageBox("Component to debug in Edit/Preferences does not exist");
		}
//	    else
//		Gui::messageBox("Debugger in Oovcde Analysis/Settings does not exist");
	    }
	else
	    Gui::messageBox("Component to debug must be set in Edit/Preferences");
	}
    else
	Gui::messageBox("Debugger tool path must be set in Oovcde Analysis/Settings");
    return ok;
    }

void EditFiles::showInteractNotebookTab(char const * const tabName)
    {
    GtkNotebook *book = GTK_NOTEBOOK(Builder::getBuilder()->getWidget("InteractNotebook"));
    Gui::setCurrentPage(book, Gui::findTab(book, tabName));
    }


extern "C" G_MODULE_EXPORT gboolean on_DebugGo_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
	sEditFiles->getDebugger().resume();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStepOver_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
	sEditFiles->getDebugger().stepOver();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStepInto_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
	sEditFiles->getDebugger().stepInto();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugPause_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles)
	sEditFiles->getDebugger().interrupt();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStop_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles)
	sEditFiles->getDebugger().stop();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugToggleBreakpoint_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles)
	{
	FileEditView const *view = sEditFiles->getEditView();
	if(view)
	    {
	    int line = Gui::getCurrentLineNumber(view->getTextView());
	    DebuggerBreakpoint bp(view->getFilename().c_str(), line);
	    sEditFiles->getDebugger().toggleBreakpoint(bp);
	    Gui::redraw(GTK_WIDGET(view->getTextView()));
	    }
	}
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugViewVariable_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
	{
	FileEditView const *view = sEditFiles->getEditView();
	if(view)
	    {
	    sEditFiles->showInteractNotebookTab("Data");
	    sEditFiles->getDebugger().startGetVariable(
		    Gui::getSelectedText(view->getTextView()));
#if(DBG_EDITF)
    sDbgFile.printflush("ViewVar %s\n", Gui::getSelectedText(view->getTextView()));
#endif
	    }
	}
    return false;
    }
