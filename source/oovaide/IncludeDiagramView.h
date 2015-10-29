/*
 * IncludeDiagramView.h
 *
 *  Created on: Jun 18, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDEDIAGRAMVIEW_H_
#define INCLUDEDIAGRAMVIEW_H_

#include "IncludeDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include <gtk/gtk.h>

class IncludeDiagramView
    {
    public:
        IncludeDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions), mNullDrawer(mIncludeDiagram)
            {}
        void initialize(IncDirDependencyMapReader const &incMap)
            {
            mIncludeDiagram.initialize(incMap);
            restart();
            }

        void clearGraphAndAddInclude(OovStringRef incName)
            {
            setCairoContext();
            mIncludeDiagram.clearGraphAndAddInclude(mNullDrawer, incName);
            }
        void addSuppliers();

        // For use by extern functions.
        void graphButtonPressEvent(const GdkEventButton *event);
        void graphButtonReleaseEvent(const GdkEventButton *event);
        IncludeDiagram &getDiagram()
            { return mIncludeDiagram; }

        void drawToDrawingArea();
        OovStatusReturn drawSvgDiagram(File &file);
        void restart();
        void relayout()
            {
            mIncludeDiagram.relayout(mNullDrawer);
            requestRedraw();
            }
        bool isModified() const
            { return mIncludeDiagram.isModified(); }
        void viewFileSource();
        void setFontSize(int size)
            {
            mIncludeDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            }
        int getFontSize()
            { return mIncludeDiagram.getDiagramBaseFontSize(); }

    private:
        GuiOptions const &mGuiOptions;
        IncludeDiagram mIncludeDiagram;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            mNullDrawer.setCurrentDrawingFontSize(mIncludeDiagram.getDiagramBaseFontSize());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
        void displayContextMenu(guint button, guint32 acttime, gpointer data);
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
