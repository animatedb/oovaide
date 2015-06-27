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
    };


class OperationDiagramView
    {
    public:
        OperationDiagramView():
            mListener(nullptr)
            {}

        void initialize(const ModelData &modelData,
            OperationDiagramListener *listener)
            {
            mListener = listener;
            mOperationDiagram.initialize(modelData);
            }

        void clearGraphAndAddOperation(OovStringRef const className,
                OovStringRef const opName, bool isConst)
            {
            setCairoContext();
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
        void drawSvgDiagram(FILE *fp);
        void gotoClass(OovStringRef const className);
        bool isModified() const
            { return mOperationDiagram.isModified(); }
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }

    private:
        OperationDiagram mOperationDiagram;
        OperationDiagramListener *mListener;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
