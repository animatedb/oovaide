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
#include <gtk/gtk.h>

class ComponentDiagramView
    {
    public:
        ComponentDiagramView()
            {}
        void initialize(IncDirDependencyMapReader const &incMap)
            {
            mComponentDiagram.initialize(incMap);
            restart();
            }

        // For use by extern functions.
        void buttonPressEvent(const GdkEventButton *event);
        void buttonReleaseEvent(const GdkEventButton *event);
        ComponentDiagram &getDiagram()
            { return mComponentDiagram; }

        void drawToDrawingArea();
        void drawSvgDiagram(FILE *fp);
        void restart();
        void relayout()
            {
            NullDrawer nulDrawer(mCairoContext.getCairo());
            mComponentDiagram.relayout(nulDrawer);
            requestRedraw();
            }
        bool isModified() const
            { return true; }

    private:
        ComponentDiagram mComponentDiagram;
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
