// ZoneDiagram.cpp

#include "ZoneDiagram.h"
#include "Svg.h"
#include "Journal.h"

static ZoneDiagram *gZoneDiagram;
static ZoneDiagramList *gZoneDiagramList;
static GraphPoint gStartPosInfo;


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

void ZoneClassInfoToolTipWindow::lostFocus()
    {
    hide();
    }

void ZoneClassInfoToolTipWindow::handleCursorMovement(ZoneScreenDrawer const &drawer,
	int x, int y, OovStringRef str)
    {
    if(!mTopWindow)
	{
	mTopWindow = gtk_window_new(GTK_WINDOW_POPUP);
	mLabel = GTK_LABEL(gtk_label_new(nullptr));
	// This doesn't fix the left justify
//	gtk_label_set_justify(mLabel, GTK_JUSTIFY_LEFT);
	gtk_container_add(GTK_CONTAINER(mTopWindow), GTK_WIDGET(mLabel));
	}
    Gui::setText(mLabel, str);
    Gui::setVisible(mTopWindow, true);
    // If the focus isn't grabbed, the drawing area won't get the
    // focus-out event.
    gtk_widget_grab_focus(GTK_WIDGET(drawer.getDrawingArea()));
    // Position tooltip window relative to the drawing area window.
    // If it clips on the right, then move it left.
    GdkWindow *drawWin = gtk_widget_get_window(GTK_WIDGET(drawer.getDrawingArea()));
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


void ZoneDiagramList::init()
    {
    mComponentTree.init(*Builder::getBuilder(), "ZoneTreeview",
	    "Component List", GuiTree::CT_StringBool);
    gZoneDiagramList = this;
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


ZoneDiagramListener::~ZoneDiagramListener()
    {
    }

void ZoneDiagram::initialize(const ModelData &modelData,
	ZoneDiagramListener *listener)
    {
    mModelData = &modelData;
    mListener = listener;
    gZoneDiagram = this;
    }

void ZoneDiagram::zoom(bool inc)
    {
    if(inc)
	{
	if(mZoom < 100)
	    mZoom *= 1.5;
	}
    else
	{
	if(mZoom > .1)
	    mZoom /= 1.5 ;
	}
    updateDiagram(true);
    }

void ZoneDiagram::clearGraphAndAddWorldZone()
    {
    getZoneGraph().clearAndAddWorldNodes(getModelData());
    updateDiagram();
    }

void ZoneDiagram::updateGraph(const ZoneDrawOptions &options, bool resetPositions)
    {
    getZoneGraph().setDrawOptions(options);
    updateDiagram(resetPositions);
    }

void ZoneDiagram::restart()
    {
    mZoom = 1;
    clearGraphAndAddWorldZone();
    }

void ZoneDiagram::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(Builder::getBuilder()->getWidget("DiagramDrawingarea"));

    SvgDrawer svgDrawer(fp, cairo.getCairo());

    ZoneDrawer &zoneDrawer = getZoneDrawer();
    DiagramDrawer *diagScreenDrawer = zoneDrawer.getDrawer();
    zoneDrawer.setDrawer(&svgDrawer);
    zoneDrawer.drawGraph();
    zoneDrawer.setDrawer(diagScreenDrawer);
/*
    /// @todo - this could use mZoneScreenDrawer with the SvgDrawer.
    /// This would save a bit of memory and calculation time.
    ZoneDrawer zoneDrawer(&svgDrawer);

    zoneDrawer.setZoom(mZoom);
    zoneDrawer.updateGraph(getZoneGraph(), false);
    zoneDrawer.drawGraph();
*/
    }

ZoneScreenDrawer::ZoneScreenDrawer(ZoneDrawer &zoneDrawer):
	mCairoDrawer(nullptr), mZoneDrawer(zoneDrawer)
    {
    }

void ZoneScreenDrawer::requestDraw(ZoneGraph const &graph, double zoom, bool resetPositions)
    {
    if(getCairo())	// Drawing area is not constructed initially.
	{
	mZoneDrawer.setZoom(zoom);
	mZoneDrawer.updateGraph(graph, resetPositions);
	gtk_widget_queue_draw(getDrawingArea());
	}
    }

void ZoneScreenDrawer::updateDrawingAreaSize()
    {
    GraphSize size = mZoneDrawer.getDrawingSize();
    gtk_widget_set_size_request(getDrawingArea(), size.x, size.y);
    }

void ZoneScreenDrawer::drawToDrawingArea()
    {
    updateDrawingAreaSize();
    if(getCairo())	// Drawing area is not constructed initially.
	{
	ZoneGraph const *graph = mZoneDrawer.getGraph();
	if(graph)
	    {
	    mZoneDrawer.drawGraph();
	    cairo_stroke(mCairoContext.getCairo());
	    }
	}
    }

GtkWidget *ZoneScreenDrawer::getDrawingArea() const
    {
    return Builder::getBuilder()->getWidget("DiagramDrawingarea");
    }

cairo_t *ZoneScreenDrawer::getCairo()
    {
//    if(!mCairoContext.getCairo())
	{
	GtkWidget *drawingArea = getDrawingArea();
	mCairoContext.setContext(drawingArea);
	}
    cairo_t *cairo = mCairoContext.getCairo();
    if(cairo)	// Drawing area is not constructed initially.
	{
	cairo_set_source_rgb(cairo, 255,255,255);
	cairo_paint(cairo);

	cairo_set_source_rgb(cairo, 0,0,0);
	cairo_set_line_width(cairo, 1.0);
	mCairoDrawer.setGraphicsLib(cairo);
	mZoneDrawer.setDrawer(&mCairoDrawer);
	}
    return cairo;
    }

void ZoneDiagram::updateDiagram(bool resetPositions)
    {
    getZoneGraph().updateGraph();
    mZoneScreenDrawer.requestDraw(getZoneGraph(), mZoom, resetPositions);
    }

void ZoneDiagram::drawToDrawingArea()
    {
    mZoneScreenDrawer.drawToDrawingArea();
    }

static void graphDisplayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawZonePopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);

    ZoneDrawOptions opts = gZoneDiagram->getZoneGraph().getDrawOptions();
    GtkCheckMenuItem *funcsItem = GTK_CHECK_MENU_ITEM(
	    Builder::getBuilder()->getWidget("ShowFunctionRelationsCheckmenuitem"));
    gtk_check_menu_item_set_active(funcsItem, opts.mDrawFunctionRelations);

    GtkCheckMenuItem *allClassesItem = GTK_CHECK_MENU_ITEM(
	    Builder::getBuilder()->getWidget("ShowAllClassesCheckmenuitem"));
    gtk_check_menu_item_set_active(allClassesItem, opts.mDrawAllClasses);
    }

void ZoneDiagram::graphButtonPressEvent(const GdkEventButton *event)
    {
    gStartPosInfo.set(event->x, event->y);
    }

void ZoneDiagram::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	ZoneNode const *node = getZoneDrawer().getZoneNode(gStartPosInfo);
	if(node)
	    {
	    getZoneDrawer().setPosition(node, gStartPosInfo,
		    GraphPoint(event->x, event->y));
	    updateDiagram(false);
	    }
	}
    else
	{
	graphDisplayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

void ZoneDiagram::handleDrawingAreaMotion(int x, int y)
    {
    const ZoneNode *node = getZoneDrawer().getZoneNode(GraphPoint(x, y));
    if(node)
	{
	std::string str = "Class name: ";
	str += node->mType->getName();
	str += "\nComponent name: ";
	str += node->mType->getClass()->getModule()->getName();
	mToolTipWindow.handleCursorMovement(mZoneScreenDrawer, x, y, str);
	}
    else
	{
	mToolTipWindow.hide();
	}
    }

void ZoneDiagram::listDisplayContextMenu(const GdkEventButton *event)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("ZoneListPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, event->button, event->time);
    }


void ZoneDiagram::showChildComponents(bool show)
    {
    gZoneDiagramList->getComponentTree().setSelectedChildCheckboxes(show);
    OovStringVec comps = gZoneDiagramList->getComponentTree().getSelectedChildNodeNames('/');
    for(auto const &comp : comps)
	{
	setFilter(comp, !show);
	}
    updateDiagram();
    }

void ZoneDiagram::showAllComponents(bool show)
    {
    gZoneDiagramList->getComponentTree().setAllCheckboxes(show);
    OovStringVec comps = gZoneDiagramList->getComponentTree().getAllChildNodeNames('/');
    for(auto const &comp : comps)
	{
	setFilter(comp, !show);
	}
    updateDiagram();
    }

extern "C" G_MODULE_EXPORT void on_ZoneGotoClassMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const ZoneNode *node = gZoneDiagram->getZoneDrawer().getZoneNode(gStartPosInfo);
    if(node)
	{
	gZoneDiagram->gotoClass(node->mType->getName());
	}
    }

extern "C" G_MODULE_EXPORT void on_ZoneViewSourceMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const ZoneNode *node = gZoneDiagram->getZoneDrawer().getZoneNode(gStartPosInfo);
    if(node)
	{
	const ModelClassifier *classifier = node->mType->getClass();
	if(classifier)
	    {
	    viewSource(classifier->getModule()->getModulePath(), classifier->getLineNum());
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ShowFunctionRelationsCheckmenuitem_toggled(
	GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = gZoneDiagram->getZoneGraph().getDrawOptions();
    drawOptions.mDrawFunctionRelations = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    gZoneDiagram->getZoneGraph().setDrawOptions(drawOptions);
    gZoneDiagram->updateGraph(drawOptions, false);
    }

extern "C" G_MODULE_EXPORT void on_ShowAllClassesCheckmenuitem_toggled(
	GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = gZoneDiagram->getZoneGraph().getDrawOptions();
    drawOptions.mDrawAllClasses = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    gZoneDiagram->getZoneGraph().setDrawOptions(drawOptions);
    gZoneDiagram->updateGraph(drawOptions);
    }

extern "C" G_MODULE_EXPORT void on_ShowDependenciesMenuitem_toggled(
	GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = gZoneDiagram->getZoneGraph().getDrawOptions();
    drawOptions.mDrawDependencies = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    gZoneDiagram->getZoneGraph().setDrawOptions(drawOptions);
    gZoneDiagram->updateGraph(drawOptions, false);
    }

extern "C" G_MODULE_EXPORT void on_ShowChildCirclesMenuitem_toggled(
	GtkWidget *widget, gpointer data)
    {
    ZoneDrawOptions drawOptions = gZoneDiagram->getZoneGraph().getDrawOptions();
    drawOptions.mDrawChildCircles = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    gZoneDiagram->getZoneGraph().setDrawOptions(drawOptions);
    gZoneDiagram->updateDiagram();
    }

extern "C" G_MODULE_EXPORT void on_ZoneShowChildrenMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagram)
	zoneDiagram->showChildComponents(true);
    }

extern "C" G_MODULE_EXPORT void on_ZoneHideChildrenMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagram)
	zoneDiagram->showChildComponents(false);
    }

extern "C" G_MODULE_EXPORT void on_ShowAllMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagram)
	zoneDiagram->showAllComponents(true);
    }

extern "C" G_MODULE_EXPORT void on_HideAllMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
    if(zoneDiagram)
	zoneDiagram->showAllComponents(false);
    }


static class ZoneMapPathDialog *gMapPathDialog;
class ZoneMapPathDialog:public Dialog
    {
    public:
	ZoneMapPathDialog()
	    {
	    gMapPathDialog = this;
	    setDialog(GTK_DIALOG(Builder::getBuilder()->getWidget("ZoneMapPathDialog")),
		    GTK_WINDOW(Builder::getBuilder()->getWidget("MainWindow")));
	    mPathMapList.init(*Builder::getBuilder(), "ZonePathMapTreeview",
		    "Path Map List");
	    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
	    mDiagramPathMap = &zoneDiagram->getZoneGraph().getPathMap();
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
    ZoneDiagram *zoneDiagram = Journal::getJournal()->getCurrentZoneDiagram();
    zoneDiagram->clearGraphAndAddWorldZone();
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
    gZoneDiagram->restart();
    }

extern "C" G_MODULE_EXPORT void on_ZoneRemoveAllMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gZoneDiagram->getZoneGraph().clearGraph();
    gZoneDiagram->updateDiagram();
    }

extern "C" G_MODULE_EXPORT void on_ZoneZoomInMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gZoneDiagram->zoom(true);
    }

extern "C" G_MODULE_EXPORT void on_ZoneZoomOutMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gZoneDiagram->zoom(false);
    }
