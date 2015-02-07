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
#include "OovProcess.h"
#include "ModelObjects.h"
#include "Journal.h"

class WindowBuildListener:public OovProcessListener
    {
    public:
	WindowBuildListener():
	    mOpenProject(false), mProcessComplete(false),
	    mStatusTextView(nullptr)
	    {}
	void initListener(Builder &builder)
	    {
	    // "DrawingStatusPaned"
	    mStatusTextView = GTK_TEXT_VIEW(builder.getWidget("StatusTextview"));
	    Gui::clear(mStatusTextView);
	    }
	void clearListener(Builder &builder)
	    { initListener(builder); }
	virtual ~WindowBuildListener()
	    {}
	virtual void onStdOut(OovStringRef const out, int len)
	    {
// Adding this guard causes a hang in linux when children have tons of errors.
	    LockGuard guard(mMutex);
	    mStdOutAndErr.append(out, len);
	    }
	virtual void onStdErr(OovStringRef const out, int len)
	    {
	    LockGuard guard(mMutex);
	    mStdOutAndErr.append(out, len);
	    }
	virtual void processComplete()
	    {
	    mProcessComplete = true;
	    }
	// This is called from the GUI thread.
	bool onBackgroundProcessIdle(bool &completed);

	protected:
	    bool mOpenProject;
	    bool mProcessComplete;

	private:
	    GtkTextView *mStatusTextView;
	    std::string mStdOutAndErr;
	    InProcMutex mMutex;
    };

class Menu
    {
    public:
	Menu(class oovGui &gui):
	    mGui(gui), mBuildIdle(true), mProjectOpen(false), mInit(true)
	    {}
	void updateMenuEnables();

    private:
	class oovGui &mGui;
	bool mBuildIdle;
	bool mProjectOpen;
	bool mInit;
    };

#define LAZY_UPDATE 0

class oovGui:public JournalListener, public WindowBuildListener
    {
    friend class Menu;
    public:
	oovGui():
	    mProjectOpen(false), mMenu(*this), mBackgroundProc(*this)
	    {}
	~oovGui()
	    { clear(); }
	void init();
	void clear();
	static gboolean onIdle(gpointer data);
	void openProject();
	enum eSrcManagerOptions { SM_Analyze, SM_Build, SM_CovInstr, SM_CovBuild, SM_CovStats };
	bool runSrcManager(OovStringRef const buildConfigName, eSrcManagerOptions smo);
	void stopSrcManager();
	void updateProject();
	virtual void displayClass(OovStringRef const className);
	virtual void addClass(OovStringRef const className);
	virtual void displayOperation(OovStringRef const className,
		OovStringRef const operName, bool isConst);
	void updateJournalList();
	void updateComponentList()
	    { mComponentList.updateComponentList(); }
	void updateOperationList(const ModelData &modelData, OovStringRef const className);
	void updateClassList(OovStringRef const className);
	void makeComplexityFile();
	void makeMemberUseFile();
	void makeDuplicatesFile();
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
	Journal &getJournal()
	    { return mJournal; }
	void setProjectOpen(bool open)
	    {
	    mProjectOpen = open;
	    mMenu.updateMenuEnables();
	    }
	bool isProjectOpen() const
	    { return mProjectOpen; }
	// fn is only filled if a fn, colons, and line number are found.
	int getStatusSourceFile(std::string &fn);
	GtkWindow *getWindow()
	    {
	    return GTK_WINDOW(getBuilder().getWidget("TopWindow"));
	    }
#if(LAZY_UPDATE)
	void setBackgroundUpdateClassListSize(int size)
	    {
	    mLazyClassListCurrentIndex = 0;
	    mLazyClassListCount = size;
	    }
	bool backgroundUpdateClassListItem()
	    {
	    bool didSomething = mLazyClassListCurrentIndex < mLazyClassListCount;
	    if(didSomething)
		{
		if(mModelData.mTypes[mLazyClassListCurrentIndex]->getObjectType() == otClass)
		    mClassList.appendText(mModelData.mTypes[mLazyClassListCurrentIndex]->getName());
		mLazyClassListCurrentIndex++;
		}
	    return didSomething;
	    }
#endif

    private:
	ClassList mClassList;
	ComponentList mComponentList;
	OperationList mOperationList;
	JournalList mJournalList;
	Builder mBuilder;
	ModelData mModelData;
	Journal mJournal;
	std::string mLastSavedPath;
	bool mProjectOpen;
#if(LAZY_UPDATE)
	int mLazyClassListCurrentIndex;
	int mLazyClassListCount;
#endif
	Menu mMenu;
	OovBackgroundPipeProcess mBackgroundProc;
    };

#endif /* OOVCDE_H_ */
