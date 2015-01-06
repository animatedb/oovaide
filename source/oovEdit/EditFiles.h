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


struct ScrolledFileView
    {
    ScrolledFileView():
    mScrolled(nullptr), mDesiredLine(-1)
	{}
    GtkNotebook *getBook()
	{
	return GTK_NOTEBOOK(gtk_widget_get_ancestor(GTK_WIDGET(mScrolled),
		GTK_TYPE_NOTEBOOK));
	}
    const GtkTextView *getTextView() const
	{ return mFileView.getTextView(); }
    GtkWidget *getViewTopParent()
	{ return GTK_WIDGET(mScrolled); }
    FileEditView &getFileEditView()
	{ return mFileView; }
    // WARNING: This path was entered by the user, so it may not have the
    FilePath const &getFilename() const
	{ return mFilename; }

    // WARNING: This path was entered by the user, so it may not have the
    // correct case in Windows.
    FilePath mFilename;
    GtkScrolledWindow *mScrolled;
    FileEditView mFileView;
    int mDesiredLine;
    };

class EditFiles
    {
    public:
	EditFiles(Debugger &debugger, EditOptions &editOptions);
	void init(Builder &builder);
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
	std::string getEditViewSelectedText();
	// For signal handlers
	void setFocusEditTextView(GtkTextView *editTextView);
	/// Handles keys where the behavior is modified. The main keys are
	/// related to indenting.
	bool handleKeyPress(GdkEvent *event);
	/// Draws the left margin along with the line numbers.
	void drawLeftMargin(GtkWidget *widget, cairo_t *cr, int &width, int &pixPerChar);
	/// Draws the right margin near 80 columns.
	void drawRightMargin(GtkWidget *widget, cairo_t *cr, int leftMargin, int pixPerChar);
	Debugger &getDebugger()
	    { return mDebugger; }
	/// Used by signal handler to close the page.
	void removeNotebookPage(GtkWidget *pageWidget);
	/// Displays an error if the debugger has not been setup in options.
	bool checkDebugger();
	void showInteractNotebookTab(char const * const tabName);

    private:
	EditOptions &mEditOptions;
	GtkNotebook *mHeaderBook;
	GtkNotebook *mSourceBook;
	Builder *mBuilder;
	std::vector<std::unique_ptr<ScrolledFileView>> mFileViews;
	size_t mFocusEditViewIndex;
	Debugger &mDebugger;
	timeval mLastHightlightIdleUpdate;
	void idleHighlight();
	int getPageNumber(GtkNotebook *notebook, GtkTextView const *view) const;
	ScrolledFileView *getScrolledFileView()
	    {
	    if(mFocusEditViewIndex < mFileViews.size())
		return mFileViews[mFocusEditViewIndex].get();
	    else
		return nullptr;
	    }
    };



#endif /* EDIT_FILES_H_ */
