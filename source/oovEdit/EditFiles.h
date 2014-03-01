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
#include <sys/time.h>
#include <vector>


struct ScrolledFileView
    {
    ScrolledFileView():
	mDesiredLine(-1)
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
    int mPageIndex;
    };

class EditFiles
    {
    public:
	EditFiles(Debugger &debugger);
	void init(Builder &builder);
	void onIdle();
	void updateDebugMenu();
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
	void drawLeftMargin(GtkWidget *widget, cairo_t *cr, int &width, int &pixPerChar);
	void drawRightMargin(GtkWidget *widget, cairo_t *cr, int leftMargin, int pixPerChar);
	Debugger &getDebugger()
	    { return mDebugger; }
	void removeNotebookPage(GtkWidget *pageWidget);
	bool checkDebugger();

    private:
	GtkNotebook *mHeaderBook;
	GtkNotebook *mSourceBook;
	Builder *mBuilder;
	std::vector<ScrolledFileView> mFileViews;
	size_t mFocusEditViewIndex;
	Debugger &mDebugger;
	timeval mLastHightlightIdleUpdate;
	void addFile(char const * const fn, bool useMainView, int lineNum);
	void idleHighlight();
	ScrolledFileView *getScrolledFileView()
	    {
	    if(mFocusEditViewIndex < mFileViews.size())
		return &mFileViews[mFocusEditViewIndex];
	    else
		return nullptr;
	    }
    };



#endif /* EDIT_FILES_H_ */
