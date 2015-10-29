/*
 * ComponentDiagramView.h
 *
 *  Created on: Jun 18, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTDIAGRAMVIEW_H_
#define COMPONENTDIAGRAMVIEW_H_

#include "ComponentDiagram.h"
#include "Builder.h"
#include "CairoDrawer.h"
#include "Options.h"

class ComponentDiagramView
    {
    public:
        ComponentDiagramView(GuiOptions const &guiOptions):
            mGuiOptions(guiOptions), mNullDrawer(mComponentDiagram)
            {
            setCairoContext();
            }
        void initialize(IncDirDependencyMapReader const &incMap)
            {
            setCairoContext();
            mDrawOptions.drawImplicitRelations =
                    mGuiOptions.getValueBool(OptGuiShowCompImplicitRelations);
            mComponentDiagram.initialize(incMap);
            restart();
            }

        // For use by extern functions.
        void buttonPressEvent(const GdkEventButton *event);
        void buttonReleaseEvent(const GdkEventButton *event);
        ComponentDiagram &getDiagram()
            { return mComponentDiagram; }

        void drawToDrawingArea();
        OovStatusReturn drawSvgDiagram(File &file);
        void restart();
        void relayout()
            {
            mComponentDiagram.relayout(mNullDrawer);
            requestRedraw();
            }
        bool isModified() const
            { return true; }
        ComponentDrawOptions &getDrawOptions()
            { return mDrawOptions; }
// DEAD CODE
//        GuiOptions const &getGuiOptions()
//            { return mGuiOptions; }
        void setFontSize(int size)
            {
            mComponentDiagram.setDiagramBaseAndGlobalFontSize(size);
            mNullDrawer.setCurrentDrawingFontSize(size);
            relayout();
            }
        int getFontSize()
            { return mComponentDiagram.getDiagramBaseFontSize(); }

    private:
        GuiOptions const &mGuiOptions;
        ComponentDiagram mComponentDiagram;
        ComponentDrawOptions mDrawOptions;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        NullDrawer mNullDrawer;
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
        void displayContextMenu(guint button, guint32 acttime, gpointer data);
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }
        /// If this is not called from drawToDrawingArea, or other similar
        /// drawing functions, then an assert "! surface->finished" will appear
        /// from cairo.  This doesn't seem to affect other diagrams, so it
        /// appears that since the component diagram is the first page, that
        /// GTK is doing something different?
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            mNullDrawer.setGraphicsLib(mCairoContext.getCairo());
            mNullDrawer.setCurrentDrawingFontSize(mComponentDiagram.getDiagramBaseFontSize());
            }
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
