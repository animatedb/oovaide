/*
 * ComponentDiagramView.cpp
 *
 *  Created on: Jun 18, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentDiagramView.h"
#include "Svg.h"
#include "Options.h"

static GraphPoint gStartPosInfo;
static ComponentDiagramView *gComponentDiagramView;


static const ComponentDrawOptions &getDrawOptions()
    {
    static ComponentDrawOptions dopts;
    dopts.drawImplicitRelations = gGuiOptions.getValueBool(OptGuiShowCompImplicitRelations);
    return dopts;
    }

void ComponentDiagramView::restart()
    {
    setCairoContext();
    NullDrawer nulDrawer(mCairoContext.getCairo());
    mComponentDiagram.restart(getDrawOptions(), nulDrawer);
    requestRedraw();
    }

void ComponentDiagramView::updateDrawingAreaSize()
    {
    GraphSize size = mComponentDiagram.getGraphSize();
    gtk_widget_set_size_request(getDiagramWidget(), size.x, size.y);
    }

void ComponentDiagramView::drawToDrawingArea()
    {
    GtkCairoContext cairo(getDiagramWidget());
    CairoDrawer cairoDrawer(cairo.getCairo());
    cairoDrawer.clearAndSetDefaults();

    mComponentDiagram.drawDiagram(cairoDrawer);
    updateDrawingAreaSize();
    }

void ComponentDiagramView::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(getDiagramWidget());
    SvgDrawer svgDrawer(fp, cairo.getCairo());

    mComponentDiagram.drawDiagram(svgDrawer);
    }

void ComponentDiagramView::buttonPressEvent(const GdkEventButton *event)
    {
    gComponentDiagramView = this;
    gStartPosInfo.set(static_cast<int>(event->x),
        static_cast<int>(event->y));
    }

void ComponentDiagramView::buttonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
        {
        ComponentNode *node = mComponentDiagram.getNode(
                gStartPosInfo.x, gStartPosInfo.y);
        if(node)
            {
            GraphPoint offset = gStartPosInfo;
            offset.sub(node->getRect().start);

            GraphPoint newPos = GraphPoint(static_cast<int>(event->x),
                static_cast<int>(event->y));
            newPos.sub(offset);

            node->setPos(newPos);
            mComponentDiagram.setModified();
            drawToDrawingArea();
            }
        }
    else
        {
        displayContextMenu(event->button, event->time, (gpointer)event);
        }
    }

void ComponentDiagramView::displayContextMenu(guint button, guint32 acttime,
    gpointer /*data*/)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawComponentPopupMenu");
    GtkCheckMenuItem *implicitItem = GTK_CHECK_MENU_ITEM(
            Builder::getBuilder()->getWidget("DrawComponentPopupMenu"));
    ComponentDrawOptions opts = getDrawOptions();
    gtk_check_menu_item_set_active(implicitItem, opts.drawImplicitRelations);
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

extern "C" G_MODULE_EXPORT void on_RestartComponentsMenuitem_activate(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gComponentDiagramView->restart();
    }

extern "C" G_MODULE_EXPORT void on_RelayoutComponentsMenuitem_activate(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    gComponentDiagramView->relayout();
    }

extern "C" G_MODULE_EXPORT void on_RemoveComponentMenuitem_activate(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    ComponentNode *node = gComponentDiagramView->getDiagram().getNode(gStartPosInfo.x,
            gStartPosInfo.y);
    if(node)
        {
        gComponentDiagramView->getDiagram().removeNode(*node, getDrawOptions());
        gComponentDiagramView->relayout();
        }
    }

extern "C" G_MODULE_EXPORT void on_ShowImplicitRelationsMenuitem_toggled(
        GtkWidget * /*widget*/, gpointer /*data*/)
    {
    bool drawImplicitRelations = gGuiOptions.getValueBool(OptGuiShowCompImplicitRelations);
    gGuiOptions.setNameValueBool(OptGuiShowCompImplicitRelations, !drawImplicitRelations);
    gComponentDiagramView->restart();
    gGuiOptions.writeFile();
    }
