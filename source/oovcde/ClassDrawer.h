/*
 * ClassDrawer.h
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSDRAWER_H_
#define CLASSDRAWER_H_

#include "DiagramDrawer.h"
#include "ClassGraph.h"

class ClassDrawer
    {
    public:
	ClassDrawer(DiagramDrawer &drawer):
	    mDrawer(drawer)
	    {}
	void drawDiagram(const ClassGraph &graph, const ClassDrawOptions &options);
	// This is used publicly to calculate node sizes.
	GraphSize drawNode(const ClassNode &node);

    private:
	DiagramDrawer &mDrawer;
	// Typically node1 is consumer, and node2 is producer
	/// @todo - should make this constant and clear
	void drawConnectionLine(const ClassNode &node1, const ClassNode &node2);
	void drawConnectionSymbols(const ClassRelationDrawOptions &options, const ClassNode &node1,
	    const ClassNode &node2, const ClassConnectItem &connectItem);
	void getStrings(const ClassNode &node,
		std::vector<std::string> &nodeStrs, std::vector<std::string> &attrStrs,
		std::vector<std::string> &operStrs);
	void splitClassStrings(std::vector<std::string> &nodeStrs,
		std::vector<std::string> &attrStrs, std::vector<std::string> &operStrs);
    };


#endif /* CLASSDRAWER_H_ */
