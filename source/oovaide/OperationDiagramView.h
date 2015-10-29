/*
 * OperationDiagramView.h
 *
 *  Created on: Jun 19, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONDIAGRAMVIEW_H_
#define OPERATIONDIAGRAMVIEW_H_

#include "OperationDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include <gtk/gtk.h>

class OperationDiagramListener
    {
    public:
        virtual ~OperationDiagramListener();
        virtual void gotoClass(OovStringRef const className) = 0;
        virtual void gotoOperation(OovStringRef const className,
                OovStringRef const operName, bool isConst) = 0;
    };


class OperationDiagramView
    {
    public:
        OperationDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions), mListener(nullptr),
            mNullDrawer(mOperationDiagram)
            {
            setCairoContext();
            }

        void initialize(const ModelData &modelData,
            OperationDiagramListener *listener)
            {
            mListener = listener;
            mOperationDiagram.initialize(modelData);
            }

        void clearGraphAndAddOperation(OovStringRef const className,
                OovStringRef const opName, bool isConst)
            {
            mOperationDiagram.clearGraphAndAddOperation(className, opName, isConst);
            requestRedraw();
            }
        void restart()
            {
            mOperationDiagram.restart();
            requestRedraw();
            }

        // For use by extern functions.
        void graphButtonPressEvent(const GdkEventButton *event);
        void graphButtonReleaseEvent(const GdkEventButton *event);
        OperationDiagram &getDiagram()
            { return mOperationDiagram; }

        void drawToDrawingArea();
        OovStatusReturn drawSvgDiagram(File &file);
        void gotoClass(OovStringRef const className);
        void gotoOperation(OperationCall const *oper);
        bool isModified() const
            { return mOperationDiagram.isModified(); }
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }
        void viewSource(OovStringRef const module,
                unsigned int lineNum);
        void setFontSize(int size)
            {
            mOperationDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            }
        int getFontSize()
            { return mOperationDiagram.getDiagramBaseFontSize(); }

    private:
        GuiOptions const &mGuiOptions;
        OperationDiagram mOperationDiagram;
        OperationDiagramListener *mListener;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            mNullDrawer.setCurrentDrawingFontSize(mOperationDiagram.getDiagramBaseFontSize());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
