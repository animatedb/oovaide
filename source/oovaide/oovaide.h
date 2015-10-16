/*
 * oovaide.h
 *
 *  Created on: Dec 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVAIDE_H_
#define OOVAIDE_H_

#include <gtk/gtk.h>
#include "Builder.h"
#include "OovProject.h"
#include "Contexts.h"
#include "OovError.h"
#include "GlobalSettings.h"
#include <atomic>

// The output status window works in two modes.
// 1. Normally it will stop at the first error that appears. If no error
//    appears, it keeps displayed the most recent information.
// 2. This mode always displayed the most recent information even
//    if there is an error.
// In either mode if the cursor is not at the end, it will not be moved
// by the program. This allows the cursor to be moved by the user.
// The user can transition between modes by pressing either the move to
// top error button, or the move to bottom buffer button.
class WindowBuildListener:public OovProcessListener
    {
    public:
        WindowBuildListener():
            mStatusTextView(nullptr), mComplete(false), mErrorMode(EM_MoveTopError)
            {}
        virtual ~WindowBuildListener();
        void initListener(Builder &builder);
        GtkTextBuffer *getBuffer()
            { return GuiTextBuffer::getBuffer(mStatusTextView); }
        void clearStatusTextView(Builder &builder)
            { initListener(builder); }
        virtual void onStdOut(OovStringRef const out, size_t len) override
            {
            LockGuard guard(mMutex);
            mStdOutAndErr.append(out, len);
            }
        virtual void onStdErr(OovStringRef const out, size_t len) override
            {
            LockGuard guard(mMutex);
            mStdOutAndErr.append(out, len);
            }
        virtual void processComplete() override;
        /// This is called from the GUI thread.
        /// @return true if something was done.
        bool onBackgroundProcessIdle(bool &complete);
        void moveTopError();
        void moveUpError();
        void moveDownError();
        void moveBottomBuffer();

    private:
        GtkTextView *mStatusTextView;
        std::string mStdOutAndErr;
        InProcMutex mMutex;
        GuiHighlightTag mErrHighlightTag;
        bool mComplete;
        enum eErrorModes { EM_MoveTopError, EM_MoveEndBuffer };
        eErrorModes mErrorMode;

        void searchForErrorFromIter(GtkTextIter iter, bool forward);
    };


/// The virtual functions in this class may be called from a background thread.
/// The idle function should be called from the GUI thread.
class WindowProjectStatusListener: public OovTaskStatusListener
    {
    public:
        WindowProjectStatusListener():
            mProgressIteration(0), mState(TS_Stopped)
            {}
        ~WindowProjectStatusListener();
        virtual OovTaskStatusListenerId startTask(OovStringRef const &text, size_t i) override;
        /// @return true to keep going, false to stop iteration.
        virtual bool updateProgressIteration(OovTaskStatusListenerId id,
                size_t i, OovStringRef const &text) override;
        // Set a flag so the GUI idle can close the dialog.
        virtual void endTask(OovTaskStatusListenerId id) override
            { mState = TS_Stopping; }
        // Call this from the onIdle.
        void idleUpdateProgress();

    private:
        int mProgressIteration;
        std::mutex mMutex;
        OovString mUpdateText;
        enum eTaskStates { TS_Stopped, TS_Running, TS_Stopping };
        std::atomic<eTaskStates> mState;
        TaskBusyDialog mBackDlg;
    };

class oovGui:public OovErrorListener, private GlobalSettingsListener
    {
    friend class Menu;
    public:
        oovGui(Builder &builder):
            mBuilder(builder), mContexts(mProject)
            {}
        ~oovGui();
        void init();
        static gboolean onIdle(gpointer data)
            {
            oovGui *gui = reinterpret_cast<oovGui*>(data);
            return gui->onBackgroundIdle(data);
            }
        void runSrcManager(OovStringRef const buildConfigName,
                OovProject::eSrcManagerOptions smo);
        void stopSrcManager()
            { mProject.stopSrcManager(); }

        void updateGuiForAnalysis();
        void updateGuiForProjectChange();

        void updateMenuEnables(ProjectStatus const &projStat);

        void newProject();
        void openProject();
        void newModule();
        void openDrawing();
        void saveDrawing();
        void saveDrawingAs();
        void exportDrawingAs();

        void makeComplexityFile();
        void makeMemberUseFile();
        void makeMethodUseFile();
        void makeDuplicatesFile();
        void displayProjectStats();
        void makeLineStats();
        void makeCmake();
        void showProjectSettingsDialog();

        void statusTopError()
            { mWindowBuildListener.moveTopError(); }
        void statusUpError()
            { mWindowBuildListener.moveUpError(); }
        void statusDownError()
            { mWindowBuildListener.moveDownError(); }
        void statusBottomError()
            { mWindowBuildListener.moveBottomBuffer(); }

        // true = ok to exit
        bool okToExit()
            { return mContexts.okToExit(); }
        void cppArgOptionsChangedUpdateDrawings()
            { mContexts.cppArgOptionsChangedUpdateDrawings(); }
        Builder &getBuilder()
            { return mBuilder; }
        OovProject &getProject()
            { return mProject; }
        virtual void errorListener(OovStringRef str, OovErrorTypes et) override;
        virtual void loadProject(OovStringRef projDir) override
            { openProject(projDir); }
        // fn is only filled if a fn, colons, and line number are found.
        int getStatusSourceFile(std::string &fn);

    private:
        Builder &mBuilder;
        OovProject mProject;
        Contexts mContexts;
        ProjectStatus mLastProjectStatus;
        WindowBuildListener mWindowBuildListener;
        WindowProjectStatusListener mProjectStatusListener;
        gboolean onBackgroundIdle(gpointer data);

        void openProject(OovStringRef projDir);
        OovStatusReturn loadFile(File &drawFile)
            { return mContexts.loadFile(drawFile); }
        OovStatusReturn saveFile(File &drawFile)
            { return mContexts.saveFile(drawFile); }
        OovStatusReturn exportFile(File &svgFile)
            { return mContexts.exportFile(svgFile); }
        std::string getDiagramName(OovStringRef ext) const;
        void setDiagramName(OovStringRef name);
        ProjectStatus const &getLastProjectStatus() const
            { return mLastProjectStatus; }
        void clearAnalysis();
        bool canStartAnalysis();
    };

#endif /* OOVAIDE_H_ */
