/*
 * PortionDiagramView.h
 *
 *  Created on: Jun 19, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTIONDIAGRAMVIEW_H_
#define PORTIONDIAGRAMVIEW_H_

#include "PortionDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include <gtk/gtk.h>


class PortionDiagramView
    {
    public:
        PortionDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions)
            {}

        void initialize(const ModelData &modelData)
            {
            mPortionDiagram.initialize(modelData);
            }

        void clearGraphAndAddClass(OovStringRef const className)
            {
            setCairoContext();
            mPortionDiagram.clearGraphAndAddClass(mNullDrawer, className);
            requestRedraw();
            }

        // For use by extern functions.
        void graphButtonPressEvent(const GdkEventButton *event);
        void graphButtonReleaseEvent(const GdkEventButton *event);
        PortionDiagram &getDiagram()
            { return mPortionDiagram; }
        void relayout()
            {
            mPortionDiagram.relayout(mNullDrawer);
            requestRedraw();
            }

        void drawToDrawingArea();
        bool drawSvgDiagram(File &file);
        bool saveDiagram(File &file)
            { return mPortionDiagram.saveDiagram(file); }
        bool loadDiagram(File &file)
            {
            setCairoContext();
            return mPortionDiagram.loadDiagram(file, mNullDrawer);
            }
        void gotoClass(OovStringRef const className);
        bool isModified() const
            { return mPortionDiagram.isModified(); }
        void viewClassSource();
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }

    private:
        GuiOptions const &mGuiOptions;
        PortionDiagram mPortionDiagram;
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
