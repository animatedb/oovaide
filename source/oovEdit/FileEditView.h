/*
 * FileEditView.h
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef FILEEDITVIEW_H_
#define FILEEDITVIEW_H_


#include "Highlighter.h"
#include "Indenter.h"
#include "History.h"

#define USE_NEW_TIME 1
#if(USE_NEW_TIME)
//#define __cplusplus 201403L
#include <chrono>
#endif

// This is written so that when the completion list is visible, it should
// have focus. List navigation keys will be used to move in the list,
// and other keys will be forwarded to the editor window.
class CompletionList
    {
    public:
        CompletionList():
            mEditView(nullptr), mTopWidget(nullptr), mTextBuffer(nullptr),
            mStartIdentifierOffset(0), mCompletionTriggerPointOffset(0),
            mLastKey(0), mLastNonModifierKey(0), mGettingCompletionData(false)
#if(!USE_NEW_TIME)
            , mGettingCompletionDataStartTime(0)
#endif
            {}
        void init(GtkTextBuffer *textBuffer);
        void positionCompletionWindow();
        // Detects when to display the completion list.
        // Return indicates to start getting completion data.
        bool handleEditorKey(int key, int modKeys);
        bool handleListKey(int key, int modKeys);
        void setList(class FileEditView *view, OovStringVec const &strs);
        void activateSelectedItem();
        void lostFocus();
        int getCompletionTriggerPointOffset()
            { return mCompletionTriggerPointOffset; }
        bool isGettingCompletionData() const
            { return mGettingCompletionData; }
        bool okToShowList() const;

    private:
        FileEditView *mEditView;
        GtkWidget *mTopWidget;
        GuiList mGuiList;
        GtkTextBuffer *mTextBuffer;
        int mStartIdentifierOffset;
        int mCompletionTriggerPointOffset;
        int mLastKey;
        int mLastNonModifierKey;
        bool mGettingCompletionData;
#if(USE_NEW_TIME)
        /// https://www.eclipse.org/forums/index.php/t/490066/
        std::chrono::high_resolution_clock::time_point mGettingCompletionDataStartTime;
#else
        time_t mGettingCompletionDataStartTime;
#endif
        void setWindowPosition(GdkWindow *compWin, int screenWidth);
        void quitGettingCompletionData()
            { mGettingCompletionData = false; }
        void startGettingCompletionData();
    };

class FileEditViewListener
    {
    public:
        virtual void textBufferModified(class FileEditView *view, bool modified) = 0;
    };

class FileEditView
    {
    public:
        FileEditView():
            mTextView(nullptr), mTextBuffer(nullptr), mCurHistoryPos(0),
            mDoingHistory(false), mLastViewTopOffset(0), mLastViewBotOffset(0),
            mHighlightTextContentChange(false), mListener(nullptr)
            {}
        void init(GtkTextView *textView, FileEditViewListener *listener);
        bool openTextFile(OovStringRef const fn);
        bool saveTextFile();
        bool saveAsTextFileWithDialog();
        bool checkExitSave();
        std::string getSelectedText();
        bool find(char const * const findStr, bool forward, bool caseSensitive);
        bool findAndReplace(char const * const findStr, bool forward,
            bool caseSensitive, char const * const replaceStr);
        void findToken(eFindTokenTypes ft)
            { mHighlighter.findToken(ft, GuiTextBuffer::getCursorOffset(mTextBuffer)); }
        void getFindTokenResults(std::string &fn, size_t &offset)
            { mHighlighter.getFindTokenResults(fn, offset); }
        OovString getClassNameAtLocation()
            {
            return mHighlighter.getClassNameAtLocation(
                    GuiTextBuffer::getCursorOffset(mTextBuffer));
            }
        void getMethodNameAtLocation(OovString &className, OovString &methodName)
            {
            mHighlighter.getMethodNameAtLocation(
                GuiTextBuffer::getCursorOffset(mTextBuffer), className, methodName);
            }
        void cut()
            {
            GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_cut_clipboard(mTextBuffer, clipboard, true);
            }
        void copy()
            {
            GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_copy_clipboard(mTextBuffer, clipboard);
            }
        void paste()
            {
            GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_paste_clipboard(mTextBuffer, clipboard, NULL, true);
            }
        void deleteSel()
            {
            gtk_text_buffer_delete_selection(mTextBuffer, true, true);
            }
        void undo()
            {
            mDoingHistory = true;
            if(mCurHistoryPos > 0)
                { mHistoryItems[--mCurHistoryPos].undo(mTextBuffer); }
            mDoingHistory = false;
            }
        void redo()
            {
            mDoingHistory = true;
            if(mCurHistoryPos < mHistoryItems.size())
                { mHistoryItems[mCurHistoryPos++].redo(mTextBuffer); }
            mDoingHistory = false;
            }
        void addHistoryItem(const HistoryItem &item)
            {
            if(mCurHistoryPos < mHistoryItems.size())
                { mHistoryItems[mCurHistoryPos] = item; }
            else
                { mHistoryItems.push_back(item); }
            mCurHistoryPos++;
            }
        bool doingHistory() const
            { return mDoingHistory; }
        GtkTextView *getTextView() const
            { return mTextView; }
        GtkWindow *getWindow()
            {
            GtkWindow *wnd = nullptr;
            if(mTextView)
            wnd = GTK_WINDOW(mTextView);
            return wnd;
            }
            // Return = true if find def/decl has results.
        bool idleHighlight();
        void gotoLine(int lineNum);

        // These are called by callbacks.
        void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
                gchar *text, gint len);
        void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
                GtkTextIter *end);
        /// Used to process keys that require special handling.  These are
        /// things like code completion, indenting, home key, etc.
        bool handleKeys(GdkEvent *event);
        std::string getFilePath() const
            { return mFilePath; }
        // Only the name part of the file path
        std::string getFileName() const;
        GtkTextBuffer *getTextBuffer() const
            { return mTextBuffer; }
        Highlighter &getHighlighter()
            { return mHighlighter; }
        bool viewChange(int &topOffset, int &botOffset);
        void highlightedContent()
            { mHighlightTextContentChange = false; }

    private:
        std::string mFilePath;
        GtkTextView *mTextView;
        GtkTextBuffer *mTextBuffer;
        std::vector<HistoryItem> mHistoryItems;
        size_t mCurHistoryPos;          // Range is 0 to size()-1.
        bool mDoingHistory;             // Doing undo or redo.
        int mLastViewTopOffset;
        int mLastViewBotOffset;
        bool mHighlightTextContentChange;
        Highlighter mHighlighter;
        Indenter mIndenter;
        CompletionList mCompleteList;
        FileEditViewListener *mListener;

        void setFileName(OovStringRef const fn)
            { mFilePath = fn; }
        bool saveAsTextFile(OovStringRef const fn);
        void highlightRequest();
        void moveToIter(GtkTextIter startIter, GtkTextIter *endIter=NULL);
        GuiText getBuffer();
        void setModified(bool modified)
            {
            if(mListener)
                mListener->textBufferModified(this, modified);
            }
    };


#endif /* FILEEDITVIEW_H_ */
