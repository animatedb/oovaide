/*
 * EditFiles.cpp
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "EditFiles.h"
#include "FilePath.h"
#include "Project.h"
#include "OovError.h"
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265
#endif

#define DBG_EDITF 0
#if(DBG_EDITF)
#include "Debug.h"
DebugFile sDbgFile("DbgEditFiles.txt", false);
#endif

// This works on Windows with GTK 3.6.
// It works on Ubuntu 14.10 with GTK 3.12, but not with Ubuntu 15.10 later than 3.14.
// See  gtk/tests/testtextview.c.
#if(GTK_MINOR_VERSION >= 14)
#define USE_DRAW_LAYER 1
#endif


static EditFiles *sEditFiles;


static inline guint16 convertGdkColorToPango(double color)
    {
    return color * 65536;
    }


LeftMargin::~LeftMargin()
    {
    if(mMarginLayout)
        {
        g_object_unref(mMarginLayout);
        }
    // Freeing the remaining margin stuff causes a crash. Maybe if is freed
    // when the layout is freed?
/*
    if(mMarginAttrList)
        {
        pango_attr_list_unref(mMarginAttrList);
        }
    if(mMarginAttr)
        {
//        pango_attribute_destroy(mMarginAttr);
        }
*/
    }


void LeftMargin::updateTextInfo(GtkTextView *textView)
    {
    char str[8];
    snprintf(str, sizeof(str), "%d",
            gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(textView)));
    pango_layout_set_text(mMarginLayout, str, -1);
    pango_layout_get_pixel_size(mMarginLayout, &mTextWidth, NULL);
    int len = strlen(str);
    if(len > 0)
        mPixPerChar = mTextWidth / len;
    else
        mPixPerChar = 10;
    }

void LeftMargin::setupMargin(GtkTextView *textView)
    {
    if(!mMarginLayout)
        {
        GtkWidget *widget = GTK_WIDGET(textView);

        mMarginLayout = gtk_widget_create_pango_layout(widget, "");
        updateTextInfo(textView);

        pango_layout_set_width(mMarginLayout, mTextWidth);
        pango_layout_set_alignment(mMarginLayout, PANGO_ALIGN_RIGHT);

        GtkStyleContext *widgetStyle = gtk_widget_get_style_context(widget);
        GdkRGBA color;
        gtk_style_context_get_color(widgetStyle, GTK_STATE_FLAG_NORMAL, &color);
        mMarginAttr = pango_attr_foreground_new(convertGdkColorToPango(color.red),
                convertGdkColorToPango(color.green), convertGdkColorToPango(color.blue));
        mMarginAttrList = pango_attr_list_new();

        mMarginAttr->start_index = 0;
        mMarginAttr->end_index = G_MAXUINT;
        pango_attr_list_insert(mMarginAttrList, mMarginAttr);
        pango_layout_set_attributes(mMarginLayout, mMarginAttrList);
        }
    }

int LeftMargin::getMarginWidth() const
    {
    return mTextWidth + getBeforeMarginLineSepWidth() + getMarginLineWidth() +
            getAfterMarginLineSepWidth();
    }

int LeftMargin::getMarginHeight(GtkTextView *textView) const
    {
    GdkWindow *marginWindow = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_LEFT);
    int marginWindowHeight;
    gdk_window_get_geometry(marginWindow, nullptr, nullptr, nullptr, &marginWindowHeight);
    return marginWindowHeight;
    }

void LeftMargin::drawMarginLineNum(GtkTextView *textView, cairo_t *cr, int lineNum, int yPos)
    {
    GtkWidget *widget = GTK_WIDGET(textView);

    if(mMarginLayout)
        {
        char str[8];
        snprintf(str, sizeof(str), "%d", lineNum);
        pango_layout_set_text(mMarginLayout, str, -1);
        gtk_render_layout(gtk_widget_get_style_context(widget), cr,
                mTextWidth, yPos, mMarginLayout);
        }
    }

void LeftMargin::drawMarginLine(GtkTextView *textView, cairo_t *cr)
    {
    cairo_set_source_rgb(cr, 0/255.0, 0/255.0, 0/255.0);
    cairo_rectangle(cr, mTextWidth + getBeforeMarginLineSepWidth(), 0,
            getMarginLineWidth(), getMarginHeight(textView));
    }


//////////

EditFiles::EditFiles(Debugger &debugger, EditOptions &editOptions):
    mEditOptions(editOptions),
    mHeaderBook(nullptr), mSourceBook(nullptr), mBuilder(nullptr),
    mLastFocusGtkTextView(nullptr), mDebugger(debugger)
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

void EditFiles::closeAll()
    {
    mFileViews.clear();
    }

EditFiles &EditFiles::getEditFiles()
    {
    return *sEditFiles;
    }

void EditFiles::onIdle()
    {
    idleHighlight();
    for(auto &fv : mFileViews)
        {
        if(fv->mDesiredLine != -1)
            {
#if(DBG_EDITF)
            sDbgFile.printflush("onIdle file %s line %d\n",
                    fv->getFileEditView().getFileName().c_str(),
                    fv->mDesiredLine);
#endif
            fv->mFileView.gotoLine(fv->mDesiredLine);
            gtk_widget_grab_focus(GTK_WIDGET(fv->mFileView.getTextView()));
            fv->mDesiredLine = -1;
            }
        }
    }

ScrolledFileView *EditFiles::getScrolledFileView(GtkTextBuffer *textBuffer)
    {
    ScrolledFileView *scrolledView = nullptr;

    auto fvIter = std::find_if(sEditFiles->mFileViews.begin(), sEditFiles->mFileViews.end(),
            [&textBuffer](std::unique_ptr<ScrolledFileView> const &fv) -> bool
                { return(fv->getFileEditView().getTextBuffer() == textBuffer); });
    if(fvIter != sEditFiles->mFileViews.end())
        {
        scrolledView = fvIter->get();
        }
    return scrolledView;
    }

ScrolledFileView *EditFiles::getScrolledFileView(GtkTextView *textView)
    {
    ScrolledFileView *scrolledView = nullptr;

    auto fvIter = std::find_if(sEditFiles->mFileViews.begin(), sEditFiles->mFileViews.end(),
            [&textView](std::unique_ptr<ScrolledFileView> const &fv) -> bool
                { return(fv->getFileEditView().getTextView() == textView); });
    if(fvIter != sEditFiles->mFileViews.end())
        {
        scrolledView = fvIter->get();
        }
    return scrolledView;
    }


FileEditView *EditFiles::getEditView()
    {
    FileEditView *view = nullptr;
    if(mLastFocusGtkTextView)
        {
        ScrolledFileView *scrollView = getScrolledFileView(mLastFocusGtkTextView);
        view = &scrollView->getFileEditView();
        }
    return view;
    }

// DEAD CODE
/*
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
*/

void EditFiles::updateDebugMenu()
    {
    DebuggerChildStates state = mDebugger.getChildState();
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugGo")),
            state != DCS_ChildRunning);
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugStepOver")),
            state != DCS_ChildRunning);
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugStepInto")),
            state != DCS_ChildRunning);
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugPause")),
            state == DCS_ChildRunning);
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugStop")), true);
    Gui::setEnabled(GTK_MENU_ITEM(mBuilder->getWidget("DebugViewVariable")),
            state != DCS_ChildRunning);
    }

void EditFiles::gotoLine(int lineNum)
    {
    if(mLastFocusGtkTextView)
        {
        ScrolledFileView *fv = getScrolledFileView(mLastFocusGtkTextView);
        if(fv)
            {
            fv->mDesiredLine = lineNum;
            }
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

void ScrolledFileView::drawMargins(cairo_t *cr)
    {
    drawLeftMargin(cr);
    drawRightMargin(cr);
    }

void ScrolledFileView::drawLeftMargin(cairo_t *cr)
    {
    GtkTextView *textView = getTextView();
    DebuggerLocation dbgLoc = mDebugger.getStoppedLocation();
    int marginWindowHeight = mLeftMargin.getMarginHeight(textView);
    std::vector<LineInfo> linesInfo = getLinesInfo(textView, marginWindowHeight);
    int lineHeight = linesInfo[1].yPos - linesInfo[0].yPos;

    DebuggerLocation thisFileLoc(getFilename());
    int full = lineHeight*.8;
    int half = full/2;
    for(auto const & lineInfo : linesInfo)
        {
        int yPos;
        thisFileLoc.setLine(lineInfo.lineNum);
        gtk_text_view_buffer_to_window_coords (textView,
            GTK_TEXT_WINDOW_LEFT, 0, lineInfo.yPos, NULL, &yPos);

        mLeftMargin.drawMarginLineNum(textView, cr, lineInfo.lineNum, yPos);

        int centerY = yPos + lineHeight/2;
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
    mLeftMargin.drawMarginLine(textView, cr);
    cairo_fill(cr);
//  g_object_unref (G_OBJECT (layout));
    }

void ScrolledFileView::drawRightMargin(cairo_t *cr)
    {
    GtkTextView *textView = getTextView();
    GdkWindow *window = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_WIDGET);
    int textWindowHeight;
    gdk_window_get_geometry(window, NULL, NULL, NULL, &textWindowHeight);

    GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(
            gtk_widget_get_parent(GTK_WIDGET(textView)));
    GtkAdjustment *adjust = gtk_scrolled_window_get_hadjustment(scrolledWindow);
    int scrollAdjust = (int)gtk_adjustment_get_value(adjust);
//    cairo_text_extents_t extents;
//    cairo_text_extents(cr, "5555555555", &extents);

    cairo_set_source_rgb(cr, 128/255.0, 128/255.0, 128/255.0);
    cairo_rectangle(cr, (mLeftMargin.getMarginWidth() +
            (mLeftMargin.getPixPerChar() * 80)) - scrollAdjust,
            0, mLeftMargin.getMarginLineWidth(), textWindowHeight);
    cairo_fill(cr);
    }

static void displayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("EditPopupMenu");
    DebuggerChildStates state = sEditFiles->getDebugger().getChildState();
    Gui::setEnabled(GTK_BUTTON(Builder::getBuilder()->getWidget("DebugViewVariable")),
            state != DCS_ChildRunning);
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

#if(USE_DRAW_LAYER)
extern "C" G_MODULE_EXPORT gboolean onMarginDraw(GtkTextView *textView,
        cairo_t *cr, gpointer user_data)
    {
    if(sEditFiles)
        {
        ScrolledFileView *scrolledView = sEditFiles->getScrolledFileView(textView);
        if(scrolledView)
            {
            scrolledView->drawMargins(cr);
            }
        }
    return false;
    }
typedef void (*drawLayerFuncType)(GtkTextView *textView, GtkTextViewLayer layer,
        cairo_t *cr);
static void textView_draw_layer(GtkTextView *textView, GtkTextViewLayer layer,
        cairo_t *cr)
    {
    if(layer == GTK_TEXT_VIEW_LAYER_ABOVE)
        {
        onMarginDraw(textView, cr, nullptr);
        }
    }
static void overrideDrawLayer(GtkWidget *widget)
    {
    GtkTextViewClass *textViewClass = GTK_TEXT_VIEW_GET_CLASS(widget);
    textViewClass->draw_layer = textView_draw_layer;
    }
#else
extern "C" G_MODULE_EXPORT gboolean onTextViewDraw(GtkWidget *widget,
        cairo_t *cr, gpointer user_data)
    {
    ScrolledFileView *scrolledView = static_cast<ScrolledFileView*>(user_data);
    if(sEditFiles)
        {
        scrolledView->drawMargins(cr);
        }
    return false;
    }
#endif

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
    if(eventBut->button == 3)   // Right button
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

void EditFiles::removeNotebookPage(GtkWidget *pageWidget)
    {
    mLastFocusGtkTextView = nullptr;
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

static GtkWidget *newTabLabel(OovStringRef const tabText, GtkWidget *viewTopParent)
    {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(box), gtk_label_new(tabText));
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
    bool header = fileName.matchExtension(hExt);
    return(!header);
    }

GtkNotebook *EditFiles::getBook(FileEditView *view)
    {
    GtkNotebook *book = nullptr;
    ScrolledFileView *scrollView = getScrolledFileView(view->getTextView());
    auto iter = std::find_if(mFileViews.begin(), mFileViews.end(),
        [&scrollView](std::unique_ptr<ScrolledFileView> const &scview)
            { return(scview.get() == scrollView); }
        );
    if(iter != mFileViews.end())
        {
        book = (*iter)->getBook();
        }
    return book;
    }

void EditFiles::viewFile(OovStringRef const fn, int lineNum)
    {
    FilePath fp;

    fp.getAbsolutePath(fn, FP_File);
    auto iter = std::find_if(mFileViews.begin(), mFileViews.end(),
        [fp](std::unique_ptr<ScrolledFileView> const &fv) -> bool
        { return fv->mFilename.comparePaths(fp) == 0; });
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
            ScrolledFileView *scrolledView = new ScrolledFileView(mDebugger);
            scrolledView->mFileView.init(GTK_TEXT_VIEW(editView), this);
            scrolledView->mFileView.openTextFile(fp);
            scrolledView->mScrolled = GTK_SCROLLED_WINDOW(scrolled);
            scrolledView->mFilename = fp;

            gtk_container_add(GTK_CONTAINER(scrolled), editView);
            Gui::appendPage(book, scrolled,
                    newTabLabel(fp.getName(), scrolledView->getViewTopParent()));
            gtk_widget_show_all(scrolled);
            scrolledView->mLeftMargin.setupMargin(GTK_TEXT_VIEW(editView));

            // Set up the windows to draw the line numbers in the left margin.
            // GTK has changed, and different setups are required for different
            // versions of GTK.
            // This is not allowed for a text view
            //          gtk_widget_set_app_paintable(editView, true);
#if(USE_DRAW_LAYER)
            overrideDrawLayer(editView);
            // If the left window is not created, errors display, "Attempt to
            // convert text buffer coordinates to coordinates for a nonexistent
            // buffer or private child window of GtkTextView". So create a
            // window that is only one pixel wide, and draw the line numbers
            // on the main text view window.
            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(editView),
                    GTK_TEXT_WINDOW_LEFT, 1);
            gtk_text_view_set_left_margin(GTK_TEXT_VIEW(editView),
                    scrolledView->mLeftMargin.getMarginWidth());
#else
            // The margin is drawn on the border window.
            gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(editView),
                    GTK_TEXT_WINDOW_LEFT, scrolledView->mLeftMargin.getMarginWidth());
            g_signal_connect(editView, "draw", G_CALLBACK(onTextViewDraw), scrolledView);
#endif

            g_signal_connect(editView, "focus_in_event",
                    G_CALLBACK(on_EditFiles_focus_in_event), NULL);
            g_signal_connect(editView, "key_press_event",
                    G_CALLBACK(on_EditFiles_key_press_event), NULL);
            g_signal_connect(editView, "button_press_event",
                    G_CALLBACK(on_EditFiles_button_press_event), NULL);
            mFileViews.push_back(std::unique_ptr<ScrolledFileView>(scrolledView));
#if(DBG_EDITF)
        sDbgFile.printflush("viewFile count %d, line %d\n", mFileViews.size(), lineNum);
#endif
            iter = mFileViews.end()-1;
            }
        }
    GtkNotebook *notebook = (*iter)->getBook();
    if(notebook)
        {
        int pageIndex = getPageNumber(notebook, (*iter)->getTextView());
        Gui::setCurrentPage(notebook, pageIndex);
        // focus is set after the screen is displayed by onIdle
//      gtk_widget_grab_focus(GTK_WIDGET((*iter).getTextView()));
#if(DBG_EDITF)
        sDbgFile.printflush("viewFile %d\n", pageIndex);
#endif
        if(lineNum > 1)
            {
            (*iter)->mDesiredLine = lineNum;
            }
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
    if(curTime.tv_sec != mLastHightlightIdleUpdate.tv_sec ||
            abs(curTime.tv_usec - mLastHightlightIdleUpdate.tv_usec) > 300)
        {
        OovString fn;
        size_t offset;
        for(auto &view : mFileViews)
            {
            FileEditView &editView = view->getFileEditView();
            if(editView.idleHighlight())
                {
                editView.getFindTokenResults(fn, offset);
                }
            }
        if(fn.length() > 0)
            {
            viewFile(fn, offset);
            }
        mLastHightlightIdleUpdate = curTime;
        }
    }

void EditFiles::viewModule(OovStringRef const fn, int lineNum)
    {
    FilePath moduleName(fn, FP_File);
    FilePath cppExt("cpp", FP_Ext);
    FilePath hExt("h", FP_Ext);
    bool header = moduleName.matchExtension(hExt);
    bool source = moduleName.matchExtension(cppExt);

    if(header || source)
        {
        moduleName.appendExtension("h");
        if(header)
            viewFile(moduleName, lineNum);
        else
            viewFile(moduleName, 1);

        moduleName.appendExtension("cpp");
        if(source)
            viewFile(moduleName, lineNum);
        else
            viewFile(moduleName, 1);
        }
    else
        {
        viewFile(fn, lineNum);
        }
    }

void EditFiles::setFocusEditTextView(GtkTextView *editTextView)
    {
    mLastFocusGtkTextView = editTextView;
    }

bool EditFiles::handleKeyPress(GdkEvent *event)
    {
    bool handled = false;
    if(getEditView())
        handled = getEditView()->handleKeys(event);
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

    NameValueFile projFile(Project::getProjectFilePath());
    if(!projFile.readFile())
        {
        OovString str = "Unable to get project file to get debugger: ";
        str += Project::getProjectFilePath();
        OovError::report(ET_Error, str);
        }
    getDebugger().setDebuggerFilePath(projFile.getValue(OptToolDebuggerPath));
    getDebugger().setDebuggee(mEditOptions.getValue(OptEditDebuggee));
    getDebugger().setDebuggeeArgs(mEditOptions.getValue(OptEditDebuggeeArgs));
    getDebugger().setWorkingDir(mEditOptions.getValue(OptEditDebuggerWorkingDir));
//Gui::messageBox("Debugging is not recommended. It is very unstable.");
    std::string debugger = getDebugger().getDebuggerFilePath();
    std::string debuggee = getDebugger().getDebuggeeFilePath();
    if(debugger.length() > 0)
        {
        if(debuggee.length() > 0)
            {
// The debugger could be on the path.
//          if(fileExists(debugger))
                {
                bool success = true;
                if(FileIsFileOnDisk(debuggee, success))
                    {
                    ok = true;
                    }
                else
                    Gui::messageBox("Component to debug in Edit/Preferences does not exist");
                }
//          else
//              Gui::messageBox("Debugger in Oovcde Analysis/Settings does not exist");
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

void EditFiles::setTabText(FileEditView *editView, OovStringRef text)
    {
    GtkNotebook *book = getBook(editView);
    GtkWidget *widget = GTK_WIDGET(getScrolledFileView(editView->getTextView())->
            getViewTopParent());

    GtkWidget *tabLabel = gtk_notebook_get_tab_label(book, widget);
    /// The top widget is a box container that is added using gtk_notebook_append_page.
    /// The first child in the box is the label, the second is the close button.
    GList *children = gtk_container_get_children(GTK_CONTAINER(tabLabel));
    if(children)
		{
    	/// Set text of first child
    	Gui::setText(GTK_LABEL(children->data), text);
		}
    g_list_free(children);

//    gtk_notebook_set_tab_label_text(book, widget, text.getStr());
    }

bool EditFiles::saveAsTextFileWithDialog()
    {
    FileEditView *editView = getEditView();
    bool saved = editView->saveAsTextFileWithDialog();
    if(saved)
        {
        setTabText(editView, editView->getFileName());
        }
    return saved;
    }

void EditFiles::textBufferModified(FileEditView *editView, bool modified)
    {
    OovString text = editView->getFileName();
    if(modified)
        text += '*';
    setTabText(editView, text);
    }

void EditFiles::bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len)
    {
    ScrolledFileView *scrolledView = getScrolledFileView(textbuffer);
    if(scrolledView)
        {
        scrolledView->getFileEditView().bufferInsertText(
                textbuffer, location, text, len);
        }
    }

void EditFiles::bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end)
    {
    ScrolledFileView *scrolledView = getScrolledFileView(textbuffer);
    if(scrolledView)
        {
        scrolledView->getFileEditView().bufferDeleteRange(
                textbuffer, start, end);
        }
    }

bool EditFiles::handleButtonPress(GtkWidget *widget, GdkEventButton const &button)
    {
    bool handled = false;
    if(button.type == GDK_2BUTTON_PRESS)
        {
        ScrolledFileView *scrolledView = getScrolledFileView(GTK_TEXT_VIEW(widget));
        if(scrolledView)
            {
            scrolledView->getFileEditView().buttonPressSelect(
                    scrolledView->getLeftMargin().getMarginWidth(),
                    button.x, button.y);
            Debugger &deb = sEditFiles->getDebugger();
            if(deb.isDebuggerRunning())
                {
                sEditFiles->showInteractNotebookTab("Data");
                deb.startGetVariable(Gui::getSelectedText(
                        scrolledView->getTextView()));
                }
            }
        handled = true;
        }
    return handled;
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
            DebuggerBreakpoint bp(view->getFilePath(), line);
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
    const char *str = Gui::getSelectedText(view->getTextView());
    sDbgFile.printflush("ViewVar %s\n", str);
#endif
            }
        }
    return false;
    }


extern "C" G_MODULE_EXPORT void on_MainDebugGoToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
        sEditFiles->getDebugger().resume();
    }

extern "C" G_MODULE_EXPORT void on_MainDebugStopToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    if(sEditFiles)
        sEditFiles->getDebugger().stop();
    }

extern "C" G_MODULE_EXPORT void on_MainDebugStepIntoToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
        sEditFiles->getDebugger().stepInto();
    }

extern "C" G_MODULE_EXPORT void on_MainDebugStepOverToolbutton_clicked(GtkWidget * /*widget*/, gpointer data)
    {
    if(sEditFiles && sEditFiles->checkDebugger())
        sEditFiles->getDebugger().stepOver();
    }
