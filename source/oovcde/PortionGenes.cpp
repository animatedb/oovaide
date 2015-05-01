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

void PortionGenes::initialize(PortionDrawer &drawer, int nodeHeight)
    {
    mDrawer = &drawer;
    mNodeHeight = nodeHeight;
    int numNodes = drawer.getNumNodes();
    int numGenes = 0;
    if(numNodes > 100)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<int>(sqrt(numNodes)) * 10;
    else if(numNodes > 50)	// Too many nodes to draw well, so make fewer genes for speed
	numGenes = static_cast<int>(sqrt(numNodes)) * 20;
    else
	numGenes = static_cast<int>(sqrt(numNodes)) * 80 + 50;
    // Each gene contains the Y position for every node.
    const int sizePos = sizeof(GeneValue);
    int geneBytes = numNodes * sizePos;
    mMaxDrawingHeight = nodeHeight * 5.0 * getMaxNodesInColumn();
    GenePool::initialize(geneBytes, numGenes, 0.35, 0.005, 0, mMaxDrawingHeight);
    }

int PortionGenes::getMaxNodesInColumn() const
    {
    int numNodes = mDrawer->getNumNodes();
    std::map<int, int> columnCounts;
    for(int i=0; i<numNodes; i++)
	{
	columnCounts[mDrawer->getPosition(i).x]++;
	}
    int maxNodes = 0;
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
    int bestGeneI = getBestGeneIndex();
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
    for(int genei=0; genei<getNumGenes(); genei++)
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
	    setYPosition(genei, nodei, getYPosition(genei, nodei)-lowestY);
	    }

	int distance = getNodeYDistances(genei);
	if(distance > mMaxDistanceQ)
	    {
	    mMaxDistanceQ = distance;
	    }

	int nodeOverlapCount = getNodeOverlapCount(genei);
	if(nodeOverlapCount > mMaxOverlapQ)
	    {
	    mMaxOverlapQ = nodeOverlapCount;
	    }

	int geneHeight = getDrawingHeight(genei);
	if(geneHeight > mMaxHeightQ)
	    {
	    mMaxHeightQ = geneHeight;
	    }
	}
    }

QualityType PortionGenes::calculateSingleGeneQuality(int geneIndex) const
    {
    int maxQual = std::numeric_limits<QualityType>::max() / 3;

    int nodesOverlapCount = getNodeOverlapCount(geneIndex);
    int overlapQ = (int)((1.0f - ((float)nodesOverlapCount / mMaxOverlapQ)) * maxQual);

    int distQ = 0;
#define PORT_DIST 1
#if(PORT_DIST)
    // Minimize distance between connected nodes.
    int distance = getNodeYDistances(geneIndex);
    distQ = (int)((1.0f - ((float)distance / mMaxDistanceQ)) * maxQual) * .5;
#endif

    int sizeQ = 0;
#define PORT_SIZE 1
#if(PORT_SIZE)
    // Minimize total drawing height.
    int geneHeight = getDrawingHeight(geneIndex);
    sizeQ = (int)((1.0f - ((float)geneHeight / mMaxHeightQ)) * maxQual) * .25;
#endif
    QualityType q = overlapQ + distQ + sizeQ;
//printf("%d ", geneIndex);
//printf("%d ", distance);
//printf("   Q lap %d dist %d size %d total %d\n", overlapQ, distQ, sizeQ, q);
//fflush(stdout);
    return q;
    }

int PortionGenes::getNodeYDistances(int geneIndex) const
    {
    int distance = 0;
    for(size_t ci=0; ci<mDrawer->getNumConnections(); ci++)
	{
	PortionConnection const &conn = mDrawer->getConnection(ci);
	distance += abs((getYPosition(geneIndex, conn.mConsumerNodeIndex) -
		getYPosition(geneIndex, conn.mSupplierNodeIndex)));
	}
    return distance;
    }

int PortionGenes::getNodeOverlapCount(int geneIndex) const
    {
    int nodesOverlapCount = 0;
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
bool PortionGenes::nodesOverlap(int geneIndex, int node1, int node2) const
    {
    // Add some space between nodes
    int nodeHeight = mNodeHeight * 2;

    int p1Y = getYPosition(geneIndex, node1);
    int p2Y = getYPosition(geneIndex, node2);
    // X positions are in columns, so exact match works to check for overlap.
    bool overlap = ((abs(p1Y-p2Y) < nodeHeight) &&
	    (mDrawer->getPosition(node1).x == mDrawer->getPosition(node2).x));
    return overlap;
    }

GeneValue PortionGenes::getYPosition(int geneIndex, int nodeIndex) const
    {
    return getValue(geneIndex, nodeIndex*sizeof(GeneValue));
    }

void PortionGenes::setYPosition(int geneIndex, int nodeIndex, GeneValue val)
    {
    return setValue(geneIndex, nodeIndex*sizeof(GeneValue), val);
    }

int PortionGenes::getDrawingHeight(int geneIndex) const
    {
    int ySize = 0;
    for(size_t ni=0; ni<mDrawer->getNumNodes(); ni++)
	{
	int y = getYPosition(geneIndex, ni);
	if(y > ySize)
	    {
	    ySize = y;
	    }
	}
    return ySize;
    }
