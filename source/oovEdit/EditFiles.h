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
#include <vector>


class EditFiles
    {
    public:
	EditFiles();
	void init(GtkNotebook *headerBook, GtkNotebook *srcBook);
	void onIdle();
	void viewFile(char const * const fn, int lineNum);
	void gotoLine(int lineNum);
	bool checkExitSave();
	FileEditView *getEditView()
	    {
	    if(mFocusEditViewIndex < mFileViews.size())
		return &mFileViews[mFocusEditViewIndex].mFileView;
	    else
		return nullptr;
	    }
	// For signal handlers
	void setFocusEditTextView(GtkTextView *editTextView);
	bool handleKeyPress(GdkEvent *event);
	void drawMargin(GtkWidget *widget, cairo_t *cr);

    private:
	GtkNotebook *mHeaderBook;
	GtkNotebook *mSourceBook;
	struct ScrolledFileView
	    {
	    ScrolledFileView():
		mDesiredLine(-1)
		{}
	    std::string mFilename;
	    GtkScrolledWindow *mScrolled;
	    FileEditView mFileView;
	    int mDesiredLine;
	    };
	std::vector<ScrolledFileView> mFileViews;
	size_t mFocusEditViewIndex;
	GdkCursor *mDebuggerCurrentLocationCursor;
	GdkCursor *mDebuggerBreakpointCursor;
	void addFile(char const * const fn, bool useMainView, int lineNum);
	void idleHighlight()
	    {
	    if(getEditView())
		getEditView()->idleHighlight();
	    }
	ScrolledFileView *getScrolledFileView()
	    {
	    if(mFocusEditViewIndex < mFileViews.size())
		return &mFileViews[mFocusEditViewIndex];
	    else
		return nullptr;
	    }
    };



#endif /* EDIT_FILES_H_ */
