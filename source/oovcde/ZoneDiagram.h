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
	virtual ~ZoneDiagramListener()
	    {}
	virtual void gotoClass(OovStringRef const className) = 0;
    };

class ZoneScreenDrawer
    {
    public:
	ZoneScreenDrawer();
	void requestDraw(ZoneGraph const &graph, double zoom);
	void drawToDrawingArea();
	ZoneDrawer const &getZoneDrawer() const
	    { return mZoneDrawer; }

    private:
	GtkCairoContext mCairoContext;
	CairoDrawer mCairoDrawer;
	ZoneDrawer mZoneDrawer;
	GtkWidget *getDrawingArea();
	cairo_t *getCairo();
    };

/// This defines functions used to interact with a zone diagram. The
/// ZoneDiagram uses the ZoneDrawer to draw the ZoneGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ZoneDiagram
    {
    public:
	ZoneDiagram():
	    mModelData(nullptr), mZoneScreenDrawer(),
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
	void updateGraph(const ZoneDrawOptions &options);
	void drawSvgDiagram(FILE *fp);
	void restart();
	void zoom(bool inc);

	// For use by extern functions.
	void graphButtonPressEvent(const GdkEventButton *event);
	void graphButtonReleaseEvent(const GdkEventButton *event);
	void listDisplayContextMenu(const GdkEventButton *event);
	// This queues a draw request and sets the widget size.
	void updateDiagram();
	// This should be called from the draw event.
	void drawToDrawingArea();
	void showChildComponents(bool show);
	void showAllComponents(bool show);

	ZoneGraph &getZoneGraph()
	    { return mZoneGraph; }
	ZoneDrawer const &getZoneDrawer() const
	    { return mZoneScreenDrawer.getZoneDrawer(); }
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

    private:
	const ModelData *mModelData;
	ZoneGraph mZoneGraph;
	ZoneScreenDrawer mZoneScreenDrawer;
	ZoneDiagramListener *mListener;
	double mZoom;
    };

#endif
