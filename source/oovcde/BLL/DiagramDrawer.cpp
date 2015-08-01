/*
 * DiagramDrawer.cpp
 *
 *  Created on: Jul 3, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "DiagramDrawer.h"
#include "Options.h"
#include "OovProcess.h"
#include "Gui.h"
#include "Project.h"
#include <stdio.h>
#include <math.h>       // for atan, sqrt
#include <limits>
#include <map>


// Update the returned pos only if the found breakstring starting from
// startpos is less than pos.
static size_t findEarliestBreakPos(size_t pos, std::string const &str,
        char const *breakStr, size_t startPos)
    {
    size_t breakPos = str.find(breakStr, startPos);
    if(breakPos != std::string::npos)
        {
        breakPos += std::string(breakStr).length();
        if(pos == std::string::npos || breakPos < pos)
            {
            pos = breakPos;
            }
        }
    return pos;
    }

size_t findSplitPos(std::string const &str, size_t minLength, size_t maxLength)
    {
    size_t pos = maxLength;
    pos = findEarliestBreakPos(pos, str, ",", minLength);
    pos = findEarliestBreakPos(pos, str, "::", minLength);
    pos = findEarliestBreakPos(pos, str, " ", minLength);
    pos = findEarliestBreakPos(pos, str, "<", minLength);
    pos = findEarliestBreakPos(pos, str, ">", minLength);
    if(pos == std::string::npos || pos > maxLength)
        pos = maxLength;
    return pos;
    }

void splitStrings(OovStringVec &strs, size_t minLength, size_t maxLength)
    {
    for(size_t i=0; i<strs.size(); i++)
        {
        if(strs[i].length() > maxLength)
            {
            size_t pos = findSplitPos(strs[i], minLength, maxLength);
            std::string temp = getSplitStringPrefix() + strs[i].substr(pos);
            strs[i].resize(pos);
            strs.insert(strs.begin()+static_cast<int>(i)+1, temp);
            }
        }
    }

DiagramArrow::DiagramArrow(GraphPoint producer, GraphPoint consumer, int arrowSize)
    {
    int xdist = producer.x-consumer.x;
    int ydist = producer.y-consumer.y;
    double lineAngleRadians;

    arrowSize = -arrowSize;
    if(ydist != 0)
        lineAngleRadians = atan2(xdist, ydist);
    else
        {
        if(producer.x > consumer.x)
            lineAngleRadians = M_PI/2;
        else
            lineAngleRadians = -M_PI/2;
        }
    const double triangleAngle = (2 * M_PI) / arrowSize;
    // calc left point of symbol
    mLeftPos.set(static_cast<int>(sin(lineAngleRadians-triangleAngle) * arrowSize),
        static_cast<int>(cos(lineAngleRadians-triangleAngle) * arrowSize));
    // calc right point of symbol
    mRightPos.set(static_cast<int>(sin(lineAngleRadians+triangleAngle) * arrowSize),
        static_cast<int>(cos(lineAngleRadians+triangleAngle) * arrowSize));
    }


DiagramDrawer::~DiagramDrawer()
    {}

int DiagramDrawer::getPad(int div) const
    {
    int pad = static_cast<int>(getTextExtentHeight("W"));
    pad /= div;
    if(pad < 1)
        pad = 1;
    return pad;
    }

void DiagramDrawer::getConnectionPoints(GraphRect const &consumerRect,
        GraphRect const &supplierRect,
        GraphPoint &consumerPoint, GraphPoint &supplierPoint)
    {
    supplierPoint = supplierRect.getCenter();
    consumerPoint = consumerRect.getCenter();

    // If x positions overlap, then draw from top/bottom.
    if(consumerRect.isXOverlapped(supplierRect))
        {
        // Place connection at top or bottom
        if(supplierPoint.y > consumerPoint.y)
            {
            consumerPoint.y += consumerRect.size.y/2;
            supplierPoint.y -= supplierRect.size.y/2;
            }
        else
            {
            consumerPoint.y -= consumerRect.size.y/2;
            supplierPoint.y += supplierRect.size.y/2;
            }
        }
    else
        {
        if(supplierPoint.x > consumerPoint.x)
            {
            consumerPoint.x += consumerRect.size.x/2;
            supplierPoint.x -= supplierRect.size.x/2;
            }
        else
            {
            consumerPoint.x -= consumerRect.size.x/2;
            supplierPoint.x += supplierRect.size.x/2;
            }
        }
    }


void DiagramDependencyDrawer::drawArrowDependency(DiagramDrawer &drawer,
        GraphPoint &consumerPoint, GraphPoint &supplierPoint)
    {
    drawer.drawLine(supplierPoint, consumerPoint);
    DiagramArrow arrow(supplierPoint, consumerPoint, 10);
    drawer.drawLine(supplierPoint, supplierPoint + arrow.getLeftArrowPosition());
    drawer.drawLine(supplierPoint, supplierPoint + arrow.getRightArrowPosition());
    }

GraphRect DiagramDependencyDrawer::getNodeRect(DiagramDrawer &drawer,
        size_t nodeIndex) const
    {
    GraphPoint nodePos = getNodePosition(nodeIndex);
    OovString name = getNodeName(nodeIndex);
    int textWidth = static_cast<int>(drawer.getTextExtentWidth(name));
    // Make all heights the same to look uniform.
    float textHeight = drawer.getTextExtentHeight("Wy");
    /// @todo - fix pad here and in drawNodeText
    int pad2 = static_cast<int>((textHeight / 7.f) * 4);
    return GraphRect(nodePos.x, nodePos.y, textWidth + pad2,
        static_cast<int>(textHeight) + pad2);
    }

size_t DiagramDependencyDrawer::getNodeIndex(DiagramDrawer &drawer, GraphPoint p,
        size_t numNodes) const
    {
    size_t nodeIndex = NO_INDEX;
    for(size_t i=0; i<numNodes; i++)
        {
        GraphRect rect = getNodeRect(drawer, i);
        if(rect.isPointIn(p))
            {
            nodeIndex = i;
            break;
            }
        }
    return nodeIndex;
    }

std::vector<int> DiagramDependencyDrawer::getColumnPositions(
        DiagramDrawer &drawer, std::vector<size_t> const &depths) const
    {
    // Add 1 because operations start at depth of column 1.
    // Add 1 so there is a spacing column one beyond the last column.
    std::vector<int> columnSpacing(depths.size() + 2);
    int pad = static_cast<int>(drawer.getTextExtentWidth("W"));
    // first get the width required for each column.
    for(size_t ni=0; ni<depths.size(); ni++)
        {
        size_t columnIndex = depths[ni];
        int &columnWidth = columnSpacing[columnIndex];
        int nodeWidth = static_cast<int>(getNodeRect(drawer, ni).size.x + pad * 5);
        if(nodeWidth > columnWidth)
            {
            columnWidth = nodeWidth;
            }
        }
    // Now convert to positions.
    int pos = 0;
    for(size_t ci=0; ci<columnSpacing.size(); ci++)
        {
        int space = columnSpacing[ci];
        columnSpacing[ci] = pos;
        pos += space;
        }
    return columnSpacing;
    }


// Just use Keneth Kelly's colors
// http://stackoverflow.com/questions/470690/how-to-automatically-generate-n-distinct-colors
// @param index 0 : MaxColors
size_t DistinctColors::getNumColors()
    {
    return 19;
    }

Color DistinctColors::getColor(size_t index)
    {
    static int colors[] =
        {
        0xFFB300, // Vivid Yellow
        0x803E75, // Strong Purple
        0xFF6800, // Vivid Orange
        0xA6BDD7, // Very Light Blue
// RESERVED     0xC10020, // Vivid Red
        0xCEA262, // Grayish Yellow
        0x817066, // Medium Gray

        // Not good for color blind people
        0x007D34, // Vivid Green
        0xF6768E, // Strong Purplish Pink
        0x00538A, // Strong Blue
        0xFF7A5C, // Strong Yellowish Pink
        0x53377A, // Strong Violet
        0xFF8E00, // Vivid Orange Yellow
        0xB32851, // Strong Purplish Red
        0xF4C800, // Vivid Greenish Yellow
        0x7F180D, // Strong Reddish Brown
        0x93AA00, // Vivid Yellowish Green
        0x593315, // Deep Yellowish Brown
        0xF13A13, // Vivid Reddish Orange
        0x232C16, // Dark Olive Green
        };
    return(Color(static_cast<uint8_t>(colors[index]>>16),
        static_cast<uint8_t>((colors[index]>>8) & 0xFF),
        static_cast<uint8_t>((colors[index]) & 0xFF)));
    }


static const int NumGenerations = 30;

void DiagramDependencyGenes::initialize(DiagramDependencyDrawer &drawer, size_t nodeHeight)
    {
    mDrawer = &drawer;
    mNodeHeight = nodeHeight;
    size_t numNodes = drawer.getNumNodes();
    size_t numGenes = 0;
    if(numNodes > 100)  // Too many nodes to draw well, so make fewer genes for speed
        numGenes = static_cast<size_t>(sqrt(numNodes)) * 10;
    else if(numNodes > 50)      // Too many nodes to draw well, so make fewer genes for speed
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

size_t DiagramDependencyGenes::getMaxNodesInColumn() const
    {
    size_t numNodes = mDrawer->getNumNodes();
    std::map<int, size_t> columnCounts;     // First is pos, second is count.
    for(size_t i=0; i<numNodes; i++)
        {
        columnCounts[mDrawer->getNodePosition(i).x]++;
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

void DiagramDependencyGenes::updatePositionsInDrawer()
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
        GraphPoint pos = mDrawer->getNodePosition(i);
        pos.y = getYPosition(bestGeneI, i);
        mDrawer->setNodePosition(i, pos);
        }
    }

void DiagramDependencyGenes::setupQualityEachGeneration()
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

QualityType DiagramDependencyGenes::calculateSingleGeneQuality(size_t geneIndex) const
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

size_t DiagramDependencyGenes::getNodeYDistances(size_t geneIndex) const
    {
    size_t distance = 0;
    for(size_t ci=0; ci<mDrawer->getNumConnections(); ci++)
        {
        size_t consumerIndex;
        size_t supplierIndex;
        mDrawer->getConnection(ci, consumerIndex, supplierIndex);
        distance += static_cast<size_t>(
                abs((getYPosition(geneIndex, consumerIndex) -
                getYPosition(geneIndex, supplierIndex))));
        }
    return distance;
    }

size_t DiagramDependencyGenes::getNodeOverlapCount(size_t geneIndex) const
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
bool DiagramDependencyGenes::nodesOverlap(size_t geneIndex, size_t node1, size_t node2) const
    {
    // Add some space between nodes
    size_t nodeHeight = mNodeHeight * 2;

    int p1Y = getYPosition(geneIndex, node1);
    int p2Y = getYPosition(geneIndex, node2);
    // X positions are in columns, so exact match works to check for overlap.
    bool overlap = ((static_cast<size_t>(abs(p1Y-p2Y)) < nodeHeight) &&
            (mDrawer->getNodePosition(node1).x == mDrawer->getNodePosition(node2).x));
    return overlap;
    }

GeneValue DiagramDependencyGenes::getYPosition(size_t geneIndex, size_t nodeIndex) const
    {
    return getValue(geneIndex, nodeIndex*sizeof(GeneValue));
    }

void DiagramDependencyGenes::setYPosition(size_t geneIndex, size_t nodeIndex, GeneValue val)
    {
    return setValue(geneIndex, nodeIndex*sizeof(GeneValue), val);
    }

size_t DiagramDependencyGenes::getDrawingHeight(size_t geneIndex) const
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

