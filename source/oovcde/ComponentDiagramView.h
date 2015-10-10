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
            mGuiOptions(guiOptions)
            {}
        void initialize(IncDirDependencyMapReader const &incMap)
            {
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
            NullDrawer nulDrawer(mCairoContext.getCairo());
            mComponentDiagram.relayout(nulDrawer);
            requestRedraw();
            }
        bool isModified() const
            { return true; }
        ComponentDrawOptions &getDrawOptions()
            { return mDrawOptions; }
// DEAD CODE
//        GuiOptions const &getGuiOptions()
//            { return mGuiOptions; }

    private:
        GuiOptions const &mGuiOptions;
        ComponentDiagram mComponentDiagram;
        ComponentDrawOptions mDrawOptions;
        /// Used to calculate font sizes.
        GtkCairoContext mCairoContext;
        void setCairoContext()
            {
            mCairoContext.setContext(getDiagramWidget());
            }
        GtkWidget *getDiagramWidget()
            { return Builder::getBuilder()->getWidget("DiagramDrawingarea"); }
        void updateDrawingAreaSize();
        void displayContextMenu(guint button, guint32 acttime, gpointer data);
        void requestRedraw()
            { gtk_widget_queue_draw(getDiagramWidget()); }
    };


#endif /* CLASSDIAGRAMVIEW_H_ */
