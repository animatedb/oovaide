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
#include "ClassList.h"
#include "ComponentList.h"
#include "OperationList.h"
#include "JournalList.h"
#include "Journal.h"
#include "OovProject.h"
#include <atomic>

class WindowBuildListener:public OovProcessListener
    {
    public:
	WindowBuildListener():
	    mStatusTextView(nullptr), mComplete(false)
	    {}
	void initListener(Builder &builder);
	GtkTextBuffer *getBuffer()
	    { return GuiTextBuffer::getBuffer(mStatusTextView); }
	void clearStatusTextView(Builder &builder)
	    { initListener(builder); }
	virtual ~WindowBuildListener()
	    {}
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
	BackgroundDialog mBackDlg;
    };

class oovGui:public JournalListener
    {
    friend class Menu;
    public:
	void init();
	~oovGui();
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

	void addClass(OovStringRef const className);
	virtual void displayClass(OovStringRef const className) override;
	virtual void displayOperation(OovStringRef const className,
		OovStringRef const operName, bool isConst) override;

	void updateGuiForAnalysis();
	void updateGuiForProjectChange();

	void updateMenuEnables(ProjectStatus const &projStat);
	/// Reads from the journal and updates the GUI journal list.
	void updateJournalList();
	void updateComponentList()
	    { mComponentList.updateComponentList(); }
	void updateOperationList(const ModelData &modelData, OovStringRef const className);
	void updateClassList(OovStringRef const className);

	void makeComplexityFile();
	void makeMemberUseFile();
	void makeDuplicatesFile();
	void displayProjectStats();
	void makeLineStats();

	void setLastSavedPath(const std::string &fn)
	    { mLastSavedPath = fn; }
	std::string &getLastSavedPath()
	    {
	    if(mLastSavedPath.length() == 0)
		mLastSavedPath = getDefaultDiagramName();
	    return mLastSavedPath;
	    }
	std::string getDefaultDiagramName();
	std::string getSelectedClass() const
	    {
	    return mClassList.getSelected();
	    }
	std::string getSelectedComponent() const
	    {
	    return mComponentList.getSelectedFileName();
	    }
	void clearSelectedComponent()
	    {
	    mComponentList.clearSelection();
	    }
	std::string getSelectedOperation() const
	    {
	    return mOperationList.getSelected();
	    }
	int getSelectedJournalIndex() const
	    { return mJournalList.getSelectedIndex(); }
	Builder &getBuilder()
	    { return mBuilder; }
	OovProject &getProject()
	    { return mProject; }
	Journal &getJournal()
	    { return mJournal; }
	ZoneDiagramList &getZoneList()
	    { return(mZoneList); }
	// fn is only filled if a fn, colons, and line number are found.
	int getStatusSourceFile(std::string &fn);
	GtkWindow *getWindow()
	    {
	    return GTK_WINDOW(getBuilder().getWidget("TopWindow"));
	    }
	ProjectStatus &getLastProjectStatus()
	    { return mLastProjectStatus; }

    private:
	ClassList mClassList;
	// This is the list of components (directories), not modules (source files).
	ComponentList mComponentList;
	OperationList mOperationList;
	ZoneDiagramList mZoneList;
	JournalList mJournalList;
	Builder mBuilder;
	OovProject mProject;
	ProjectStatus mLastProjectStatus;
	Journal mJournal;
	std::string mLastSavedPath;
	WindowBuildListener mWindowBuildListener;
	WindowProjectStatusListener mProjectStatusListener;
	gboolean onBackgroundIdle(gpointer data);
    };

#endif /* OOVCDE_H_ */
