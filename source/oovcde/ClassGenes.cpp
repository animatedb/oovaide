/*
 * DiagramGenes.cpp
 *
 *  Created on: Jun 21, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassGenes.h"
#include "ClassGraph.h"
#include <stdlib.h>	// For abs
#include "Debug.h"
#include "Gui.h"

#define DEBUG_GENES 0
#if(DEBUG_GENES)
static bool sDisplayGene;
static DebugFile sFile("DebugGenes.txt");
#endif

static const int NumGenerations = 30;

void ClassGenes::initialize(const ClassGraph &graph, int avgNodeSize)
    {
    mGraph = &graph;
    int numNodes = graph.getNodes().size();
    int numGenes = 0;
    if(numNodes > 100)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<int>(sqrt(numNodes)) * 3;
    else if(numNodes > 50)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<int>(sqrt(numNodes)) * 5;
    else
	numGenes = static_cast<int>(sqrt(numNodes)) * 30 + 50;
    const int numPos = 2;
    const int sizePos = sizeof(uint16_t);
    int geneBytes = numNodes * sizePos * numPos;
    int maxPos = static_cast<int>(sqrt(numNodes) * (avgNodeSize* 1.5));
    GenePool::initialize(geneBytes, numGenes, 0.35, 0.005, 0, maxPos);
    }

#define LINES_OVERLAP 1
#if(LINES_OVERLAP)

int between(int a, int b, int c)
    {
    if (a > b)
	{
	int temp = a;
	a = b;
	b = temp;
	}
    return(a <= c && c <= b);
    }

double CCW(GraphPoint a, GraphPoint b, GraphPoint c)
    {
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
    }

bool linesOverlap(DiagramLine a, DiagramLine b)
    {
    if ((CCW(a.s, a.e, b.s) * CCW(a.s, a.e, b.e) < 0) &&
	    (CCW(b.s, b.e, a.s) * CCW(b.s, b.e, a.e) < 0))
	return true;

    if (CCW(a.s, a.e, b.s) == 0 && between(a.s.x, a.e.x, b.s.x) &&
	    between(a.s.y, a.e.y, b.s.y))
	return true;
    if (CCW(a.s, a.e, b.e) == 0 && between(a.s.x, a.e.x, b.e.x) &&
	    between(a.s.y, a.e.y, b.e.y))
	return true;
    if (CCW(b.s, b.e, a.s) == 0 && between(b.s.x, b.e.x, a.s.x) &&
	    between(b.s.y, b.e.y, a.s.y))
	return true;
    if (CCW(b.s, b.e, a.e) == 0 && between(b.s.x, b.e.x, a.e.x) &&
	    between(b.s.y, b.e.y, a.e.y))
	return true;
    return false;
    }

void ClassGenes::getLineForNodes(int geneIndex, int node1, int node2, DiagramLine &line) const
    {
    GraphRect r1;
    GraphRect r2;
    getNodeRect(geneIndex, node1, r1);
    getNodeRect(geneIndex, node2, r2);
    DiagramLine l(r1.centerx(), r1.centery(), r2.centerx(), r2.centery());
    line = l;
    }

bool ClassGenes::lineNodeOverlap(int geneIndex, int ni, DiagramLine const &line) const
    {
    GraphRect rect;
    getNodeRect(geneIndex, ni, rect);
    return linesOverlap(DiagramLine(rect.start.x, rect.start.y, rect.endx(), rect.start.y), line) ||
	linesOverlap(DiagramLine(rect.endx(), rect.start.y, rect.endx(), rect.endy()), line) ||
	linesOverlap(DiagramLine(rect.endx(), rect.endy(), rect.start.x, rect.endy()), line) ||
	linesOverlap(DiagramLine(rect.start.x, rect.endy(), rect.start.x, rect.start.y), line);
    }

#else

bool ClassGenes::isDistanceGoodQuality(int geneIndex, int ni1, int ni2) const
    {
    bool good = true;

    GraphRect rect1;
    GraphRect rect2;
    getNodeRect(geneIndex, ni1, rect1);
    getNodeRect(geneIndex, ni2, rect2);
    bool dummy = false;
    const ClassConnectItem *ci = mGraph->getNodeConnection(ni1, ni2, dummy);
    if(ci)
	{
	int maxXSize = std::max(rect1.size.x, rect2.size.x);
	int maxYSize = std::max(rect1.size.y, rect2.size.y);
	good = ((abs(rect1.start.x-rect2.start.x) < (maxXSize * 1.5)) &&
		(abs(rect1.start.y-rect2.start.y) < (maxYSize * 1.5)));
	}
    return good;
    }

#endif

void ClassGenes::setupQualityEachGeneration()
    {
    // Go through all genes and find largest size to use to scale size quality
    mDiagramSize.clear();
    for(size_t gi=0; gi<numgenes; gi++)
	{
	GraphSize size = getSize(gi);
	if(size.x > mDiagramSize.x)
	    mDiagramSize.x = size.x;
	if(size.y > mDiagramSize.y)
	    mDiagramSize.y = size.y;
	}
    }

GraphSize ClassGenes::getSize(int geneIndex) const
    {
    GraphSize size;
    for(size_t ni=0; ni<mGraph->getNodes().size(); ni++)
	{
	GraphRect rect;
	getNodeRect(geneIndex, ni, rect);
	if(rect.endx() > size.x)
	    size.x = rect.endx();
	if(rect.endy() > size.y)
	    size.y = rect.endy();
	}
    return size;
    }

QualityType ClassGenes::calculateSingleGeneQuality(size_t geneIndex) const
    {
    int nodesOverlapCount = 0;
#if(LINES_OVERLAP)
    int lineNodeOverlapCount = 0;
#else
    int distCount = 0;
#endif
    int numNodes = mGraph->getNodes().size();
    for(int ni1=0; ni1<numNodes; ni1++)
	{
	for(int ni2=ni1+1; ni2<numNodes; ni2++)
	    {
	    if(nodesOverlap(geneIndex, ni1, ni2))
		nodesOverlapCount++;
#if(LINES_OVERLAP)
#else
	    if(isDistanceGoodQuality(geneIndex, ni1, ni2))
		distCount++;
#endif
	    }
	}
#if(LINES_OVERLAP)
    int numConnections = mGraph->getConnections().size();
    for(auto const &nodePair : mGraph->getConnections())
	{
	DiagramLine line;
	getLineForNodes(geneIndex, nodePair.first.n1, nodePair.first.n2, line);
	for(int ni=0; ni<numNodes; ni++)
	    {
	    if(lineNodeOverlap(geneIndex, ni, line))
		lineNodeOverlapCount++;
	    }
	}
#endif
    int totalNodesQ = (numNodes * (numNodes-1)) / 2;
    QualityType nodesQ = totalNodesQ - nodesOverlapCount;

    int totalLineNodeQ = (numConnections * numNodes);
#if(LINES_OVERLAP)
    QualityType lineQ = totalLineNodeQ - lineNodeOverlapCount;
#else
    QualityType lineQ = distCount;
#endif
    GraphSize geneSize = getSize(geneIndex);
    QualityType sizeQ = 0;
    if(geneSize.x + geneSize.y > 0)
	{
	sizeQ = 10 - (10 * (geneSize.x + geneSize.y) / (mDiagramSize.x+mDiagramSize.y));
	if(sizeQ < 0)
	    sizeQ = 0;
	if(sizeQ > 10)
	    sizeQ = 10;
	}
    QualityType q = (nodesQ * 100) + (lineQ * 10) + sizeQ;
#if(DEBUG_GENES)
    if(sDisplayGene)
	{
	fprintf(sFile.mFp, "%4d/%4d %4d/%4d %4d %5d\n", nodesQ * 100, totalNodesQ * 100,
		lineQ * 10, totalLineNodeQ * 10,
		sizeQ, q);
	fflush(sFile.mFp);
	}
#endif
    return q;
    }

//If we check that:
// - The left edge of B is to the left of right edge of R.
// - The top edge of B is above the R bottom edge.
// - The right edge of B is to the right of left edge of R.
// - The bottom edge of B is below the R upper edge.
//Then we can say that rectangles are overlapping.
bool ClassGenes::nodesOverlap(int geneIndex, int node1, int node2) const
    {
    GraphRect rect1;
    getNodeRect(geneIndex, node1, rect1);
    GraphRect rect2;
    getNodeRect(geneIndex, node2, rect2);
    bool overlap = ((rect1.endx() >= rect2.start.x) &&
	    (rect1.endy() >= rect2.start.y) &&
	    (rect1.start.x <= rect2.endx()) &&
	    (rect1.start.y <= rect2.endy()));
    return overlap;
    }

void ClassGenes::getNodeRect(int geneIndex, int nodeIndex,
	GraphRect &rect) const
    {
    // Get the size from the node, and the position from the gene pool.
    rect.size = mGraph->getNodeSizeWithPadding(nodeIndex);
    getPosition(geneIndex, nodeIndex, rect.start);
    }

void ClassGenes::getPosition(int geneIndex, int nodeIndex, GraphPoint &pos) const
    {
    pos.x = getValue(geneIndex, nodeIndex*2*sizeof(GeneValue));
    pos.y = getValue(geneIndex, nodeIndex*2*sizeof(GeneValue)+sizeof(GeneValue));
    }

void ClassGenes::updatePositionsInGraph(class ClassGraph &graph,
	OovTaskStatusListener *listener, OovTaskContinueListener &contListener)
    {
    OovTaskStatusListenerId taskId = 0;
    if(listener)
	{
	taskId = listener->startTask("Optimizing layout.", NumGenerations);
	}
    for(int i=0; i<NumGenerations && contListener.continueProcessingItem(); i++)
	{
	singleGeneration();
	if(!listener->updateProgressIteration(taskId, i, nullptr))
	    {
	    break;
	    }
	}
    int bestGeneI = getBestGeneIndex();
#if(DEBUG_GENES)
    sDisplayGene = true;
    calculateSingleGeneQuality(bestGeneI);
    sDisplayGene = false;
#endif
    for(size_t i=0; i<graph.getNodes().size(); i++)
	{
	GraphPoint pos;
	getPosition(bestGeneI, i, pos);
	graph.getNodes()[i].setPosition(pos);
	}
    if(listener)
	{
	listener->endTask(taskId);
	}
    }
