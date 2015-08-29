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
#include "EditFiles.h"
#include <string.h>


#include "IncludeMap.h"
#include "BuildConfigReader.h"
#include "OovProcess.h"

static CompletionList *sCurrentCompleteList;


// This currently does not get the package command line arguments,
// but usually these won't be needed for compilation.
static void getCppArgs(OovStringRef const srcName, OovProcessChildArgs &args)
    {
    ProjectReader proj;
    ProjectBuildArgs buildArgs(proj);
    proj.readProject(Project::getProjectDirectory());
    buildArgs.loadBuildArgs(BuildConfigAnalysis);
    OovStringVec cppArgs = buildArgs.getCompileArgs();
    for(auto const &arg : cppArgs)
        {
        args.addArg(arg);
        }

    BuildConfigReader cfg;
    std::string incDepsPath = cfg.getIncDepsFilePath();
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


static void signalBufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
        gchar *text, gint len, gpointer user_data)
    {
    EditFiles::getEditFiles().bufferInsertText(textbuffer, location, text, len);
    }

static void signalBufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
        GtkTextIter *end, gpointer user_data)
    {
    EditFiles::getEditFiles().bufferDeleteRange(textbuffer, start, end);
    }

static bool signalButtonPressEvent(GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
    return EditFiles::getEditFiles().handleButtonPress(widget, event->button);
    }


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
            key == GDK_KEY_Home || key == GDK_KEY_KP_Home ||
            key == GDK_KEY_End || key == GDK_KEY_KP_End ||
            key == GDK_KEY_Page_Up || key == GDK_KEY_KP_Page_Up ||
            key == GDK_KEY_Page_Down || key == GDK_KEY_KP_Page_Down ||
            key == GDK_KEY_Return);
    }

static inline bool isControlKey(int key)
    {
    return (key == GDK_KEY_Control_L || key == GDK_KEY_Control_R);
    }

static bool isModifierKey(int key)
    {
    return(key == GDK_KEY_Shift_R || key == GDK_KEY_Shift_L || isControlKey(key));
    }

bool CompletionList::handleEditorKey(int key, int modKeys)
    {
    sCurrentCompleteList = this;
    bool getCompletionData = false;
    if((key == '.' || (mLastNonModifierKey == '-' && key == '>')) ||
        (key == ' ' && isControlKey(mLastKey))
            )
        {
        getCompletionData = true;
        startGettingCompletionData();
        mCompletionTriggerPointOffset = GuiTextBuffer::getCursorOffset(mTextBuffer);

        if(key == ' ')
            {
            mStartIdentifierOffset = mCompletionTriggerPointOffset;
            GtkTextIter iter = GuiTextBuffer::getCursorIter(mTextBuffer);
            while(GuiTextBuffer::decIter(&iter))
                {
                int offset = GuiTextBuffer::getIterOffset(iter);
                char c = GuiTextBuffer::getChar(mTextBuffer, offset);
                if(!isIdentC(c))
                    break;
                mStartIdentifierOffset = offset;
                }
            }
        else
            {
            mCompletionTriggerPointOffset++;
            mStartIdentifierOffset = mCompletionTriggerPointOffset;
            }

        mGuiList.clear();

/*
        GtkTreePath *path = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(mGuiList.getTreeView(), path, nullptr, false);
        gtk_tree_path_free(path);
*/
//        gtk_widget_set_can_focus(mTopWidget, TRUE);
//        gtk_window_set_decorated(GTK_WINDOW(mTopWidget), FALSE);
//        gtk_window_set_type_hint(GTK_WINDOW(mTopWidget), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
//        gtk_window_set_transient_for(GTK_WINDOW(mTopWidget), main_top_level_window);
//        gtk_widget_grab_focus(GTK_WIDGET(mGuiList.getTreeView()));
        }
    /// The completion list can have identifiers and functions. When it has
    /// functions, then it can have a few other items in the list such as
    /// parenthesis, pointers, spaces, etc., but if the user typed in that
    /// much, then there is probably no reason to display the list.
    ///
    /// The identifier keys should really be sent to the list when it displays,
    /// so for now, just train the user to quit typing until the list displays.
    else if(isGettingCompletionData() /*&& (!isIdentC(key) && (key != GDK_KEY_BackSpace))*/)
        {
        quitGettingCompletionData();
        }
    if(!isModifierKey(key))
        {
        mLastNonModifierKey = key;
        }
    mLastKey = key;
    return getCompletionData;
    }


/*
 * Classes of keys:
 *      List control keys (arrows, return)      !quit list,     !modify editbuf,        list movement,
 *      Modifier keys (shift)                   !quit list,     !modify editbuf,
 *      Delete/backspace                        !quit list,     modify editbuf
 *      Non-insertable (esc)                    quit list,      !modify editbuf
 *      punctuation (parens, comma)             quit list,      modify editbuf
 *      identifiers                             !quit list,     modify editbuf
 */
/// @todo - Need to handle searching list
/// @todo - Need to position completion window
bool CompletionList::handleListKey(int key, int modKeys)
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
    /// @todo - Ctrl-Z does should get sent to the editor instead of discarding
    /// and quitting the list.
    else if(isInsertableC(key) && !(modKeys & GDK_CONTROL_MASK))
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
    if(Gui::isVisible(mTopWidget) && handled)
        {
        int cursOffset = GuiTextBuffer::getCursorOffset(mTextBuffer);
        OovString str = GuiTextBuffer::getText(mTextBuffer,
            mStartIdentifierOffset, cursOffset);
        mGuiList.setSelected(mGuiList.findLongestMatch(str));
        }
    if(doneList)
        {
        quitGettingCompletionData();
        Gui::setVisible(mTopWidget, false);
        }
    return handled;
    }

void CompletionList::lostFocus()
    {
    Gui::setVisible(mTopWidget, false);
    }

void CompletionList::startGettingCompletionData()
    {
    mGettingCompletionData = true;
#if(USE_NEW_TIME)
    mGettingCompletionDataStartTime = std::chrono::high_resolution_clock::now();
#else
    time(&mGettingCompletionDataStartTime);
#endif
    }

bool CompletionList::okToShowList() const
    {
#if(USE_NEW_TIME)
	std::chrono::high_resolution_clock::time_point currentTime =
		std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> timeSpan = std::chrono::duration_cast<
		std::chrono::duration<double>>(currentTime - mGettingCompletionDataStartTime);
    bool timeIsUp = (timeSpan.count() > std::chrono::seconds(1).count());
#else
    time_t curTime;
    time(&curTime);
    /// The time resolution is poor, so the time will vary between 1 and 2 seconds.
    bool timeIsUp = (abs(mGettingCompletionDataTime - curTime) > 1);
#endif
    return(mGettingCompletionData && timeIsUp);
    }

void CompletionList::setList(FileEditView *view, OovStringVec const &strs)
    {
    if(strs.size() > 0)
        {
        mEditView = view;
        mGuiList.clear();

        // When the completion point is on a member operator, then there is
        // no identifier, and the identifier offset is after the trigger point.
        // If the completion point is on the middle of an identifier, then
        // the identifier is before the completion trigger point.
        OovString prefix;
        if(mStartIdentifierOffset < mCompletionTriggerPointOffset)
            {
            prefix = GuiTextBuffer::getText(mTextBuffer,
                mStartIdentifierOffset, mCompletionTriggerPointOffset);
            }
        for(auto const &str : strs)
            {
            if(prefix.compare(0, prefix.length(), str, 0, prefix.length()) == 0)
                {
                mGuiList.appendText(str);
                }
            }
// For some reason in linux, the top widget never gets the focus.
#ifndef __linux__
        Gui::setVisible(mTopWidget, true);
#endif
        mGuiList.setSelected(strs[0]);
        quitGettingCompletionData();
        }
    }

void CompletionList::setWindowPosition(GdkWindow *compWin, int screenWidth)
    {
    // Place the completion window on the opposite side of the screen as
    // the editor cursor.
    if(mEditView)
        {
        GtkTextIter cursIter = GuiTextBuffer::getCursorIter(
            mEditView->getTextBuffer());
        GdkRectangle cursRect;
        gtk_text_view_get_iter_location(mEditView->getTextView(), &cursIter,
            &cursRect);
        GdkWindow *win = gtk_text_view_get_window(mEditView->getTextView(),
            GTK_TEXT_WINDOW_WIDGET);
        int winX;
        int winY;
        gdk_window_get_origin(win, &winX, &winY);
        int cursPosX;
        int cursPosY;
        gtk_text_view_buffer_to_window_coords(mEditView->getTextView(),
            GTK_TEXT_WINDOW_WIDGET,
            cursRect.x, cursRect.y, &cursPosX, &cursPosY);
        if(cursPosX + winX > screenWidth / 2)
            {
            gdk_window_move(compWin, 0, 0);
            }
        else
            {
            gdk_window_move(compWin, screenWidth / 2, 0);
            }
        }
    }

void CompletionList::positionCompletionWindow()
    {
    GtkWidget *treeViewWidget = GTK_WIDGET(mGuiList.getTreeView());
    int screenHeight = gdk_screen_get_height(gtk_widget_get_screen(treeViewWidget));
    int screenWidth = gdk_screen_get_width(gtk_widget_get_screen(treeViewWidget));

    GtkWidget *compWidget = Builder::getBuilder()->getWidget("CompletionListWindow");
    GdkWindow *compWin = gtk_widget_get_window(compWidget);
    setWindowPosition(compWin, screenWidth);

    GtkRequisition min, nat;
    gtk_widget_get_preferred_size(treeViewWidget, &min, &nat);

    // Windows has the task bar at the bottom, so this has to be smaller than that.
    int winHeight = screenHeight * .9;
    int winWidth = screenWidth * .9;
    if(nat.height > winHeight)
        {
        nat.height = winHeight;
        }
    else
        {
        nat.height += 10; //gtk_container_get_border_height(GTK_CONTAINER(sw));
        }
    if(nat.width > winWidth / 2)
        {
        nat.width = winWidth / 2;
        }
    gdk_window_resize(compWin, nat.width, nat.height);
    }

void CompletionList::activateSelectedItem()
    {
    int cursOffset = GuiTextBuffer::getCursorOffset(mTextBuffer);
    if(mStartIdentifierOffset < cursOffset)
        {
        GuiTextBuffer::erase(mTextBuffer, mStartIdentifierOffset, cursOffset);
        }
    OovString str = mGuiList.getSelected();
    size_t spacePos = str.find(' ');
    if(spacePos != std::string::npos)
        {
        str.resize(spacePos);
        }
    gtk_text_buffer_insert_at_cursor(mTextBuffer, str.getStr(), str.length());
    Gui::setVisible(mTopWidget, false);
    }

static int textViewWindowToBufferOffset(GtkTextView *textView, int winX, int winY)
    {
    int bufX;
    int bufY;
    gtk_text_view_window_to_buffer_coords(textView, GTK_TEXT_WINDOW_WIDGET,
            winX, winY, &bufX, &bufY);
    GtkTextIter iter;
    gtk_text_view_get_iter_at_location(textView, &iter, bufX, bufY);
    return gtk_text_iter_get_offset(&iter);
    }

//static void scrollChild(GtkWidget *widget, GdkEvent *event, gpointer user_data);

gboolean draw(GtkWidget *widget, void *cr, gpointer user_data)
    {
    bool handled = false;
    FileEditView *view = static_cast<FileEditView*>(user_data);
    int topOffset;
    int botOffset;
    if(view->viewChange(topOffset, botOffset))
        {
        GtkTextBuffer *textBuf = view->getTextBuffer();
        if(view->getHighlighter().applyTags(textBuf, topOffset, botOffset))
            {
            handled = true;
            view->highlightedContent();
            }
        }
    return handled;
    }


void FileEditView::init(GtkTextView *textView, FileEditViewListener *listener)
    {
    mTextView = textView;
    mListener = listener;
    mTextBuffer = gtk_text_view_get_buffer(mTextView);
    mIndenter.init(mTextBuffer);
    mCompleteList.init(mTextBuffer);
    gtk_widget_grab_focus(GTK_WIDGET(mTextView));
    g_signal_connect(G_OBJECT(mTextBuffer), "insert-text",
          G_CALLBACK(signalBufferInsertText), NULL);
    g_signal_connect(G_OBJECT(mTextBuffer), "delete-range",
          G_CALLBACK(signalBufferDeleteRange), NULL);
    g_signal_connect(G_OBJECT(mTextView), "button-press-event",
          G_CALLBACK(signalButtonPressEvent), NULL);
//          g_signal_connect(G_OBJECT(mBuilder.getWidget("EditTextScrolledwindow")),
//                  "scroll-child", G_CALLBACK(scrollChild), NULL);
    g_signal_connect(G_OBJECT(mTextView), "draw", G_CALLBACK(draw), this);
    }

bool FileEditView::openTextFile(OovStringRef const fn)
    {
    setFileName(fn);
    File file(fn, "rb");
    if(file.isOpen())
        {
        fseek(file.getFp(), 0, SEEK_END);
        long fileSize = ftell(file.getFp());
        if(fileSize >= 0 && fileSize < 10000000)
            {
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
        }
    return(file.isOpen());
    }

std::string FileEditView::getFileName() const
    {
    return FilePath(mFilePath, FP_File).getName();
    }

bool FileEditView::saveTextFile()
    {
    bool success = false;
    if(mFilePath.length() == 0)
        {
        success = saveAsTextFileWithDialog();
        }
    else
        {
        success = true;
        }
    if(success)
        {
        success = saveAsTextFile(mFilePath);
        }
    return success;
    }

bool FileEditView::viewChange(int &topOffset, int &botOffset)
    {
    GtkTextView *textView = getTextView();
    GdkWindow *textWindow = gtk_text_view_get_window(textView, GTK_TEXT_WINDOW_WIDGET);
    int winX;
    int winY;
    gdk_window_get_position(textWindow, &winX, &winY);
    topOffset = textViewWindowToBufferOffset(textView, winX, winY);

    int width = gdk_window_get_width(textWindow);
    int height = gdk_window_get_height(textWindow);
    botOffset = textViewWindowToBufferOffset(textView, winX+width, winY+height);
    bool changed = (mLastViewTopOffset != topOffset ||
            mLastViewBotOffset != botOffset || mHighlightTextContentChange);
    if(changed)
        {
        mLastViewTopOffset = topOffset;
        mLastViewBotOffset = botOffset;
        }
    return changed;
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
    if(mFilePath.length())
        {
    //  int numArgs = 0;
    //  char const * cppArgv[40];
        OovProcessChildArgs cppArgs;
        getCppArgs(mFilePath, cppArgs);

        FilePath path;
        path.getAbsolutePath(mFilePath, FP_File);
        mHighlighter.highlightRequest(path, cppArgs.getArgv(), cppArgs.getArgc());
        mHighlightTextContentChange = true;
        }
    }

void FileEditView::gotoLine(int lineNum)
    {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &iter, lineNum-1);
    GtkTextIter endIter;
    gtk_text_buffer_get_iter_at_line(mTextBuffer, &endIter, lineNum);
    moveToIter(iter, &endIter);
    }

void FileEditView::moveToIter(GtkTextIter startIter, GtkTextIter *endIter)
    {
    GtkTextMark *mark = gtk_text_buffer_get_insert(mTextBuffer);
    if(mark)
        {
//              The solution involves creating an idle proc (which is a procedure which
//              GTK will call when it has nothing else to do until told otherwise by the
//              return value of this procedure). In that idle proc the source view is
//              scrolled constantly, until it is determined that the source view has in
//              fact scrolled to the location desired. This was accomplished by using a couple of nice functions:
//                  gtk_get_iter_location to get the location of the cursor.
//                  gtk_text_view_get_visible_rect to get the rectangle of the visible part of the document in the source view.
//                  gtk_intersect to check whether the cursor is in the visible rectangle.
//              gtk_text_buffer_get_iter_at_mark(mTextBuffer, &iter, mark);

        GtkTextIter tempEndIter = startIter;
        if(endIter)
            tempEndIter = *endIter;
        gtk_text_buffer_select_range(mTextBuffer, &startIter, &tempEndIter);
        gtk_text_buffer_move_mark(mTextBuffer, mark, &startIter);
        Gui::scrollToCursor(mTextView);
//      gtk_text_view_scroll_to_mark(mTextView, mark, 0, TRUE, 0, 0.5);
        }
    }

bool FileEditView::saveAsTextFileWithDialog()
    {
    PathChooser ch;
    OovString filename = mFilePath;
    bool saved = false;
    std::string prompt = "Save ";
    prompt += mFilePath;
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
        setModified(false);
        }
    return(writeSize > 0);
    }

bool FileEditView::checkExitSave()
    {
    bool exitOk = true;
    if(gtk_text_buffer_get_modified(mTextBuffer))
        {
        std::string prompt = "Save ";
        prompt += mFilePath;
        GtkDialog *dlg = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", prompt.c_str()));
        gtk_dialog_add_button(dlg, GUI_CANCEL, GTK_RESPONSE_CANCEL);
        gint result = gtk_dialog_run(dlg);
        if(result == GTK_RESPONSE_YES)
            {
            if(mFilePath.length() > 0)
                {
                exitOk = saveTextFile();
                }
            else
                {
                exitOk = saveAsTextFileWithDialog();
                }
            }
        else if(result == GTK_RESPONSE_NO)
            {
            exitOk = true;
            }
        else
            {
            exitOk = false;
            }
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
        {
        moveToIter(startMatch, &endMatch);
        }
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
    setModified(true);
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
    setModified(true);
    }

void FileEditView::buttonPressSelect(int leftMarginWidth, double buttonX, double buttonY)
    {
    int bufX;
    int bufY;
    gtk_text_view_window_to_buffer_coords(mTextView, GTK_TEXT_WINDOW_WIDGET,
        leftMarginWidth + buttonX, buttonY, &bufX, &bufY);
    GtkTextIter iter;
    gtk_text_view_get_iter_at_location(mTextView, &iter, bufX, bufY);
    GtkTextIter startIter = iter;
    GtkTextIter endIter = iter;
    while(1)
        {
        char c = gtk_text_iter_get_char(&iter);
        if(isIdentC(c))
            {
            startIter = iter;
            if(!gtk_text_iter_backward_char(&iter))
                {
                break;
                }
            }
        else
            {
            break;
            }
        }
    iter = endIter;
    while(1)
        {
        char c = gtk_text_iter_get_char(&iter);
        if(isIdentC(c))
            {
            if(!gtk_text_iter_forward_char(&iter))
                {
                break;
                }
            endIter = iter;
            }
        else
            {
            break;
            }
        }
    gtk_text_buffer_select_range(mTextBuffer, &startIter, &endIter);
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
        if(mCompleteList.okToShowList())
            {
            OovStringVec members = mHighlighter.getShowMembers();
            mCompleteList.setList(this, members);
            }
        }
    return foundToken;
    }

bool FileEditView::handleKeys(GdkEvent *event)
    {
    bool handled = false;

    int modKeys = (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK) & event->key.state;
    if(mCompleteList.handleEditorKey(event->key.keyval, modKeys))
        {
#if(CODE_COMPLETE)
        mHighlighter.showMembers(mCompleteList.getCompletionTriggerPointOffset());
#endif
        }

    switch(event->key.keyval)
        {
        case '>':       // Checking for "->"
        case '.':
            {
#if(!CODE_COMPLETE)
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

        case GDK_KEY_ISO_Left_Tab:      // This is shift tab on PC
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
    int modKeys = (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK) & event->key.state;
    return sCurrentCompleteList->handleListKey(event->key.keyval, modKeys);
    }

extern "C" G_MODULE_EXPORT void on_CompletionListTreeview_focus_out_event(GtkWidget *widget, gpointer data)
    {
    sCurrentCompleteList->lostFocus();
    }

extern "C" G_MODULE_EXPORT bool on_CompletionListTreeview_draw(GtkWidget *widget, gpointer data)
    {
    sCurrentCompleteList->positionCompletionWindow();
    return false;
    }

/*
extern "C" G_MODULE_EXPORT void on_ViewsPaned_check_resize(GtkWidget *widget, gpointer data)
    {
    GtkWindow *window = GTK_WINDOW(widget);
    int width;
    int height;
    gtk_window_get_size(window, &width, &height);
    gtk_paned_set_position(GTK_PANED(widget), width/2);
    }

extern "C" G_MODULE_EXPORT void on_ControlPaned_check_resize(GtkWidget *widget, gpointer data)
    {
    GtkWindow *window = GTK_WINDOW(widget);
    int width;
    int height;
    gtk_window_get_size(window, &width, &height);
    gtk_paned_set_position(GTK_PANED(widget), height * 3/4);
    }
*/
