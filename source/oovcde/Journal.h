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

enum eRecordTypes { RT_Class, RT_Sequence, RT_Component };

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
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) = 0;
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) = 0;
	virtual void drawingAreaDrawEvent() = 0;
	virtual void cppArgOptionsChangedUpdateDrawings() = 0;
	virtual void saveFile(FILE *fp) = 0;
	virtual bool isModified() const = 0;
	OovStringRef const getName() const
	    { return mName; }
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
		JournalListener &listener):
	    JournalRecord(RT_Class, listener)
	    {
	    mClassDiagram.initialize(builder, model, this);
	    }
	ClassDiagram mClassDiagram;

    private:
	virtual void gotoClass(OovStringRef const className)
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event)
	    { mClassDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event)
	    { mClassDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent()
	    { mClassDiagram.drawToDrawingArea(); }
	virtual void cppArgOptionsChangedUpdateDrawings()
	    { mClassDiagram.updateGraph(); }
	virtual void saveFile(FILE *fp)
	    { mClassDiagram.drawSvgDiagram(fp); }
	virtual bool isModified() const
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
	virtual void gotoClass(OovStringRef const className)
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event)
	    { mOperationDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event)
	    { mOperationDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent()
	    { mOperationDiagram.drawToDrawingArea(); }
	virtual void cppArgOptionsChangedUpdateDrawings()
	    {}
	virtual void saveFile(FILE *fp)
	    { mOperationDiagram.drawSvgDiagram(fp); }
	virtual bool isModified() const
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
	virtual void gotoClass(OovStringRef const className)
	    { displayClass(className); }
	virtual void drawingAreaButtonPressEvent(const GdkEventButton *event)
	    { mComponentDiagram.buttonPressEvent(event); }
	virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event)
	    { mComponentDiagram.buttonReleaseEvent(event); }
	virtual void drawingAreaDrawEvent()
	    { mComponentDiagram.drawToDrawingArea(); }
	virtual void cppArgOptionsChangedUpdateDrawings()
	    {}
	virtual void saveFile(FILE *fp)
	    { mComponentDiagram.drawSvgDiagram(fp); }
	virtual bool isModified() const
	    { return mComponentDiagram.getGraph().isModified(); }
    };

/// This class provides a non-volatile history of diagrams.
class Journal
    {
    public:
	Journal();
	virtual ~Journal()
	    {}
	void init(Builder &builder, const ModelData &model, JournalListener &listener)
	    { mBuilder = &builder; mModel = &model; mListener = &listener; }
	void clear();
	void displayClass(OovStringRef const className);
	void addClass(OovStringRef const className);
	void displayOperation(OovStringRef const className, OovStringRef const operName,
		bool isConst);
	void displayComponents();
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

    private:
	std::vector<JournalRecord*> mRecords;
	int mCurrentRecord;
	Builder *mBuilder;
	const ModelData *mModel;
	JournalListener *mListener;
	const JournalRecord *getCurrentRecord() const
	    { return (mRecords.size() > 0) ? mRecords[mRecords.size()-1] : NULL; }
	void addRecord(JournalRecord *record,	OovStringRef const name);
	void removeRecord(int index);
	int findRecord(OovStringRef const name);
    };

#endif /* JOURNAL_H_ */
