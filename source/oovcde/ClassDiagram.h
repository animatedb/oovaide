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
	virtual void gotoClass(char const * const className) = 0;
    };



// This must remain for the life of the program since GUI events
// can be generated any time.
class ClassDiagram
    {
    public:
	ClassDiagram():
	    mModelData(nullptr), mBuilder(nullptr), mListener(nullptr)
	    {}
	void initialize(Builder &builder, const ModelData &modelData,
		ClassDiagramListener *listener);
	void updateGraph();
	// Create a new graph and add a class node.
	void clearGraphAndAddClass(char const * const className);
	// Add a class node to an existing graph.
	void addClass(char const * const className);
	void drawSvgDiagram(FILE *fp);

	// For use by extern functions.
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
	void gotoClass(char const * const className)
	    {
	    if(mListener)
		{
		mListener->gotoClass(className);
		}
	    }
    private:
	const ModelData *mModelData;
	ClassGraph mClassGraph;
	Builder *mBuilder;
	std::string mLastSelectedClassName;
	ClassDiagramListener *mListener;
	void displayContextMenu(guint button, guint32 acttime, gpointer data);
	void setLastSelectedClassName(const std::string name)
	    { mLastSelectedClassName = name; }
    };

#endif
