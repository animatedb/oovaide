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
#include "EditorIpc.h"
#include "ControlWindow.h"


class Editor:public DebuggerListener, public OovErrorListener
    {
    public:
        Editor();
        void init();
        void startStdinListening()
            { mEditorIpc.startStdinListening(); }
        void loadSettings();
        void saveSettings();
        static gboolean onIdleCallback(gpointer data);
        gboolean onIdle(gpointer data);
        void openTextFile(OovStringRef const fn);
        void openTextFile();
        OovStatusReturn saveTextFile();
        OovStatusReturn saveAsTextFileWithDialog();
        void findDialog();
        void findAgain(bool forward);
        void findInFilesDialog();
        void findInFiles(char const * const srchStr, char const * const path,
                bool caseSensitive, bool sourceOnly, GtkTextView *view);
//      void setTabs(int numSpaces);
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
        void analyze()
            { mEditorIpc.analyze(); }
        void build()
            { mEditorIpc.build(); }
        void stopAnalyze()
            { mEditorIpc.stopAnalyze(); }
        bool checkExitSave();
        void gotoToken(eFindTokenTypes ft)
            {
            if(mEditFiles.getEditView())
                {
                mEditFiles.getEditView()->findToken(ft);
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
        void goToMethod()
            {
            OovString className;
            OovString methodName;
            mEditFiles.getEditView()->getMethodNameAtLocation(className, methodName);
            mEditorIpc.goToMethod(className, methodName);
            }
        void viewClassDiagram()
            {
            mEditorIpc.viewClassDiagram(
                    mEditFiles.getEditView()->getClassNameAtLocation());
            }
        void viewPortionDiagram()
            {
            mEditorIpc.viewPortionDiagram(
                    mEditFiles.getEditView()->getClassNameAtLocation());
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
/*
        void drawHighlight()
            {
            if(mEditFiles.getEditView())
                mEditFiles.getEditView()->drawHighlight();
            }
*/
        void editPreferences();
        void setPreferencesWorkingDir();
        void idleDebugStatusChange(eDebuggerChangeStatus st);
        virtual void errorListener(OovStringRef str, OovErrorTypes et) override
            {
            mDebugOut.append(str);
            ControlWindow::showNotebookTab(ControlWindow::CT_Control);
            }
        virtual void DebugOutput(OovStringRef const str) override
            {
            mDebugOut.append(str);
            }
        virtual void DebugStatusChanged() override
            {
            }
        void debugSetStackFrame(OovStringRef const frameLine);
        void displayControlMenu(guint button, guint32 acttime, gpointer data);
        void updateDebugDataValue();
        void removeDebugDataValue();

    private:
        Builder mBuilder;
        EditFiles mEditFiles;
        Debugger mDebugger;
        OovString mLastSearch;
        OovString mProjectDir;
        ProjectReader mProject;
        bool mLastSearchCaseSensitive;
        OovString mDebugOut;
        EditOptions mEditOptions;
        GuiTree mVarView;
        EditorIpc mEditorIpc;

        void find(OovStringRef const findStr, bool forward, bool caseSensitive);
        void findAndReplace(OovStringRef const findStr, bool forward, bool caseSensitive,
                OovStringRef const replaceStr);
};


#endif /* OOVEDIT_H_ */
