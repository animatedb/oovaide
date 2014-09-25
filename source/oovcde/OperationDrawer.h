/*
 * OperationDrawer.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONDRAWER_H_
#define OPERATIONDRAWER_H_

#include "DiagramDrawer.h"
#include "OperationGraph.h"
#include <set>

struct DrawString
    {
    DrawString()
	{}
    DrawString(GraphPoint p, const  char *s):
	pos(p), str(s)
	{}
    GraphPoint pos;
    std::string str;
    };

/// Used to draw an operation graph.
class OperationDrawer
    {
    public:
	OperationDrawer(DiagramDrawer &drawer):
	    mDrawer(drawer), mPad(0), mCharHeight(0)
	    {}
	// This must be called before drawing svg diagrams.
	void setDiagramSize(GraphSize size);

	// Graph is not const because class positions get updated.
	GraphSize drawDiagram(OperationGraph &graph, const OperationDrawOptions &options);

    private:
	DiagramDrawer &mDrawer;
	int mPad;
	int mCharHeight;
	GraphSize drawClass(const OperationClass &node, const OperationDrawOptions &options);
	void drawLifeLines(const std::vector<OperationClass> &classes,
		std::vector<int> const &classEndY, int endy);
	GraphSize drawOperation(GraphPoint pos, OperationDefinition &operDef,
		const OperationGraph &graph, const OperationDrawOptions &options,
		std::set<const OperationDefinition*> &drawnOperations);
	GraphSize drawOperationNoText(GraphPoint pos, OperationDefinition &operDef,
		const OperationGraph &graph, const OperationDrawOptions &options,
		std::set<const OperationDefinition*> &drawnOperations,
		std::vector<DrawString> &drawStrings);
    };

#endif /* OPERATIONDRAWER_H_ */
