/*
 * OperationDiagram.h
 *
 *  Created on: Jul 29, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONDIAGRAM_H_
#define OPERATIONDIAGRAM_H_

#include "OperationDrawer.h"
#include "Builder.h"


class OperationDiagramListener
    {
    public:
	virtual ~OperationDiagramListener()
	    {}
	virtual void gotoClass(char const * const className) = 0;
    };

class OperationDiagram
    {
    public:
	OperationDiagram():
	    mModelData(nullptr), mBuilder(nullptr), mListener(nullptr)
	    {}
	void initialize(Builder &builder, const ModelData &modelData,
		OperationDiagramListener *mListener);
	void clearGraph();
	void clearGraphAndAddOperation(char const * const className,
		char const * const opName, bool isConst);
	void drawSvgDiagram(FILE *fp);

	// For use by extern functions.
	void buttonPressEvent(const GdkEventButton *event);
	void buttonReleaseEvent(const GdkEventButton *event);
	void updateDiagram();
	void drawToDrawingArea();
	OperationGraph &getOpGraph()
	    { return mOpGraph; }
	const OperationGraph &getOpGraph() const
	    { return mOpGraph; }
	Builder &getBuilder()
	    { return *mBuilder; }
	const ModelData &getModelData() const
	    { return *mModelData; }
	const OperationDrawOptions &getOptions() const
	    { return mOptions; }
	void gotoClass(char const * const className)
	    {
	    if(mListener)
		{
		mListener->gotoClass(className);
		}
	    }

    private:
	const ModelData *mModelData;
	OperationDrawOptions mOptions;
	OperationGraph mOpGraph;
	Builder *mBuilder;
	OperationDiagramListener *mListener;
    };


#endif /* OPERATIONDIAGRAM_H_ */
