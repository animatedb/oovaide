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
            mGuiOptions(guiOptions), mNullDrawer(mPortionDiagram)
            {
            setCairoContext();
            }

        void initialize(const ModelData &modelData)
            {
            mPortionDiagram.initialize(modelData);
            }

        void clearGraphAndAddClass(OovStringRef const className)
            {
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
        OovStatusReturn drawSvgDiagram(File &file);
        OovStatusReturn saveDiagram(File &file)
            { return mPortionDiagram.saveDiagram(file); }
        OovStatusReturn loadDiagram(File &file)
            {
            return mPortionDiagram.loadDiagram(file, mNullDrawer);
            }
        void gotoClass(OovStringRef const className);
        bool isModified() const
            { return mPortionDiagram.isModified(); }
        void viewClassSource();
        void setFontSize(int size)
            {
            mPortionDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            }
        int getFontSize()
            { return mPortionDiagram.getDiagramBaseFontSize(); }

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
            mNullDrawer.setCurrentDrawingFontSize(mPortionDiagram.getDiagramBaseFontSize());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
