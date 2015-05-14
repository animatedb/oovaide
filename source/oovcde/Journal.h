/*
 * Journal.h
 *
 *  Created on: Aug 22, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef JOURNAL_H_
#define JOURNAL_H_

#include <vector>
#include "OovString.h"
#include "ClassDiagram.h"
#include "OperationDiagram.h"
#include "ComponentDiagram.h"
#include "ZoneDiagram.h"
#include "PortionDiagram.h"

enum eRecordTypes { RT_Component, RT_Zone, RT_Class, RT_Portion, RT_Sequence  };

class JournalListener
    {
    public:
	virtual ~JournalListener()
	    {}
	virtual void displayClass(OovStringRef const className) = 0;
	virtual void displayOperation(OovStringRef const className,
		OovStringRef const operName, bool isConst) = 0;
    };

class JournalRecord
    {
    public:
	JournalRecord(eRecordTypes type, JournalListener &listener):
	    mRecordType(type), mListener(listener)
	    {}
	virtual ~JournalRecord()
	    {}
	virtual char const *getRecordTypeName() const = 0;
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) = 0;
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) = 0;
	virtual void drawingAreaDrawEvent() = 0;
	virtual void drawingAreaMotionEvent(const GdkEventMotion * /*event*/)
	    {}
	virtual void drawingLoseFocusEvent()
	    {}
	virtual void cppArgOptionsChangedUpdateDrawings()
	    {}
	virtual void saveFile(FILE *fp) = 0;
	virtual bool isModified() const = 0;
	OovStringRef const getName() const
	    { return mName; }
	// No space is used for building file names.
	std::string getFullName(bool addSpace) const;
	void setName(OovStringRef const str)
	    { mName = str; }
	eRecordTypes getRecordType() const
	    { return mRecordType; }
	void displayClass(OovStringRef const className)
	    { mListener.displayClass(className); }

    private:
	OovString mName;
	eRecordTypes mRecordType;
	JournalListener &mListener;
    };

class JournalRecordClassDiagram:public JournalRecord, public ClassDiagramListener
    {
    public:
	JournalRecordClassDiagram(Builder &builder, const ModelData &model,
		JournalListener &journalListener,
		OovTaskStatusListener &taskStatusListener):
	    JournalRecord(RT_Class, journalListener)
	    {
	    mClassDiagram.initialize(builder, model, this, &taskStatusListener);
	    }
	ClassDiagram mClassDiagram;

    private:
	virtual char const *getRecordTypeName() const override
	    { return "Class"; }
	virtual void gotoClass(OovStringRef const className) override
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
	    { mClassDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
	    { mClassDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent() override
	    { mClassDiagram.drawToDrawingArea(); }
	virtual void cppArgOptionsChangedUpdateDrawings() override
	    { mClassDiagram.updateGraph(); }
	virtual void saveFile(FILE *fp) override
	    { mClassDiagram.drawSvgDiagram(fp); }
	virtual bool isModified() const override
	    { return mClassDiagram.getClassGraph().isModified(); }
    };

class JournalRecordOperationDiagram:public JournalRecord, public OperationDiagramListener
    {
    public:
	JournalRecordOperationDiagram(Builder &builder, const ModelData &model,
		JournalListener &listener):
	    JournalRecord(RT_Sequence, listener)
	    {
	    mOperationDiagram.initialize(builder, model, this);
	    }
	OperationDiagram mOperationDiagram;

    private:
	virtual char const *getRecordTypeName() const override
	    { return "Seq"; }
	virtual void gotoClass(OovStringRef const className) override
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
	    { mOperationDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
	    { mOperationDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent() override
	    { mOperationDiagram.drawToDrawingArea(); }
	virtual void saveFile(FILE *fp) override
	    { mOperationDiagram.drawSvgDiagram(fp); }
	virtual bool isModified() const override
	    { return mOperationDiagram.getOpGraph().isModified(); }
    };

class JournalRecordComponentDiagram:public JournalRecord
    {
    public:
	JournalRecordComponentDiagram(Builder &builder, JournalListener &listener):
	    JournalRecord(RT_Component, listener)
	    {
	    mComponentDiagram.initialize(builder);
	    }
	ComponentDiagram mComponentDiagram;

    private:
	virtual char const *getRecordTypeName() const override
	    { return "Comp"; }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
	    { mComponentDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
	    { mComponentDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent() override
	    { mComponentDiagram.drawToDrawingArea(); }
	virtual void saveFile(FILE *fp) override
	    { mComponentDiagram.drawSvgDiagram(fp); }
	// Indicate it is always modified so the single diagram is kept around.
	virtual bool isModified() const override
	    { return true; }
//	    { return mComponentDiagram.getGraph().isModified(); }
    };

class JournalRecordZoneDiagram:public JournalRecord, public ZoneDiagramListener
    {
    public:
	JournalRecordZoneDiagram(Builder & /*builder*/, const ModelData &model,
		JournalListener &listener):
	    JournalRecord(RT_Zone, listener)
	    {
	    mZoneDiagram.initialize(model, this);
	    }
	ZoneDiagram mZoneDiagram;

    private:
	virtual char const *getRecordTypeName() const override
	    { return "Zone"; }
	virtual void gotoClass(OovStringRef const className) override
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
	    { mZoneDiagram.graphButtonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
	    { mZoneDiagram.graphButtonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent() override
	    { mZoneDiagram.drawToDrawingArea(); }
	virtual void drawingAreaMotionEvent(const GdkEventMotion *event) override
	    {
            mZoneDiagram.handleDrawingAreaMotion(
                static_cast<int>(event->x), static_cast<int>(event->y));
            }
	virtual void drawingLoseFocusEvent() override
	    { mZoneDiagram.handleDrawingAreaLoseFocus(); }
	virtual void saveFile(FILE *fp) override
	    { mZoneDiagram.drawSvgDiagram(fp); }
	// Indicate it is always modified so the single diagram is kept around.
	virtual bool isModified() const override
	    { return true; }
//	    { return mZoneDiagram.getZoneGraph().isModified(); }
    };

class JournalRecordPortionDiagram:public JournalRecord
    {
    public:
	JournalRecordPortionDiagram(Builder & /*builder*/, const ModelData &model,
		JournalListener &listener):
	    JournalRecord(RT_Portion, listener)
	    {
	    mPortionDiagram.initialize(model);
	    }
	PortionDiagram mPortionDiagram;

    private:
	virtual char const *getRecordTypeName() const override
	    { return "Portion"; }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
	    { mPortionDiagram.graphButtonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
	    { mPortionDiagram.graphButtonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent() override
	    { mPortionDiagram.drawToDrawingArea(); }
	virtual void saveFile(FILE *fp) override
	    { mPortionDiagram.drawSvgDiagram(fp); }
	// Indicate it is always modified so the single diagram is kept around.
	virtual bool isModified() const override
	    { return true; }
//	    { return mZoneDiagram.getZoneGraph().isModified(); }
    };
/// This class provides a non-volatile history of diagrams.
class Journal
    {
    public:
	Journal();
	virtual ~Journal()
	    {}
	static Journal *getJournal();
	void init(Builder &builder, const ModelData &model,
		JournalListener &journalListener,
		OovTaskStatusListener &taskStatusListener)
	    {
	    mBuilder = &builder;
	    mModel = &model;
	    mJournalListener = &journalListener;
	    mTaskStatusListener = &taskStatusListener;
	    }
	void stopAndWaitForBackgroundComplete()
	    {
	    /// This deletes all records, which should clean up all background
	    /// threads as long as the destructors are correct.
	    clear();
	    }
	void clear();
	void displayClass(OovStringRef const className);
	void addClass(OovStringRef const className);
	void displayOperation(OovStringRef const className, OovStringRef const operName,
		bool isConst);
	void displayComponents();
	void displayWorldZone();
	void displayPortion(OovStringRef const className);
	void saveFile(FILE *fp);
	void cppArgOptionsChangedUpdateDrawings();
	void setCurrentRecord(size_t index)
	    {
	    if(index < mRecords.size())
		mCurrentRecord = index;
	    }
	const std::vector<JournalRecord*> getRecords() const
	    { return mRecords; }
	// For global function access.
	JournalRecord *getCurrentRecord()
	    { return (mRecords.size() > 0) ? mRecords[mCurrentRecord] : NULL; }
	void removeUnmodifiedRecords();
	ZoneDiagram *getCurrentZoneDiagram()
	    {
	    JournalRecord *rec = getCurrentRecord();
	    ZoneDiagram *zoneDiagram = nullptr;
	    if(rec && rec->getRecordType() == RT_Zone)
		{
		JournalRecordZoneDiagram *zoneRecord =
			static_cast<JournalRecordZoneDiagram *>(rec);
		if(zoneRecord)
		    zoneDiagram = &zoneRecord->mZoneDiagram;
		}
	    return zoneDiagram;
	    }

    private:
	std::vector<JournalRecord*> mRecords;
	size_t mCurrentRecord;
	Builder *mBuilder;
	const ModelData *mModel;
	JournalListener *mJournalListener;
	OovTaskStatusListener *mTaskStatusListener;
	const JournalRecord *getCurrentRecord() const
	    { return (mRecords.size() > 0) ? mRecords[mRecords.size()-1] : NULL; }
	void addRecord(JournalRecord *record,	OovStringRef const name);
	void removeRecord(int index);
	int findRecord(eRecordTypes rt, OovStringRef const name);
    };

#endif /* JOURNAL_H_ */
