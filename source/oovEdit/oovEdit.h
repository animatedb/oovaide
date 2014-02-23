/*
 * oovEdit.h
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVEDIT_H_
#define OOVEDIT_H_

#include "EditFiles.h"
#include "EditOptions.h"

class Editor:public DebuggerListener
    {
    public:
	Editor();
	void init();
	void loadSettings();
	void saveSettings();
	static gboolean onIdle(gpointer data);
	void openTextFile(char const * const fn);
	void openTextFile();
	bool saveTextFile();
	bool saveAsTextFileWithDialog();
	void findDialog();
	void findUsingDialogInfo();
	void findAgain(bool forward);
//	void setTabs(int numSpaces);
	void setStyle();
	void cut()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->cut();
	    }
	void copy()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->copy();
	    }
	void paste()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->paste();
	    }
	void deleteSel()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->deleteSel();
	    }
	void undo()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->undo();
	    }
	void redo()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->redo();
	    }
	bool checkExitSave()
	    {
	    return mEditFiles.checkExitSave();
	    }
	Builder &getBuilder()
	    { return mBuilder; }
	Debugger &getDebugger()
	    { return mDebugger; }
	EditFiles &getEditFiles()
	    { return mEditFiles; }
	void setProjectDir(char const * const projDir)
	    { mProjectDir = projDir; }
	void gotoLine(int lineNum)
	    { mEditFiles.gotoLine(lineNum); }
	void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
	        gchar *text, gint len)
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->bufferInsertText(textbuffer, location, text, len);
	    }
	void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
	        GtkTextIter *end)
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->bufferDeleteRange(textbuffer, start, end);
	    }
	void drawHighlight()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->drawHighlight();
	    }
	void editPreferences();
	virtual void DebugOutput(char const * const str)
	    {
	    mDebugOut.append(str);
	    }
	virtual void DebugStatusChanged()
	    {
	    if(mDebugger.getChildState() == GCS_GdbChildPaused)
		mDebuggerStoppedLocation = mDebugger.getStoppedLocation();
	    mUpdateDebugMenu = true;
	    }

    private:
	Builder mBuilder;
	EditFiles mEditFiles;
	Debugger mDebugger;
	std::string mLastSearch;
	std::string mProjectDir;
	bool mLastSearchCaseSensitive;
	std::string mDebugOut;
	bool mUpdateDebugMenu;
	EditOptions mEditOptions;
	DebuggerLocation mDebuggerStoppedLocation;
	void find(char const * const findStr, bool forward, bool caseSensitive);
	void setModuleName(const char *mn)
	    {
	    gtk_window_set_title(GTK_WINDOW(mBuilder.getWidget("MainWindow")), mn);
	    }
};


#endif /* OOVEDIT_H_ */
