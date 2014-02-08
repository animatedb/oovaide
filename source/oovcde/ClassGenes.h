/*
 * ClassGenes.h
 *
 *  Created on: Jun 21, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSGENES_H_
#define CLASSGENES_H_

#include "FastGene.h"
#include "ModelObjects.h"
#include "Graph.h"
#include <math.h>	// For sqrt

struct DiagramLine
    {
    DiagramLine()
	{}
    DiagramLine(int sx, int sy, int ex, int ey):
	s(sx, sy), e(ex, ey)
	{}
    GraphPoint s;
    GraphPoint e;
    };

class ClassGenes:public GenePool
    {
    public:
	void initialize(const class ClassGraph &graph, int avgNodeSize);
	// Do some iterations, and move the positions from the gene pool into the graph.
	void updatePositionsInGraph(class ClassGraph &graph);

    private:
	const class ClassGraph *mGraph;
	GraphSize mDiagramSize;

	virtual void setupQualityEachGeneration();
	virtual QualityType calculateSingleGeneQuality(int geneIndex) const;
	// GeneIndex contains the positions
	bool nodesOverlap(int geneIndex, int node1, int node2) const;
	bool lineNodeOverlap(int geneIndex, int ni, DiagramLine const &line) const;
	void getLineForNodes(int geneIndex, int node1, int node2, DiagramLine &line) const;
	void getNodeRect(int geneIndex, int nodeIndex, class GraphRect &rect) const;
	void getPosition(int geneIndex, int nodeIndex, class GraphPoint &pos) const;
	bool isDistanceGoodQuality(int geneIndex, int ni1, int ni2) const;
	GraphSize getSize(int geneIndex) const;
    };

#endif
