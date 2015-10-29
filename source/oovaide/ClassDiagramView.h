/*
 * ClassDiagramView.h
 *
 *  Created on: Jun 17, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSDIAGRAMVIEW_H_
#define CLASSDIAGRAMVIEW_H_

#include "ClassDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include "Options.h"
#include "Gui.h"

class ClassDiagramListener
    {
    public:
        virtual ~ClassDiagramListener();
        virtual void gotoClass(OovStringRef const className) = 0;
    };

class ForegroundThreadBusyDialog:public OovTaskStatusListener
    {
    public:
        virtual OovTaskStatusListenerId startTask(OovStringRef const &text,
            size_t totalIterations) override
            {
            mDialog.startTask(text, totalIterations);
            return 0;
            }
        /// @return false to stop iteration.
        virtual bool updateProgressIteration(OovTaskStatusListenerId id, size_t i,
            OovStringRef const &text=nullptr) override
            { return mDialog.updateProgressIteration(i, text, true); }
        /// This is a signal that indicates the task completed.  To end the
        /// task, override updateProcessIteration.
        virtual void endTask(OovTaskStatusListenerId id) override
            { mDialog.endTask(); }

    private:
        TaskBusyDialog mDialog;
    };

class AddClassDialog:public Dialog
    {
    public:
        AddClassDialog();
        void selectNodes(ClassDiagram const &classDiagram, ModelType const *type,
            ClassGraph::eAddNodeTypes addType, std::vector<ClassNode> &nodes);
        ModelType const *getSelectedType() const
            { return mSelectedType; }
        ClassGraph::eAddNodeTypes getSelectedAddType() const
            { return mSelectedAddType; }

    private:
        GuiTree mAddClassTree;
        ModelType const *mSelectedType;
        ClassGraph::eAddNodeTypes mSelectedAddType;
    };

class ClassDiagramView:public ClassGraphListener
    {
    public:
        ClassDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions), mNullDrawer(mClassDiagram), mListener(nullptr)
            {}
        ~ClassDiagramView()
            {}
        void initialize(const ModelData &modelData,
                ClassDiagramListener &listener,
                OovTaskStatusListener &taskStatusListener);

        void clearGraphAndAddClass(OovStringRef const className,
                ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All)
            {
            setCairoContext();
            mClassDiagram.clearGraphAndAddClass(className, addType);
            // It doesn't seem like this should be needed, but even with
            // updateDrawingAreaSize, this doesn't work. It messes up
            // drawing in other non-drawing area widgets, like the
            // class selection list.
            drawToDrawingArea();
            }
        void addClass(OovStringRef const className,
                ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All,
                int depth=ClassDiagram::DEPTH_IMMEDIATE_RELATIONS,
                bool reposition=false)
            {
            setCairoContext();
            mClassDiagram.addClass(className, addType, depth, reposition);
            }
        void buttonPressEvent(const GdkEventButton *event);
        void buttonReleaseEvent(const GdkEventButton *event);
        void drawToDrawingArea();
        OovStatusReturn drawSvgDiagram(File &file);
        OovStatusReturn saveDiagram(File &file)
            { return mClassDiagram.saveDiagram(file); }
        OovStatusReturn loadDiagram(File &file)
            {
            setCairoContext();
            return mClassDiagram.loadDiagram(file);
            }
        bool isModified() const
            { return mClassDiagram.isModified(); }
        void requestRedraw()
            {
            gtk_widget_queue_draw(getDiagramWidget());
            }
        ClassDiagram &getDiagram()
            { return mClassDiagram; }
        // For extern functions
        ClassNode *getNode(int x, int y)
            { return mClassDiagram.getNode(x, y); }
        void updateGraph(bool reposition);
        void gotoClass(OovStringRef const className);
        void displayListContextMenu(guint button, guint32 acttime, gpointer data);
        void displayAddClassDialog();
        void selectAddClass(ModelType const *type, ClassGraph::eAddNodeTypes addType);
        void viewSource(OovStringRef const module, unsigned int lineNum);
        GuiOptions const &getGuiOptions()
            { return mGuiOptions; }
        void setFontSize(int size)
            {
            mClassDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            updateGraph(false);
            }
        int getFontSize()
            { return mClassDiagram.getDiagramBaseFontSize(); }

    private:
        GuiOptions const &mGuiOptions;
        ClassDiagram mClassDiagram;
        /// Used to calculate font sizes. Apparently the setContext call that
        /// calls gdk_cairo_create cannot be called from the background
        /// thread, but the other calls to get text extents can be made from
        /// the background thread.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        ClassDiagramListener *mListener;
        void displayDrawContextMenu(guint button, guint32 acttime, gpointer data);
        void updateDrawingAreaSize();
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            mNullDrawer.setCurrentDrawingFontSize(mClassDiagram.getDiagramBaseFontSize());
            mClassDiagram.setNullDrawer(mNullDrawer);
            }
        // WARNING - This is called from a background thread.
        virtual void doneRepositioning() override
            { requestRedraw(); }
        ForegroundThreadBusyDialog mForegroundBusyDialog;
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
