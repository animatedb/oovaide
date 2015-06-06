/*
 * ZoneDiagram.h
 * Created on: Feb 9, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef ZONE_DIAGRAM_H
#define ZONE_DIAGRAM_H

#include "ZoneDrawer.h"
#include "Builder.h"

#include "Components.h"
#include "Gui.h"
#include "CairoDrawer.h"


// This is a tree of components (directories), no modules (filenames)
class ZoneDiagramList
    {
    public:
	void init();
	void update();
	GuiTreeItem getParent(std::string const &compName)
	    {
	    std::string parent = ComponentTypesFile::getComponentParentName(compName);
	    return mComponentTree.getItem(parent, '/');
	    }
	void clear()
	    {
	    mComponentTree.clear();
	    }
	GuiTree &getComponentTree()
	    { return mComponentTree; }
    private:
	GuiTree mComponentTree;
	ComponentTypesFile mComponentFile;
    };

class ZoneDiagramListener
    {
    public:
	virtual ~ZoneDiagramListener();
	virtual void gotoClass(OovStringRef const className) = 0;
    };

class ZoneScreenDrawer
    {
    public:
	ZoneScreenDrawer(ZoneDrawer &zoneDrawer);
	// If resetPositions is true, the positions overwritten.
	void requestDraw(ZoneGraph const &graph, double zoom, bool resetPositions = true);
	// This should be called from the draw event.
	void drawToDrawingArea();
	GtkWidget *getDrawingArea() const;

    private:
	GtkCairoContext mCairoContext;
	CairoDrawer mCairoDrawer;
	ZoneDrawer &mZoneDrawer;
	cairo_t *getCairo();

        void updateDrawingAreaSize();
    };

class ZoneClassInfoToolTipWindow
    {
    public:
	ZoneClassInfoToolTipWindow():
	    mTopWindow(nullptr), mLabel(nullptr)
	    {}
	~ZoneClassInfoToolTipWindow();
	void hide();
	void lostFocus();
	void handleCursorMovement(ZoneScreenDrawer const &drawer, int x, int y,
		OovStringRef str);

    private:
	GtkWidget *mTopWindow;
	GtkLabel *mLabel;
    };

/// This defines functions used to interact with a zone diagram. The
/// ZoneDiagram uses the ZoneDrawer to draw the ZoneGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ZoneDiagram
    {
    public:
	ZoneDiagram():
	    mModelData(nullptr), mZoneDrawer(nullptr), mZoneScreenDrawer(mZoneDrawer),
	    mListener(nullptr), mZoom(1)
	    {}
	void initialize(const ModelData &modelData,
		ZoneDiagramListener *mListener);
	/// Call updateDiagram after this
	void clearGraph()
            { getZoneGraph().clearGraph(); }
	/// Call updateDiagram after setting filter
	void setFilter(std::string moduleName, bool set)
	    {
	    mZoneGraph.setFilter(moduleName, set);
	    }
	void clearGraphAndAddWorldZone();
	void updateGraph(const ZoneDrawOptions &options, bool resetPositions=true);
	void drawSvgDiagram(FILE *fp);
	void restart();
	void zoom(bool inc);

	// For use by extern functions.
	void graphButtonPressEvent(const GdkEventButton *event);
	void graphButtonReleaseEvent(const GdkEventButton *event);
	void listDisplayContextMenu(const GdkEventButton *event);
	// This queues a draw request and sets the widget size.
	void updateDiagram(bool resetPositions = true);
	// This should be called from the draw event.
	void drawToDrawingArea();
	void showChildComponents(bool show);
	void showAllComponents(bool show);

	ZoneGraph &getZoneGraph()
	    { return mZoneGraph; }
	ZoneDrawer const &getZoneDrawer() const
	    { return mZoneDrawer; }
	ZoneDrawer &getZoneDrawer()
	    { return mZoneDrawer; }
	const ZoneGraph &getZoneGraph() const
	    { return mZoneGraph; }
	const ModelData &getModelData() const
	    { return *mModelData; }
	void gotoClass(OovStringRef const className)
	    {
	    if(mListener)
		{
		mListener->gotoClass(className);
		}
	    }
	void handleDrawingAreaLoseFocus()
	    { mToolTipWindow.lostFocus(); }
	void handleDrawingAreaMotion(int x, int y);

    private:
	const ModelData *mModelData;
	ZoneGraph mZoneGraph;
	ZoneDrawer mZoneDrawer;
	ZoneScreenDrawer mZoneScreenDrawer;
	ZoneDiagramListener *mListener;
	ZoneClassInfoToolTipWindow mToolTipWindow;
	double mZoom;
    };

#endif
