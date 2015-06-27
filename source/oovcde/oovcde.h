/*
 * oovcde.h
 *
 *  Created on: Dec 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVCDE_H_
#define OOVCDE_H_

#include <gtk/gtk.h>
#include "Builder.h"
#include "OovProject.h"
#include "Contexts.h"
#include <atomic>

class WindowBuildListener:public OovProcessListener
    {
    public:
        WindowBuildListener():
            mStatusTextView(nullptr), mComplete(false)
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

        private:
            GtkTextView *mStatusTextView;
            std::string mStdOutAndErr;
            InProcMutex mMutex;
            GuiHighlightTag mErrHighlightTag;
            bool mComplete;
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
        virtual OovTaskStatusListenerId startTask(OovStringRef const &text, size_t i) override
            {
            mState = TS_Running;
            std::lock_guard<std::mutex> lock(mMutex);
            mBackDlg.startTask(text, i);
            return 0;
            }
        /// @return true to keep going, false to stop iteration.
        virtual bool updateProgressIteration(OovTaskStatusListenerId id,
                size_t i, OovStringRef const &text) override
            {
            if(text)
                {
                std::lock_guard<std::mutex> lock(mMutex);
                mUpdateText = text;
                }
            mProgressIteration = i;
            return(mState == TS_Running);
            }
        // Set a flag so the GUI idle can close the dialog.
        virtual void endTask(OovTaskStatusListenerId id) override
            { mState = TS_Stopping; }
        // Call this from the onIdle.
        void idleUpdateProgress()
            {
            if(mState == TS_Running)
                {
                OovString text;
                    {
                    std::lock_guard<std::mutex> lock(mMutex);
                    text = mUpdateText;
                    }
                if(!mBackDlg.updateProgressIteration(mProgressIteration,
                        text, false))
                    {
                    mState = TS_Stopping;
                    }
                }
            if(mState == TS_Stopping)
                {
                std::lock_guard<std::mutex> lock(mMutex);
                mBackDlg.endTask();
                mState = TS_Stopped;
                }
            }

    private:
        int mProgressIteration;
        std::mutex mMutex;
        OovString mUpdateText;
        enum eTaskStates { TS_Stopped, TS_Running, TS_Stopping };
        std::atomic<eTaskStates> mState;
        TaskBusyDialog mBackDlg;
    };

class oovGui
    {
    friend class Menu;
    public:
        oovGui():
            mContexts(mProject)
            {}
        ~oovGui();
        void init();
        void clearAnalysis();
        bool canStartAnalysis();
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

        void makeComplexityFile();
        void makeMemberUseFile();
        void makeDuplicatesFile();
        void displayProjectStats();
        void makeLineStats();

        void loadFile(FILE *drawFp)
            { mContexts.loadFile(drawFp); }
        void saveFile(FILE *drawFp)
            { mContexts.saveFile(drawFp); }
        void exportFile(FILE *svgFp)
            { mContexts.exportFile(svgFp); }
        void cppArgOptionsChangedUpdateDrawings()
            { mContexts.cppArgOptionsChangedUpdateDrawings(); }
        std::string getDiagramName(OovStringRef ext);
        void setDiagramName(OovStringRef name);
        Builder &getBuilder()
            { return mBuilder; }
        OovProject &getProject()
            { return mProject; }
        // fn is only filled if a fn, colons, and line number are found.
        int getStatusSourceFile(std::string &fn);
        GtkWindow *getWindow()
            {
            return GTK_WINDOW(getBuilder().getWidget("TopWindow"));
            }
        ProjectStatus &getLastProjectStatus()
            { return mLastProjectStatus; }

    private:
        Builder mBuilder;
        OovProject mProject;
        Contexts mContexts;
        ProjectStatus mLastProjectStatus;
        WindowBuildListener mWindowBuildListener;
        WindowProjectStatusListener mProjectStatusListener;
        gboolean onBackgroundIdle(gpointer data);
    };

#endif /* OOVCDE_H_ */
