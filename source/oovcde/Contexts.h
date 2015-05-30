/*
 * Contexts.h
 * Created on: May 14, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef CONTEXTS_H_
#define CONTEXTS_H_

#include "OovProject.h"
#include "Journal.h"
#include "JournalList.h"
#include "ClassList.h"
#include "ComponentList.h"
#include "OperationList.h"

// A context is related to a task that the programmer is performing.
// There is usually a diagram associated with things that affect the diagram.
// It handles the routing of user functions (callbacks) depending on context.
//
// Examples:
//	View components
//		Display component diagram
//		Display component/module selection list
// 	View related classes:
//		Display class diagram
//		Display class selection list
//	View part of a class:
//		Display portion diagram
//		Display class selection list
class Contexts:public JournalListener
    {
    public:
	enum ePageIndices { PI_Component, PI_Zone, PI_Class, PI_Portion, PI_Seq, PI_Journal };

	Contexts(OovProject &proj);
	void init(OovTaskStatusListener &taskStatusListener);
	void stopAndWaitForBackgroundComplete()
	    { mJournal.stopAndWaitForBackgroundComplete(); }
	void clear();

	/// Reads from the journal and updates the GUI journal list.
	void updateJournalList();
	void updateComponentList()
	    { mComponentList.updateComponentList(); }
	void updateOperationList(const ModelData &modelData, OovStringRef const className);
	void updateClassList(OovStringRef const className);
	Journal &getJournal()
	    { return mJournal; }
	ZoneDiagramList &getZoneList()
	    { return mZoneList; }
	ClassList &getClassList()
	    { return mClassList; }
	const JournalRecord *getCurrentJournalRecord()
	    { return mJournal.getCurrentRecord(); }
	void saveFile(FILE *fp)
	    { mJournal.saveFile(fp); }
	void cppArgOptionsChangedUpdateDrawings()
	    { mJournal.cppArgOptionsChangedUpdateDrawings(); }

	// JournalListener callbacks
	virtual void displayClass(OovStringRef const className) override;
	virtual void displayOperation(OovStringRef const className,
		OovStringRef const operName, bool isConst) override;

	// Called from callbacks
	void classTreeViewCursorChanged();
	void operationsTreeviewCursorChanged();
	void journalTreeviewCursorChanged();
	void moduleTreeviewCursorChanged();
	void zoneTreeviewCursorChanged();
	void zoneTreeviewButtonRelease(const GdkEventButton *event);
	void listNotebookSwitchPage(int pageNum);

    private:
	OovProject &mProject;
	ePageIndices mCurrentPage;

	Journal mJournal;
	ClassList mClassList;
	// This is the list of components (directories), not modules (source files).
	ComponentList mComponentList;
	OperationList mOperationList;
	ZoneDiagramList mZoneList;
	JournalList mJournalList;

	void addClass(OovStringRef const className);
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
	    {
	    return mJournalList.getSelectedIndex();
	    }
    };

#endif /* CONTEXTS_H_ */
