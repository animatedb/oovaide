/*
 * ClassGuiBinding.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSGUIBINDING_H_
#define CLASSGUIBINDING_H_

#include "ClassGraph.h"
#include "Builder.h"

class ClassDiagramListener
    {
    public:
	virtual ~ClassDiagramListener()
	    {}
	virtual void gotoClass(OovStringRef const className) = 0;
    };


/// This defines functions used to interact with a class diagram. The
/// ClassDiagram uses the ClassDrawer to draw the ClassGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ClassDiagram
    {
    public:
	ClassDiagram():
	    mModelData(nullptr), mBuilder(nullptr), mListener(nullptr), mDesiredZoom(1.0)
	    {}
	void initialize(Builder &builder, const ModelData &modelData,
		ClassDiagramListener *listener);
	void updateGraph();
	void updateGraphSize();
	void restart();
	/// Redraw the graph. Does not change the graph or change or access the model.
	void drawDiagram(const ClassDrawOptions &options);
	// Create a new graph and add a class node.
	void clearGraphAndAddClass(OovStringRef const className,
		ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All);
	// Add a class node to an existing graph.
	void addClass(OovStringRef const className,
		ClassGraph::eAddNodeTypes addType=ClassGraph::AN_All);
	void drawSvgDiagram(FILE *fp);
	GtkWidget *getDiagramWidget()
	    { return getBuilder().getWidget("DiagramDrawingarea"); }

	// For use by extern functions.
	ClassNode *getNode(int x, int y);
	void buttonPressEvent(const GdkEventButton *event);
	void buttonReleaseEvent(const GdkEventButton *event);
	void drawToDrawingArea();
	ClassGraph &getClassGraph()
	    { return mClassGraph; }
	const ClassGraph &getClassGraph() const
	    { return mClassGraph; }
	Builder &getBuilder()
	    { return *mBuilder; }
	const ModelData &getModelData() const
	    { return *mModelData; }
	void gotoClass(OovStringRef const className)
	    {
	    if(mListener)
		{
		mListener->gotoClass(className);
		}
	    }
	std::string const &getLastSelectedClassName() const
	    { return mLastSelectedClassName; }
	void setZoom(double zoom)
	    { mDesiredZoom = zoom; }
	double getDesiredZoom() const
	    { return mDesiredZoom; }

    private:
	const ModelData *mModelData;
	ClassGraph mClassGraph;
	Builder *mBuilder;
	OovString mLastSelectedClassName;
	ClassDiagramListener *mListener;
	double mDesiredZoom;
	void displayContextMenu(guint button, guint32 acttime, gpointer data);
	void setLastSelectedClassName(OovStringRef const name)
	    { mLastSelectedClassName = name; }
    };

#endif
