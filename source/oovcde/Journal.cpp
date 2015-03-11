/*
 * Journal.cpp
 *
 *  Created on: Aug 22, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "Journal.h"

static Journal *gJournal;


std::string JournalRecord::getFullName(bool addSpace) const
    {
    std::string fullName = getRecordTypeName();
    if(addSpace)
	fullName += ' ';
    fullName += getName();
    return fullName;
    }

Journal::Journal():
    mCurrentRecord(0), mBuilder(nullptr), mModel(nullptr),
    mJournalListener(nullptr), mTaskStatusListener(nullptr)
    {
    gJournal = this;
    }

Journal *Journal::getJournal()
    {
    return gJournal;
    }

void Journal::clear()
    {
    for(auto &rec : mRecords)
	{
	delete rec;
	}
    mRecords.clear();
    }

void Journal::displayClass(OovStringRef const className)
    {
    JournalRecordClassDiagram *rec;
    int recordIndex = findRecord(className);
    if(recordIndex == -1)
	{
	rec = new JournalRecordClassDiagram(*mBuilder, *mModel,
		*mJournalListener, *mTaskStatusListener);
	addRecord(rec, className);
	rec->mClassDiagram.clearGraphAndAddClass(className,
		ClassGraph::AN_ClassesAndChildren);
	}
    else
	{
	setCurrentRecord(recordIndex);
	rec = reinterpret_cast<JournalRecordClassDiagram*>(getCurrentRecord());
	rec->mClassDiagram.drawToDrawingArea();
	}
    }

void Journal::addClass(OovStringRef const className)
    {
    JournalRecord *rec = getCurrentRecord();
    if(rec && rec->getRecordType() == RT_Class)
	{
	JournalRecordClassDiagram *recClass =
		reinterpret_cast<JournalRecordClassDiagram*>(rec);
	recClass->mClassDiagram.addClass(className);
	}
    }

void Journal::displayOperation(OovStringRef const className,
	OovStringRef const operName, bool isConst)
    {
    JournalRecordOperationDiagram *rec;
    OovString fullOperName = operName;
    fullOperName += ' ';
    fullOperName += className;
    int recordIndex = findRecord(fullOperName);
    if(recordIndex == -1)
	{
	rec = new JournalRecordOperationDiagram(*mBuilder, *mModel,
		*mJournalListener);
	addRecord(rec, fullOperName);
	rec->mOperationDiagram.clearGraphAndAddOperation(className, operName, isConst);
	}
    else
	{
	setCurrentRecord(recordIndex);
	rec = reinterpret_cast<JournalRecordOperationDiagram*>(getCurrentRecord());
	rec->mOperationDiagram.drawToDrawingArea();
	}
    }

void Journal::displayComponents()
    {
    if(mBuilder)
	{
	OovStringRef const componentName = "<Components>";
	int recordIndex = findRecord(componentName);
	JournalRecordComponentDiagram *rec;
	if(recordIndex == -1)
	    {
	    rec = new JournalRecordComponentDiagram(*mBuilder, *mJournalListener);
	    rec->mComponentDiagram.drawToDrawingArea();
	    addRecord(rec, componentName);
	    }
	else
	    {
	    setCurrentRecord(recordIndex);
	    rec = reinterpret_cast<JournalRecordComponentDiagram*>(getCurrentRecord());
	    rec->mComponentDiagram.drawToDrawingArea();
	    }
	}
    }

void Journal::displayWorldZone()
    {
    if(mBuilder)
	{
	OovStringRef const zoneName = "<WorldZone>";
	int recordIndex = findRecord(zoneName);
	JournalRecordZoneDiagram *rec;
	if(recordIndex == -1)
	    {
	    rec = new JournalRecordZoneDiagram(*mBuilder, *mModel, *mJournalListener);
	    rec->mZoneDiagram.drawToDrawingArea();
	    addRecord(rec, zoneName);
	    rec->mZoneDiagram.clearGraphAndAddWorldZone();
	    }
	else
	    {
	    setCurrentRecord(recordIndex);
	    rec = reinterpret_cast<JournalRecordZoneDiagram*>(getCurrentRecord());
	    rec->mZoneDiagram.drawToDrawingArea();
	    }
	}
    }

void Journal::addRecord(JournalRecord *record, OovStringRef const name)
    {
    record->setName(name);
    removeUnmodifiedRecords();
    mRecords.push_back(record);
    setCurrentRecord(mRecords.size()-1);
    }

int Journal::findRecord(OovStringRef const name)
    {
    int index = -1;
    for(size_t i=0; i<mRecords.size(); i++)
	{
	if(std::string(mRecords[i]->getName()).compare(name) == 0)
	    {
	    index = i;
	    break;
	    }
	}
    return index;
    }

void Journal::removeUnmodifiedRecords()
    {
    for(size_t i=0; i<mRecords.size(); i++)
	{
	if(!mRecords[i]->isModified())
	    {
	    removeRecord(i);
	    break;
	    }
	}
    }

void Journal::removeRecord(int index)
    {
    delete mRecords[index];
    mRecords.erase(mRecords.begin() + index);
    }

void Journal::saveFile(FILE *fp)
    {
    JournalRecord *rec = getCurrentRecord();
    if(rec)
	{
	rec->saveFile(fp);
	}
    }

void Journal::cppArgOptionsChangedUpdateDrawings()
    {
    JournalRecord *rec = getCurrentRecord();
    if(rec)
	rec->cppArgOptionsChangedUpdateDrawings();
    }


extern "C" G_MODULE_EXPORT void on_DiagramDrawingarea_draw(GtkWidget *widget,
	GdkEventExpose *event, gpointer user_data)
    {
    JournalRecord *rec = gJournal->getCurrentRecord();
    if(rec)
	rec->drawingAreaDrawEvent();
    }

extern "C" G_MODULE_EXPORT void on_DiagramDrawingarea_button_press_event(GtkWidget *widget,
	GdkEventExpose *event, gpointer user_data)
    {
    JournalRecord *rec = gJournal->getCurrentRecord();
    if(rec)
	rec->drawingAreaButtonPressEvent(reinterpret_cast<GdkEventButton*>(event));
    }

extern "C" G_MODULE_EXPORT void on_DiagramDrawingarea_button_release_event(GtkWidget *widget,
	GdkEventExpose *event, gpointer user_data)
    {
    JournalRecord *rec = gJournal->getCurrentRecord();
    if(rec)
	rec->drawingAreaButtonReleaseEvent(reinterpret_cast<GdkEventButton*>(event));
    }

