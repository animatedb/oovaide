/*
 * ZoneDiagramView.cpp
 *
 *  Created on: Jun 19, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ZoneDiagramView.h"
#include "Svg.h"
#include "Options.h"
#include "Journal.h"

static ZoneDiagramView *sZoneDiagramView;
static ZoneDiagramList *sZoneDiagramList;
static GraphPoint sStartPosInfo;

void ZoneDiagramList::init()
    {
    mComponentTree.init(*Builder::getBuilder(), "ZoneTreeview",
            "Component List", GuiTree::CT_StringBool, "Show");
    sZoneDiagramList = this;
    }

void ZoneDiagramList::update()
    {
    if(mComponentFile.read())
        {
        for(auto const &name : mComponentFile.getComponentNames())
            {
            mComponentTree.appendText(getParent(name),
                    ComponentTypesFile::getComponentChildName(name));
            }
        }
    mComponentTree.setAllCheckboxes(true);
    }

ZoneClassInfoToolTipWindow::~ZoneClassInfoToolTipWindow()
    {
    if(mLabel)
        {
        gtk_widget_destroy(GTK_WIDGET(mLabel));
        }
    if(mTopWindow)
        {
        gtk_widget_destroy(GTK_WIDGET(mTopWindow));
        }
    }

void ZoneClassInfoToolTipWindow::hide()
    {
    if(mTopWindow)
        {
        Gui::setVisible(mTopWindow, false);
        }
    }

void ZoneClassInfoToolTipWindow::lostPointer()
    {
    hide();
    }

void ZoneClassInfoToolTipWindow::handleCursorMovement(int x, int y,
        OovStringRef str)
    {
    if(!mTopWindow)
        {
        mTopWindow = gtk_window_new(GTK_WINDOW_POPUP);
        mLabel = GTK_LABEL(gtk_label_new(nullptr));
        // This doesn't fix the left justify
//      gtk_label_set_justify(mLabel, GTK_JUSTIFY_LEFT);
        gtk_container_add(GTK_CONTAINER(mTopWindow), GTK_WIDGET(mLabel));
        }
    Gui::setText(mLabel, str);
    Gui::setVisible(mTopWindow, true);
    // If the focus isn't grabbed, the drawing area won't get the
    // focus-out event.
    gtk_widget_grab_focus(getDiagramWidget());
    // Position tooltip window relative to the drawing area window.
    // If it clips on the right, then move it left.
    GdkWindow *drawWin = gtk_widget_get_window(getDiagramWidget());
    GdkWindow *tipWin = gtk_widget_get_window(GTK_WIDGET(mTopWindow));
    int winX;
    int winY;
    gdk_window_get_origin(drawWin, &winX, &winY);
    int padHeight = 10;
    x += winX;
    y += winY + padHeight;
    int screenWidth = gdk_screen_get_width(gtk_widget_get_screen(mTopWindow));
    int winWidth = gdk_window_get_width(tipWin);
    if(x+winWidth > screenWidth)
        x = screenWidth - winWidth;
    gdk_window_move(tipWin, x, y);
    }

void ZoneDiagramView::handleDrawingAreaMotion(int x, int y)
    {
    const ZoneNode *node = getDiagram().getZoneNode(GraphPoint(x, y));
    if(node)
        {
        std::string str = "Class name: ";
        str += node->mType->getName();
        str += "\nComponent name: ";
        str += node->mType->getClass()->getModule()->getName();
        mToolTipWindow.handleCursorMovement(x, y, str);
        }
    else
        {
        mToolTipWindow.hide();
        }
    }

void ZoneDiagramView::gotoClass(OovStringRef const className)
    {
    if(mListener)
        {
        mListener->gotoClass(className);
        }
    }


ZoneDiagramListener::~ZoneDiagramListener()
    {
    }



void ZoneDiagramView::drawToDrawingArea()
    {
    GtkCairoContext cairo(getDiagramWidget());
    CairoDrawer cairoDrawer(cairo.getCairo());
    cairoDrawer.clearAndSetDefaults();

    mZoneDiagram.drawDiagram(cairoDrawer);
    updateDrawingAreaSize();
    }

bool ZoneDiagramView::drawSvgDiagram(File &file)
    {
    GtkCairoContext cairo(getDiagramWidget());
    SvgDrawer svgDrawer(file, cairo.getCairo());
    mZoneDiagram.drawDiagram(svgDrawer);
    return svgDrawer.writeFile();
    }

void ZoneDiagramView::updateDrawingAreaSize()
    {
    GraphSize size = mZoneDiagram.getDrawingSize();
    gtk_widget_set_size_request(getDiagramWidget(), size.x, size.y);
    }

void ZoneDiagramView::listDisplayContextMenu(const GdkEventButton *event)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("ZoneListPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, event->button, event->time);
    }


static void graphDisplayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagramView)
        {
        GtkMenu *menu = Builder::getBuilder()->getMenu("DrawZonePopupMenu");
        gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);

        ZoneDrawOptions opts = zoneDiagramView->getDiagram().getDrawOptions();
        GtkCheckMenuItem *funcsItem = GTK_CHECK_MENU_ITEM(
                Builder::getBuilder()->getWidget("ShowFunctionRelationsCheckmenuitem"));
        gtk_check_menu_item_set_active(funcsItem, opts.mDrawFunctionRelations);

        GtkCheckMenuItem *allClassesItem = GTK_CHECK_MENU_ITEM(
                Builder::getBuilder()->getWidget("ShowAllClassesCheckmenuitem"));
        gtk_check_menu_item_set_active(allClassesItem, opts.mDrawAllClasses);
        }
    }

void ZoneDiagramView::graphButtonPressEvent(const GdkEventButton *event)
    {
    sZoneDiagramView = this;
    sStartPosInfo.set(event->x, event->y);
    }

void ZoneDiagramView::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
        {
        ZoneNode const *node = mZoneDiagram.getZoneNode(sStartPosInfo);
        if(node)
            {
            mZoneDiagram.setPosition(node, sStartPosInfo, GraphPoint(event->x, event->y));
            updateGraphAndRequestRedraw();
            }
        }
    else
        {
        graphDisplayContextMenu(event->button, event->time, (gpointer)event);
        }
    }


void ZoneDiagramView::showChildComponents(bool show)
    {
    sZoneDiagramList->getComponentTree().setSelectedChildCheckboxes(show);
    OovStringVec comps = sZoneDiagramList->getComponentTree().getSelectedChildNodeNames('/');
    for(auto const &comp : comps)
        {
        setFilter(comp, !show);
        }
    updateGraphAndRequestRedraw();
    }

void ZoneDiagramView::showAllComponents(bool show)
    {
    sZoneDiagramList->getComponentTree().setAllCheckboxes(show);
    OovStringVec comps = sZoneDiagramList->getComponentTree().getAllChildNodeNames('/');
    for(auto const &comp : comps)
        {
        setFilter(comp, !show);
        }
    updateGraphAndRequestRedraw();
    }

void ZoneDiagramView::viewSource(OovStringRef const module, unsigned int lineNum)
    {
    ::viewSource(mGuiOptions, module, lineNum);
    }


extern "C" G_MODULE_EXPORT void on_ZoneGotoClassMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    const ZoneNode *node = sZoneDiagramView->getDiagram().getZoneNode(sStartPosInfo);
    if(node)
        {
        sZoneDiagramView->gotoClass(node->mType->getName());
        }
    }

extern "C" G_MODULE_EXPORT void on_ZoneViewSourceMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    const ZoneNode *node = sZoneDiagramView->getDiagram().getZoneNode(sStartPosInfo);
    if(node)
        {
        const ModelClassifier *classifier = node->mType->getClass();
        if(classifier)
            {
            sZoneDiagramView->viewSource(classifier->getModule()->getModulePath(),
                    classifier->getLineNum());
            }
        }
    }

extern "C" G_MODULE_EXPORT void on_ShowFunctionRelationsCheckmenuitem_toggled(
        GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = sZoneDiagramView->getDiagram().getDrawOptions();
    drawOptions.mDrawFunctionRelations = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    sZoneDiagramView->getDiagram().setDrawOptions(drawOptions);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ShowAllClassesCheckmenuitem_toggled(
        GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = sZoneDiagramView->getDiagram().getDrawOptions();
    drawOptions.mDrawAllClasses = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    sZoneDiagramView->getDiagram().setDrawOptions(drawOptions);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ShowDependenciesMenuitem_toggled(
        GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = sZoneDiagramView->getDiagram().getDrawOptions();
    drawOptions.mDrawDependencies = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    sZoneDiagramView->getDiagram().setDrawOptions(drawOptions);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ShowChildCirclesMenuitem_toggled(
        GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = sZoneDiagramView->getDiagram().getDrawOptions();
    drawOptions.mDrawChildCircles = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    sZoneDiagramView->getDiagram().setDrawOptions(drawOptions);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ZoneShowChildrenMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagramView)
        zoneDiagramView->showChildComponents(true);
    }

extern "C" G_MODULE_EXPORT void on_ZoneHideChildrenMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagramView)
        zoneDiagramView->showChildComponents(false);
    }

extern "C" G_MODULE_EXPORT void on_ShowAllMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagramView)
        zoneDiagramView->showAllComponents(true);
    }

extern "C" G_MODULE_EXPORT void on_HideAllMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagramView)
        zoneDiagramView->showAllComponents(false);
    }


static class ZoneMapPathDialog *gMapPathDialog;
class ZoneMapPathDialog:public Dialog
    {
    public:
        ZoneMapPathDialog()
            {
            gMapPathDialog = this;
            setDialog(GTK_DIALOG(Builder::getBuilder()->getWidget("ZoneMapPathDialog")));
            mPathMapList.init(*Builder::getBuilder(), "ZonePathMapTreeview",
                    "Path Map List");
            ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
            mDiagramPathMap = &zoneDiagramView->getDiagram().getPathMap();
            mPathMapList.clear();
            for(auto const &item : *mDiagramPathMap)
                {
                appendGuiListItem(item.mSearchPath, item.mReplacePath);
                }
            }
        void append(OovString const &searchPath, OovString const &replacePath)
            {
            appendGuiListItem(searchPath, replacePath);
            mDiagramPathMap->push_back(ZonePathReplaceItem(searchPath, replacePath));
            }
        void deleteSelected()
            {
            OovString selStr = mPathMapList.getSelected();
            if(selStr.length() > 0)
                {
                OovStringVec vec = selStr.split(" > ");
                if(vec.size() == 2)
                    {
                    mPathMapList.removeSelected();
                    mDiagramPathMap->remove(ZonePathReplaceItem(vec[0], vec[1]));
                    }
                }
            }
    private:
        GuiList mPathMapList;
        ZonePathMap *mDiagramPathMap;
        void appendGuiListItem(OovString const &searchPath, OovString const &replacePath)
            {
            OovString totalStr = searchPath + " > " + replacePath;
            mPathMapList.appendText(totalStr);
            }
    };

extern "C" G_MODULE_EXPORT void on_ZoneRenamePathMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    ZoneMapPathDialog dlg;
    dlg.run(true);
    ZoneDiagramView *zoneDiagramView = Journal::getJournal()->getCurrentZoneDiagram();
    zoneDiagramView->clearGraphAndAddWorldZone();
    }

extern "C" G_MODULE_EXPORT void on_ZonePathMapAddButton_clicked(
        GtkWidget *widget, gpointer data)
    {
    OovString searchPath = Gui::getText(GTK_ENTRY(Builder::getBuilder()->
            getWidget("ZoneSearchPathEntry")));
    OovString replacePath = Gui::getText(GTK_ENTRY(Builder::getBuilder()->
            getWidget("ZoneReplacePathEntry")));
    gMapPathDialog->append(searchPath, replacePath);
    }

extern "C" G_MODULE_EXPORT void on_ZonePathMapDeleteButton_clicked(
        GtkWidget *widget, gpointer data)
    {
    gMapPathDialog->deleteSelected();
    }

extern "C" G_MODULE_EXPORT void on_ZoneRestartMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sZoneDiagramView->getDiagram().clearGraphAndAddWorldZone();
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ZoneRemoveAllMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sZoneDiagramView->getDiagram().clearGraph();
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ZoneZoomInMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sZoneDiagramView->getDiagram().zoom(true);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }

extern "C" G_MODULE_EXPORT void on_ZoneZoomOutMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sZoneDiagramView->getDiagram().zoom(false);
    sZoneDiagramView->updateGraphAndRequestRedraw();
    }
