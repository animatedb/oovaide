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
#include "OovProcess.h"

class Editor:public DebuggerListener
    {
    public:
	Editor();
	void init();
	void loadSettings();
	void saveSettings();
	static gboolean onIdle(gpointer data);
	void openTextFile(OovStringRef const fn);
	void openTextFile();
	bool saveTextFile();
	bool saveAsTextFileWithDialog();
	void findDialog();
	void findAgain(bool forward);
        void findInFilesDialog();
        void findInFiles(char const * const srchStr, char const * const path,
        	bool caseSensitive, GtkTextView *view);
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
	void gotoToken(eFindTokenTypes ft)
	    {
	    if(mEditFiles.getEditView())
		{
		std::string fn;
		int offset;
		if(mEditFiles.getEditView()->find(ft, fn, offset))
		    {
		    mEditFiles.viewFile(fn, offset);
		    }
		}
	    }
	void gotoDeclaration()
	    {
	    gotoToken(FT_FindDecl);
	    }
	void gotoDefinition()
	    {
	    gotoToken(FT_FindDef);
	    }
	Builder &getBuilder()
	    { return mBuilder; }
	Debugger &getDebugger()
	    { return mDebugger; }
	EditFiles &getEditFiles()
	    { return mEditFiles; }
	void setProjectDir(OovStringRef const projDir)
	    { mProjectDir = projDir; }
	void gotoLine(int lineNum)
	    { mEditFiles.gotoLine(lineNum); }
	void gotoLineDialog();
	void gotoFileLine(std::string const &lineBuf);
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
/*
	void drawHighlight()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->drawHighlight();
	    }
*/
	void editPreferences();
	void setPreferencesWorkingDir();
	void idleDebugStatusChange(Debugger::eChangeStatus st);
	virtual void DebugOutput(OovStringRef const str)
	    {
	    mDebugOut.append(str);
	    }
	virtual void DebugStatusChanged()
	    {
	    }
	void debugSetStackFrame(OovStringRef const frameLine);

    private:
	Builder mBuilder;
	EditFiles mEditFiles;
	Debugger mDebugger;
	OovString mLastSearch;
	OovString mProjectDir;
	bool mLastSearchCaseSensitive;
	OovString mDebugOut;
	EditOptions mEditOptions;
	GuiTree mVarView;
	void find(OovStringRef const findStr, bool forward, bool caseSensitive);
	void findAndReplace(OovStringRef const findStr, bool forward, bool caseSensitive,
		OovStringRef const replaceStr);
};


#endif /* OOVEDIT_H_ */
