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
#include "ClassDiagramView.h"
#include "OperationDiagramView.h"
#include "ComponentDiagramView.h"
#include "IncludeDiagramView.h"
#include "ZoneDiagramView.h"
#include "PortionDiagramView.h"

enum eRecordTypes { RT_Component, RT_Include, RT_Zone, RT_Class, RT_Portion, RT_Sequence  };

class JournalListener
    {
    public:
        virtual ~JournalListener();
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
        virtual ~JournalRecord();
        virtual char const *getRecordTypeName() const = 0;
        virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) = 0;
        virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) = 0;
        virtual void drawingAreaDrawEvent() = 0;
        virtual void drawingAreaMotionEvent(const GdkEventMotion * /*event*/)
            {}
        virtual void drawingLostPointerEvent()
            {}
        virtual void cppArgOptionsChangedUpdateDrawings()
            {}
        virtual void saveFile(FILE *drawFp) = 0;
        virtual void exportFile(FILE *svgFp) = 0;
        virtual void loadFile(FILE *drawFp) = 0;
        virtual bool isModified() const = 0;
        OovStringRef const getName() const
            { return mName; }
        // No space is used for building file names.
        std::string getFullName(bool addSpace) const;
        // Used for file storage name
        OovString getDiagramName() const;
        void setDiagramName(OovStringRef name)
            { mDiagramName = name; }
        void setName(OovStringRef const str)
            { mName = str; }
        eRecordTypes getRecordType() const
            { return mRecordType; }
        void displayClass(OovStringRef const className)
            { mListener.displayClass(className); }

    private:
        OovString mName;
        OovString mDiagramName;
        eRecordTypes mRecordType;
        JournalListener &mListener;
    };

class JournalRecordClassDiagram:public JournalRecord, public ClassDiagramListener
    {
    public:
        JournalRecordClassDiagram(const ModelData &model,
                JournalListener &journalListener,
                OovTaskStatusListener &taskStatusListener):
            JournalRecord(RT_Class, journalListener)
            {
            mClassDiagram.initialize(model, *this, taskStatusListener);
            }
        virtual ~JournalRecordClassDiagram();
        ClassDiagramView mClassDiagram;

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
            { mClassDiagram.updatePositions(); }
        virtual void saveFile(FILE *drawFp) override
            { mClassDiagram.saveDiagram(drawFp); }
        virtual void exportFile(FILE *svgFp) override
            { mClassDiagram.drawSvgDiagram(svgFp); }
        virtual void loadFile(FILE *drawFp) override
            { mClassDiagram.loadDiagram(drawFp); }
        virtual bool isModified() const override
            { return mClassDiagram.isModified(); }
    };

class JournalRecordOperationDiagram:public JournalRecord, public OperationDiagramListener
    {
    public:
        JournalRecordOperationDiagram(const ModelData &model,
                JournalListener &listener):
            JournalRecord(RT_Sequence, listener)
            {
            mOperationDiagram.initialize(model, this);
            }
        virtual ~JournalRecordOperationDiagram();
        OperationDiagramView mOperationDiagram;

    private:
        virtual char const *getRecordTypeName() const override
            { return "Seq"; }
        virtual void gotoClass(OovStringRef const className) override
            { displayClass(className); }
        virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
            { mOperationDiagram.graphButtonPressEvent(event); }
        virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
            { mOperationDiagram.graphButtonReleaseEvent(event); }
        virtual void drawingAreaDrawEvent() override
            { mOperationDiagram.drawToDrawingArea(); }
        virtual void saveFile(FILE *drawFp) override
            { Gui::messageBox("Saving this drawing type is not supported yet, but exporting is."); }
        virtual void exportFile(FILE *svgFp) override
            { mOperationDiagram.drawSvgDiagram(svgFp); }
        virtual void loadFile(FILE *drawFp) override
            {}
        virtual bool isModified() const override
            { return mOperationDiagram.isModified(); }
    };

class JournalRecordComponentDiagram:public JournalRecord
    {
    public:
        JournalRecordComponentDiagram(IncDirDependencyMapReader const &incMap,
            JournalListener &listener):
            JournalRecord(RT_Component, listener)
            {
            mComponentDiagram.initialize(incMap);
            }
        ~JournalRecordComponentDiagram();
        ComponentDiagramView mComponentDiagram;

    private:
        virtual char const *getRecordTypeName() const override
            { return "Comp"; }
        virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
            { mComponentDiagram.buttonPressEvent(event); }
        virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
            { mComponentDiagram.buttonReleaseEvent(event); }
        virtual void drawingAreaDrawEvent() override
            { mComponentDiagram.drawToDrawingArea(); }
        virtual void saveFile(FILE *drawFp) override
            { Gui::messageBox("Saving this drawing type is not supported yet, but exporting is."); }
        virtual void exportFile(FILE *svgFp) override
            { mComponentDiagram.drawSvgDiagram(svgFp); }
        virtual void loadFile(FILE *drawFp) override
            {}
        // Indicate it is always modified so the single diagram is kept around.
        virtual bool isModified() const override
            { return mComponentDiagram.isModified(); }
    };

class JournalRecordZoneDiagram:public JournalRecord, public ZoneDiagramListener
    {
    public:
        JournalRecordZoneDiagram(const ModelData &model,
                JournalListener &listener):
            JournalRecord(RT_Zone, listener)
            {
            mZoneDiagram.initialize(model, *this);
            }
        virtual ~JournalRecordZoneDiagram();
        ZoneDiagramView mZoneDiagram;

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
        virtual void drawingLostPointerEvent() override
            { mZoneDiagram.handleDrawingAreaLostPointer(); }
        virtual void saveFile(FILE *drawFp) override
            { Gui::messageBox("Saving this drawing type is not supported yet, but exporting is."); }
        virtual void exportFile(FILE *svgFp) override
            { mZoneDiagram.drawSvgDiagram(svgFp); }
        virtual void loadFile(FILE *drawFp) override
            {}
        // Indicate it is always modified so the single diagram is kept around.
        virtual bool isModified() const override
            { return true; }
//          { return mZoneDiagram.getZoneGraph().isModified(); }
    };

class JournalRecordPortionDiagram:public JournalRecord
    {
    public:
        JournalRecordPortionDiagram(const ModelData &model,
                JournalListener &listener):
            JournalRecord(RT_Portion, listener)
            {
            mPortionDiagram.initialize(model);
            }
        ~JournalRecordPortionDiagram();
        PortionDiagramView mPortionDiagram;

    private:
        virtual char const *getRecordTypeName() const override
            { return "Portion"; }
        virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
            { mPortionDiagram.graphButtonPressEvent(event); }
        virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
            { mPortionDiagram.graphButtonReleaseEvent(event); }
        virtual void drawingAreaDrawEvent() override
            { mPortionDiagram.drawToDrawingArea(); }
        virtual void saveFile(FILE *drawFp) override
            { mPortionDiagram.saveDiagram(drawFp); }
        virtual void exportFile(FILE *svgFp) override
            { mPortionDiagram.drawSvgDiagram(svgFp); }
        virtual void loadFile(FILE *drawFp) override
            { mPortionDiagram.loadDiagram(drawFp); }
        // Indicate it is always modified so the single diagram is kept around.
        virtual bool isModified() const override
            { return true; }
//          { return mZoneDiagram.getZoneGraph().isModified(); }
    };

class JournalRecordIncludeDiagram:public JournalRecord
    {
    public:
        JournalRecordIncludeDiagram(const IncDirDependencyMapReader &incMap,
                JournalListener &listener):
            JournalRecord(RT_Include, listener)
            {
            mIncludeDiagram.initialize(incMap);
            }
        ~JournalRecordIncludeDiagram();
        IncludeDiagramView mIncludeDiagram;

    private:
        virtual char const *getRecordTypeName() const override
            { return "Include"; }
        virtual void drawingAreaButtonPressEvent(const GdkEventButton *event) override
            { mIncludeDiagram.graphButtonPressEvent(event); }
        virtual void drawingAreaButtonReleaseEvent(const GdkEventButton *event) override
            { mIncludeDiagram.graphButtonReleaseEvent(event); }
        virtual void drawingAreaDrawEvent() override
            { mIncludeDiagram.drawToDrawingArea(); }
        virtual void saveFile(FILE *drawFp) override
            { Gui::messageBox("Saving this drawing type is not supported yet, but exporting is."); }
        virtual void exportFile(FILE *drawFp) override
            { mIncludeDiagram.drawSvgDiagram(drawFp); }
        virtual void loadFile(FILE *drawFp) override
            {}
        // Indicate it is always modified so the single diagram is kept around.
        virtual bool isModified() const override
            { return true; }
//          { return mZoneDiagram.getZoneGraph().isModified(); }
    };

/// This class provides a non-volatile history of diagrams.
class Journal
    {
    public:
        Journal();
        virtual ~Journal();
        static Journal *getJournal();
        void init(Builder &builder, const ModelData &model,
                const IncDirDependencyMapReader &incMap,
                JournalListener &journalListener,
                OovTaskStatusListener &taskStatusListener)
            {
            mBuilder = &builder;
            mModel = &model;
            mIncludeMap = &incMap;
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
        void displayInclude(OovStringRef const incName);
        void loadFile(FILE *drawFp);
        void saveFile(FILE *drawFp);
        void exportFile(FILE *svgFp);
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
        const JournalRecord *getCurrentRecord() const
            { return (mRecords.size() > 0) ? mRecords[mCurrentRecord] : NULL; }
        void removeUnmodifiedRecords();
        ZoneDiagramView *getCurrentZoneDiagram()
            {
            JournalRecord *rec = getCurrentRecord();
            ZoneDiagramView *zoneDiagram = nullptr;
            if(rec && rec->getRecordType() == RT_Zone)
                {
                JournalRecordZoneDiagram *zoneRecord =
                        static_cast<JournalRecordZoneDiagram *>(rec);
                if(zoneRecord)
                    zoneDiagram = &zoneRecord->mZoneDiagram;
                }
            return zoneDiagram;
            }
        ClassDiagramView *getCurrentClassDiagram()
            {
            JournalRecord *rec = getCurrentRecord();
            ClassDiagramView *classDiagram = nullptr;
            if(rec && rec->getRecordType() == RT_Class)
                {
                JournalRecordClassDiagram *classRecord =
                        static_cast<JournalRecordClassDiagram *>(rec);
                if(classRecord)
                    classDiagram = &classRecord->mClassDiagram;
                }
            return classDiagram;
            }

    private:
        std::vector<JournalRecord*> mRecords;
        size_t mCurrentRecord;
        Builder *mBuilder;
        const ModelData *mModel;
        const IncDirDependencyMapReader *mIncludeMap;
        JournalListener *mJournalListener;
        OovTaskStatusListener *mTaskStatusListener;
        void addRecord(JournalRecord *record,   OovStringRef const name);
        JournalRecordClassDiagram *newClassRecord(OovStringRef className);
        JournalRecordPortionDiagram *newPortionRecord(OovStringRef portionName);
        static const size_t NO_INDEX = static_cast<size_t>(-1);
        void removeRecord(size_t index);
        size_t findRecord(eRecordTypes rt, OovStringRef const name);
    };

#endif /* JOURNAL_H_ */
