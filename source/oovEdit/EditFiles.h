/*
 * EditFiles.h
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef EDIT_FILES_H_
#define EDIT_FILES_H_

#include "Gui.h"
#include "FileEditView.h"
#include "Debugger.h"
#include "FilePath.h"
#include "EditOptions.h"
#include <sys/time.h>
#include <vector>
#include <memory>


class LeftMargin
    {
    public:
        LeftMargin():
            mMarginLayout(nullptr), mMarginAttr(nullptr), mMarginAttrList(nullptr),
            mTextWidth(0), mPixPerChar(10)
            {}
        ~LeftMargin();
        void setupMargin(GtkTextView *textView);
        void drawMarginLineNum(GtkTextView *textView, cairo_t *cr, int lineNum, int yPos);
        void drawMarginLine(GtkTextView *textView, cairo_t *cr);
        int getMarginWidth() const;
        int getMarginHeight(GtkTextView *textView) const;
        int getMarginLineWidth() const
            { return 1; }
        int getPixPerChar() const
            { return mPixPerChar; }

    private:
        PangoLayout *mMarginLayout;
        PangoAttribute *mMarginAttr;
        PangoAttrList *mMarginAttrList;
        int mTextWidth;
        int mPixPerChar;
        void updateTextInfo(GtkTextView *textView);
        int getBeforeMarginLineSepWidth() const
            { return 2; }
        int getAfterMarginLineSepWidth() const
            { return 1; }
    };

class ScrolledFileView
    {
    public:
        ScrolledFileView(Debugger &debugger):
            mDebugger(debugger), mScrolled(nullptr), mDesiredLine(-1)
            {}
        GtkNotebook *getBook()
            {
            return GTK_NOTEBOOK(gtk_widget_get_ancestor(GTK_WIDGET(mScrolled),
                    GTK_TYPE_NOTEBOOK));
            }
        const GtkTextView *getTextView() const
            { return mFileView.getTextView(); }
        GtkTextView *getTextView()
            { return mFileView.getTextView(); }
        GtkWidget *getViewTopParent()
            { return GTK_WIDGET(mScrolled); }
        FileEditView &getFileEditView()
            { return mFileView; }
        // WARNING: This path was entered by the user, so it may not have the
        FilePath const &getFilename() const
            { return mFilename; }
        LeftMargin const &getLeftMargin() const
            { return mLeftMargin; }
        void drawMargins(cairo_t *cr);

    public:
        Debugger &mDebugger;
        FilePath mFilename;
        GtkScrolledWindow *mScrolled;
        FileEditView mFileView;
        int mDesiredLine;
        LeftMargin mLeftMargin;
        /// Draws the left margin along with the line numbers.
        void drawLeftMargin(cairo_t *cr);
        /// Draws the right margin near 80 columns.
        void drawRightMargin(cairo_t *cr);
    };

class EditFiles:public FileEditViewListener
    {
    public:
        EditFiles(Debugger &debugger, EditOptions &editOptions);
        static EditFiles &getEditFiles();
        void init(Builder &builder);
        // This prevents a crash if called before the windows
        // are shut down. See Tokenizer::~Tokenizer().
        void closeAll();
        void onIdle();
        void updateDebugMenu();
        /// Opens the specified file, along with the companion file. Ex: If a
        /// source file is specified, then the header is also opened.
        /// @param fn Full filename.
        /// @param lineNum Line number of specified file.
        void viewModule(OovStringRef const fn, int lineNum);
        /// @param fn Full filename.
        /// @param lineNum Line number of specified file.
        void viewFile(OovStringRef const fn, int lineNum);
        /// Go to the line number in the current view.
        /// @param lineNum The line number to put the cursor on.
        void gotoLine(int lineNum);
        /// Displays a prompt if a buffer is modified.
        /// return = true if it is ok to exit.
        bool checkExitSave();
        FileEditView *getEditView();
        static ScrolledFileView *getScrolledFileView(GtkTextView *textView);
        static ScrolledFileView *getScrolledFileView(GtkTextBuffer *textbuffer);
        std::string getEditViewSelectedText();
        // For signal handlers
        void setFocusEditTextView(GtkTextView *editTextView);
        /// Handles keys where the behavior is modified. The main keys are
        /// related to indenting.
        bool handleKeyPress(GdkEvent *event);
        Debugger &getDebugger()
            { return mDebugger; }
        /// Used by signal handler to close the page.
        void removeNotebookPage(GtkWidget *pageWidget);
        /// Displays an error if the debugger has not been setup in options.
        bool checkDebugger();
        void showInteractNotebookTab(char const * const tabName);
        bool saveAsTextFileWithDialog();
        static void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
                gchar *text, gint len);
        static void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
                GtkTextIter *end);
        static bool handleButtonPress(GtkWidget *widget, GdkEventButton const &button);

    private:
        EditOptions &mEditOptions;
        GtkNotebook *mHeaderBook;
        GtkNotebook *mSourceBook;
        Builder *mBuilder;
        std::vector<std::unique_ptr<ScrolledFileView>> mFileViews;
        GtkTextView *mLastFocusGtkTextView;
        Debugger &mDebugger;
        timeval mLastHightlightIdleUpdate;
        void idleHighlight();
        int getPageNumber(GtkNotebook *notebook, GtkTextView const *view) const;
        void setTabText(FileEditView *view, OovStringRef text);
        void textBufferModified(FileEditView *view, bool modified);
        GtkNotebook *getBook(FileEditView *view);
    };



#endif /* EDIT_FILES_H_ */
