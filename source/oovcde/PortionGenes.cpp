/*
 * PortionGenes.cpp
 * Created on: April 29, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionGenes.h"
#include <math.h>	// For sqrt
#include <limits>
#include <map>


static const int NumGenerations = 30;

void PortionGenes::initialize(PortionDrawer &drawer, size_t nodeHeight)
    {
    mDrawer = &drawer;
    mNodeHeight = nodeHeight;
    size_t numNodes = drawer.getNumNodes();
    size_t numGenes = 0;
    if(numNodes > 100)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<size_t>(sqrt(numNodes)) * 10;
    else if(numNodes > 50)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<size_t>(sqrt(numNodes)) * 20;
    else
	numGenes = static_cast<size_t>(sqrt(numNodes)) * 80 + 50;
    // Each gene contains the Y position for every node.
    const size_t sizePos = sizeof(GeneValue);
    size_t geneBytes = numNodes * sizePos;
    mMaxDrawingHeight = static_cast<size_t>(nodeHeight * 5.0 * getMaxNodesInColumn());
    GenePool::initialize(geneBytes, numGenes, 0.35, 0.005, 0,
        static_cast<GeneValue>(mMaxDrawingHeight));
    }

size_t PortionGenes::getMaxNodesInColumn() const
    {
    size_t numNodes = mDrawer->getNumNodes();
    std::map<int, size_t> columnCounts;     // First is pos, second is count.
    for(size_t i=0; i<numNodes; i++)
	{
	columnCounts[mDrawer->getPosition(i).x]++;
	}
    size_t maxNodes = 0;
    for(auto it = columnCounts.begin(); it != columnCounts.end(); ++it)
	{
	if(it->second > maxNodes)
	    {
	    maxNodes = it->second;
	    }
	}
    return maxNodes;
    }

void PortionGenes::updatePositionsInDrawer()
    {
    for(int i=0; i<NumGenerations; i++)
	{
	singleGeneration();
	}
    size_t bestGeneI = getBestGeneIndex();
//printf("Best %d\n", bestGeneI);
//fflush(stdout);
    for(size_t i=0; i<mDrawer->getNumNodes(); i++)
	{
	GraphPoint pos = mDrawer->getPosition(i);
	pos.y = getYPosition(bestGeneI, i);
        mDrawer->setPosition(i, pos);
	}
    }

void PortionGenes::setupQualityEachGeneration()
    {
    mMaxDistanceQ = 0;
    mMaxOverlapQ = 0;
    mMaxHeightQ = 0;
    size_t numNodes = mDrawer->getNumNodes();
    for(size_t genei=0; genei<getNumGenes(); genei++)
	{
	// Move Y position of each gene to zero.
	int lowestY = 25000;
	for(size_t nodei=0; nodei<numNodes; nodei++)
	    {
	    int pos = getYPosition(genei, nodei);
	    if(pos < lowestY)
		{
		lowestY = pos;
		}
	    }
	for(size_t nodei=0; nodei<numNodes; nodei++)
	    {
	    setYPosition(genei, nodei, 
                static_cast<GeneValue>(getYPosition(genei, nodei)-lowestY));
	    }

	size_t distance = getNodeYDistances(genei);
	if(distance > mMaxDistanceQ)
	    {
	    mMaxDistanceQ = distance;
	    }

	size_t nodeOverlapCount = getNodeOverlapCount(genei);
	if(nodeOverlapCount > mMaxOverlapQ)
	    {
	    mMaxOverlapQ = nodeOverlapCount;
	    }

	size_t geneHeight = getDrawingHeight(genei);
	if(geneHeight > mMaxHeightQ)
	    {
	    mMaxHeightQ = geneHeight;
	    }
	}
    }

QualityType PortionGenes::calculateSingleGeneQuality(size_t geneIndex) const
    {
    int maxQual = std::numeric_limits<QualityType>::max() / 3;

    size_t nodesOverlapCount = getNodeOverlapCount(geneIndex);
    size_t overlapQ = static_cast<size_t>((1.0f - (
        static_cast<float>(nodesOverlapCount) / mMaxOverlapQ)) * maxQual);

    size_t distQ = 0;
#define PORT_DIST 1
#if(PORT_DIST)
    // Minimize distance between connected nodes.
    size_t distance = getNodeYDistances(geneIndex);
    distQ = static_cast<size_t>(((1.0f - (
        static_cast<float>(distance) / mMaxDistanceQ)) * maxQual) * .5);
#endif

    size_t sizeQ = 0;
#define PORT_SIZE 1
#if(PORT_SIZE)
    // Minimize total drawing height.
    size_t geneHeight = getDrawingHeight(geneIndex);
    sizeQ = static_cast<size_t>(((1.0f - (
        static_cast<float>(geneHeight) / mMaxHeightQ)) * maxQual) * .25);
#endif
    QualityType q = static_cast<QualityType>(overlapQ + distQ + sizeQ);
//printf("%d ", geneIndex);
//printf("%d ", distance);
//printf("   Q lap %d dist %d size %d total %d\n", overlapQ, distQ, sizeQ, q);
//fflush(stdout);
    return q;
    }

size_t PortionGenes::getNodeYDistances(size_t geneIndex) const
    {
    size_t distance = 0;
    for(size_t ci=0; ci<mDrawer->getNumConnections(); ci++)
	{
	PortionConnection const &conn = mDrawer->getConnection(ci);
	distance += static_cast<size_t>(
                abs((getYPosition(geneIndex, conn.mConsumerNodeIndex) -
		getYPosition(geneIndex, conn.mSupplierNodeIndex))));
	}
    return distance;
    }

size_t PortionGenes::getNodeOverlapCount(size_t geneIndex) const
    {
    size_t nodesOverlapCount = 0;
    size_t numNodes = mDrawer->getNumNodes();
    for(size_t ni1=0; ni1<numNodes; ni1++)
	{
	for(size_t ni2=ni1+1; ni2<numNodes; ni2++)
	    {
	    if(nodesOverlap(geneIndex, ni1, ni2))
		nodesOverlapCount++;
	    }
	}
    return nodesOverlapCount;
    }

// If we are only adjusting the Y, only check the Y overlap.
bool PortionGenes::nodesOverlap(size_t geneIndex, size_t node1, size_t node2) const
    {
    // Add some space between nodes
    size_t nodeHeight = mNodeHeight * 2;

    int p1Y = getYPosition(geneIndex, node1);
    int p2Y = getYPosition(geneIndex, node2);
    // X positions are in columns, so exact match works to check for overlap.
    bool overlap = ((static_cast<size_t>(abs(p1Y-p2Y)) < nodeHeight) &&
	    (mDrawer->getPosition(node1).x == mDrawer->getPosition(node2).x));
    return overlap;
    }

GeneValue PortionGenes::getYPosition(size_t geneIndex, size_t nodeIndex) const
    {
    return getValue(geneIndex, nodeIndex*sizeof(GeneValue));
    }

void PortionGenes::setYPosition(size_t geneIndex, size_t nodeIndex, GeneValue val)
    {
    return setValue(geneIndex, nodeIndex*sizeof(GeneValue), val);
    }

size_t PortionGenes::getDrawingHeight(size_t geneIndex) const
    {
    size_t ySize = 0;
    for(size_t ni=0; ni<mDrawer->getNumNodes(); ni++)
	{
	size_t y = getYPosition(geneIndex, ni);
	if(y > ySize)
	    {
	    ySize = y;
	    }
	}
    return ySize;
    }
