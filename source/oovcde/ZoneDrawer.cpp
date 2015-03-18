/*
 * ZoneDiagram.cpp
 * Created on: Feb 9, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ZoneDrawer.h"
#include "FilePath.h"
#include "Project.h"
#include <math.h>	// for atan
#ifndef __linux__
#define M_PI 3.14159265
#endif


int ZoneComponentChanges::getComponentIndex(int classIndex)
    {
    int moduleIndex = size()-1;
    for(size_t i=0; i<size(); i++)
	{
	if((*this)[i] > classIndex)
	    {
	    moduleIndex = i-1;
	    break;
	    }
	}
    return moduleIndex;
    }

std::string ZoneDrawer::getNodeComponentText(size_t nodeIndex) const
    {
    return mGraph->getNodes()[nodeIndex].getMappedComponentName(
	    mGraph->getPathMap()).getStr();
//    return ZoneGraph::getComponentText(mGraph->getNodes()[nodeIndex].mType->getClass()->getModule());
    }

int ZoneDrawer::getNodeRadius() const
    {
    int size = mScale/3;
    if(size < 1)
	size = 1;
    if(size > mCharWidth * 25)
	size = mCharWidth * 25;
    return size;
    }

static double getAngleInDegrees(GraphPoint p1, GraphPoint p2)
    {
    int difx = p1.x - p2.x;
    int dify = p1.y - p2.y;
    return (atan2(-difx, dify) * 180.0 / M_PI) + 180;
    }

ZoneNode const *ZoneDrawer::getZoneNode(GraphPoint screenClickPos) const
    {
    ZoneNode const *node = nullptr;
    int numNodes = mGraph->getNodes().size();
    int radius = getCircleRadius(numNodes);
    GraphPoint centerCircle;
    centerCircle.x = getCircleCenterPosX(radius);
    centerCircle.y = getCircleCenterPosY(radius);
    double angleInDegrees = getAngleInDegrees(screenClickPos, centerCircle);
    // The angle is between the nth node, and nth+1 node, so it needs to be
    // adjusted so the user can click from the nth-.5 and nth+.5
    if(numNodes > 0)
	{
	angleInDegrees += ((360.0 / numNodes) / 2);
	if(angleInDegrees > 360)
	    angleInDegrees -= 360;
	}
    /// @todo
    int nodeIndex = (angleInDegrees / 360) * numNodes;
    if(nodeIndex >= 0 && nodeIndex < numNodes)
	node = &mGraph->getNodes()[nodeIndex];
    return node;
    }


GraphPoint ZoneDrawer::getNodePosition(double nodeIndex, int radiusXOffset,
	int radiusYOffset, RelPositions relPos) const
    {
//    double angleDegrees = (nodeIndex * 360.0) / mGraph->getNodes().size();
//    double angleRadians = angleDegrees * M_PI / 180;
    int numNodes = mGraph->getNodes().size();
    double angleRadians = nodeIndex * M_PI / numNodes * 2;
    float radius = getCircleRadius(numNodes);
    // Put index 0 at the top(y) and move clockwise
    int x = (radius + radiusXOffset) * sin(angleRadians) + getCircleCenterPosX(radius);
    int y = -((radius + radiusYOffset) * cos(angleRadians)) + getCircleCenterPosY(radius);
    if(relPos == RL_TopLeftPos)
	{
	int nodeRadius = getNodeRadius();
	x -= nodeRadius;
	y -= nodeRadius;
	}
    return GraphPoint(x, y);
    }

ZoneComponentChanges ZoneDrawer::getComponentChanges() const
    {
    ZoneComponentChanges moduleChanges;
    FilePath lastModule("", FP_File);
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
//	FilePath module(getNodeComponentText(i), FP_File);
	FilePath module(mGraph->getNodes()[i].getMappedComponentName(mGraph->getPathMap()), FP_File);
	if(module != lastModule)
	    {
	    moduleChanges.push_back(i);
	    lastModule = module;
	    }
	}
    return moduleChanges;
    }

GraphRect ZoneDrawer::getNodeRect(size_t nodeIndex)
    {
    GraphPoint pos = getNodePosition(nodeIndex);
    int radius = getNodeRadius();
    int diameter = radius * 2;
    return GraphRect(pos.x-radius, pos.y-radius, diameter, diameter);
    }

GraphRect ZoneDrawer::getComponentTextRect(double nodeIndex) const
    {
    OovString modName = getNodeComponentText(nodeIndex);
    int offset = getNodeRadius() + mCharWidth;
    if(mDrawNodeText)
	{
	offset += mCharWidth * 5;
	}
    if(offset < mCharWidth * 3)
	offset = mCharWidth * 3;
    return getTextRect(nodeIndex, modName.getStr(), offset);
/*
    GraphPoint nodePos = getNodePosition(nodeIndex, offset, offset);
    int x = 0;
    bool leftSide = (nodeIndex > mGraph->getNodes().size()/2);
    int textWidth = mDrawer->getTextExtentWidth(modName);
    int textHeight = mDrawer->getTextExtentHeight(modName);
    if(leftSide)
	x = nodePos.x - textWidth;
    else
	x = nodePos.x;
    return GraphRect(x, nodePos.y, textWidth, textHeight);
*/
    }

GraphRect ZoneDrawer::getNodeTextRect(double nodeIndex) const
    {
    OovString name = mGraph->getNodes()[nodeIndex].mType->getName();
    return getTextRect(nodeIndex, name.getStr(), 0);
    }

GraphRect ZoneDrawer::getTextRect(double nodeIndex, const char *text, int offset) const
    {
    GraphPoint nodePos = getNodePosition(nodeIndex, offset, offset);
    int radius = getNodeRadius();
    int numNodes = mGraph->getNodes().size();
    bool leftSide = (nodeIndex > numNodes/2);

    // Linearily spread the nodes out near the top and bottom of the graph.
//    if(mDrawNodeText)
	{
	float halfNodes = numNodes / 2.0f;
	float quartNodes = halfNodes / 2;
	// linY ranges from -1 to 1, 0 is side, -1 is top, +1 is bottom.
	float halfIndex = nodeIndex;
	if(halfIndex >= halfNodes)
	    halfIndex = numNodes - nodeIndex;
	float linY = (halfIndex - quartNodes) / quartNodes;
	// Scale to screen pos.
	int circleRadius = getCircleRadius(numNodes);
	int screenLinY = linY * (circleRadius + (mCharWidth * 2));
	int circleY = nodePos.y - getCircleCenterPosY(circleRadius);
	if(abs(screenLinY) > abs(circleY))
	    {
	    nodePos.y = screenLinY + getCircleCenterPosY(circleRadius);
	    }
	}

    int textWidth = mDrawer->getTextExtentWidth(text);
    int textHeight = mDrawer->getTextExtentHeight(text);
    if(leftSide)
	nodePos.x -= textWidth + mCharWidth/2 - radius;
    else
	nodePos.x += mCharWidth/2 - radius;
    return GraphRect(nodePos.x, nodePos.y, textWidth, textHeight);
    }

GraphRect ZoneDrawer::getGraphObjectsSize()
    {
    GraphRect graphRect;
    for(size_t i=0; i<mComponentChanges.size(); i++)
	{
	int nodeIndex = getNodeIndexFromComponentIndex(i);
	GraphRect textRect = getComponentTextRect(nodeIndex);
	graphRect.unionRect(textRect);
	}
    if(mDrawNodeText)
	{
	for(size_t i=0; i<mGraph->getNodes().size(); i++)
	    {
	    graphRect.unionRect(getNodeTextRect(i));
	    }
	}
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
	graphRect.unionRect(getNodeRect(i));
	}
    return graphRect;
    }

void ZoneDrawer::updateGraph(ZoneGraph const &graph)
    {
    mGraph = &graph;
    mComponentChanges = getComponentChanges();
    mTopLeftCircleOffset.x = 0;
    mTopLeftCircleOffset.y = 0;
    if(mGraph->getNodes().size() > 0)
	{
	float radius = mGraph->getNodes().size() / M_PI;
	mScale = (600 / (radius * 2)) * mZoom;	/// @todo - get screen size?
	}
    GraphRect graphRect = getGraphObjectsSize();
    mDrawNodeText = (getNodeRadius()*3 > mDrawer->getTextExtentHeight("W"));

    // Add margins to the graph
    const int margin = 20;
    mTopLeftCircleOffset.x = -(graphRect.start.x - margin);
    mTopLeftCircleOffset.y = -(graphRect.start.y - margin);
    mGraphSize.x = graphRect.size.x + mTopLeftCircleOffset.x + margin;
    mGraphSize.y = graphRect.size.y + mTopLeftCircleOffset.y + margin;
    }

GraphSize ZoneDrawer::getGraphSize() const
    {
    return mGraphSize;
    }

void ZoneDrawer::drawGraph(ZoneDrawOptions const &drawOptions)
    {
    if(mDrawer && mGraph)
	{
	// This must be set for svg
	mDrawer->setDiagramSize(getGraphSize());

	mDrawer->groupShapes(true, Color(0,0,0), Color(245,245,255));
	drawComponentBarriers();
	mDrawer->groupShapes(false, 0, 0);
	drawConnections(drawOptions);
	mDrawer->groupShapes(true, Color(0,0,0), Color(245,245,255));
	if(drawOptions.mDrawAllClasses)
	    {
	    for(size_t i=0; i<mGraph->getNodes().size(); i++)
		{
		drawNode(i);
		}
	    }
	else
	    {
	    for(size_t i=0; i<mComponentChanges.size(); i++)
		{
		drawNode(getNodeIndexFromComponentIndex(i));
		}
	    }
	mDrawer->groupShapes(false, 0, 0);
	mDrawer->groupText(true);
	drawComponentText();
	if(mDrawNodeText && drawOptions.mDrawAllClasses)
	    {
	    drawNodeText();
	    }
	mDrawer->groupText(false);
	}
    }

void ZoneDrawer::drawNode(size_t nodeIndex)
    {
    GraphPoint pos = getNodePosition(nodeIndex);
    int size = getNodeRadius();
    mDrawer->drawCircle(GraphPoint(pos.x, pos.y), size, Color(255,255,255));
    }

void ZoneDrawer::drawComponentBarriers()
    {
    for(auto const &changeIndex : mComponentChanges)
	{
	GraphPoint p1 = getNodePosition(changeIndex - .5, 5, 5);
	int size = getNodeRadius() + 25;
	GraphPoint p2 = getNodePosition(changeIndex - .5, size, size);
	mDrawer->drawLine(p1, p2, false);
	}
    }

size_t ZoneDrawer::getNodeIndexFromComponentIndex(size_t moduleIndex)
    {
    size_t nodeIndex2 = mGraph->getNodes().size() - 1;
    if(moduleIndex < mComponentChanges.size()-1)
	nodeIndex2 = mComponentChanges[moduleIndex+1];
    return (mComponentChanges[moduleIndex] + nodeIndex2) / 2;
    }

void ZoneDrawer::drawComponentText()
    {
    double origFontSize = mDrawer->getFontSize();
    mDrawer->setFontSize(origFontSize * 1.2);
    for(size_t i=0; i<mComponentChanges.size(); i++)
	{
	size_t nodeIndex = getNodeIndexFromComponentIndex(i);
	mDrawer->drawText(getComponentTextRect(nodeIndex).start,
		getNodeComponentText(nodeIndex));
	}
    mDrawer->setFontSize(origFontSize);
    }

void ZoneDrawer::drawNodeText()
    {
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
	{
	GraphRect rect = getNodeTextRect(i);
	mDrawer->drawText(rect.start, mGraph->getNodes()[i].mType->getName());
	}
    }

// Just use Kelly's colors
// http://stackoverflow.com/questions/470690/how-to-automatically-generate-n-distinct-colors
// @param index 0 : MaxColors
static const int MaxColors = 19;
static Color getColor(int index)
    {
    static int colors[] =
	{
	0xFFB300, // Vivid Yellow
	0x803E75, // Strong Purple
	0xFF6800, // Vivid Orange
	0xA6BDD7, // Very Light Blue
// RESERVED	0xC10020, // Vivid Red
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
    return(Color(colors[index]>>16, (colors[index]>>8) & 0xFF, (colors[index]) & 0xFF));
    }

std::vector<ZoneConnectIndices> ZoneDrawer::getSortedConnections(
	ZoneConnections const &connections) const
    {
    // Convert all connections to go one direction (second is client).
    size_t count = connections.size();
    std::vector<ZoneConnectIndices> sortedCons(count);
    int sortedIndex = 0;
    for(auto const &item : connections)
	{
	ZoneConnectIndices ind = item.first;
	if(item.second == ZDD_FirstIsClient)
	    {
	    int tempi = ind.mFirstIndex;
	    ind.mFirstIndex = ind.mSecondIndex;
	    ind.mSecondIndex = tempi;
	    sortedCons[sortedIndex++] = ind;
	    }
	else if(item.second == ZDD_SecondIsClient)
	    {
	    sortedCons[sortedIndex++] = ind;
	    }
	}
    sortedCons.resize(sortedIndex);
    std::sort(sortedCons.begin(), sortedCons.end(), []
        (ZoneConnectIndices const &item1, ZoneConnectIndices const &item2)
		{ return(item1.mFirstIndex < item2.mFirstIndex); });
    return sortedCons;
    }

void ZoneDrawer::drawSortedConnections(std::vector<ZoneConnectIndices> const &sortedCons,
	ZoneDrawOptions const &drawOptions)
    {
    int lastCompIndex = -1;
    bool hadLines = false;
    for(auto const &item : sortedCons)
	{
	int compIndex = 0;
	if(drawOptions.mDrawAllClasses)
	    compIndex = mComponentChanges.getComponentIndex(item.mFirstIndex);
	else
	    compIndex = item.mFirstIndex;
	if(compIndex != lastCompIndex)
	    {
	    if(lastCompIndex != -1)
		{
		mDrawer->groupShapes(false, 0, 0);
		hadLines = true;
		}
	    Color lineColor = getColor(compIndex % MaxColors);
	    mDrawer->groupShapes(true, lineColor, Color(245,245,255));
	    lastCompIndex = compIndex;
	    }
	int nodeIndex1 = item.mFirstIndex;
	int nodeIndex2 = item.mSecondIndex;
	if(!drawOptions.mDrawAllClasses)
	    {
	    nodeIndex1 = getNodeIndexFromComponentIndex(item.mFirstIndex);
	    nodeIndex2 = getNodeIndexFromComponentIndex(item.mSecondIndex);
	    }
	drawConnection(nodeIndex1, nodeIndex2, ZDD_SecondIsClient, drawOptions);
	}
    if(hadLines)
	{
	mDrawer->groupShapes(false, 0, 0);
	}
    mDrawer->groupShapes(true, Color(0xc1,0,0x20), Color(245,245,255));
    for(auto const &item : mGraph->getConnections())
	{
	if(item.second == ZDD_Bidirectional)
	    {
	    drawConnection(item.first.mFirstIndex, item.first.mSecondIndex,
		    item.second, drawOptions);
	    }
	}
    mDrawer->groupShapes(false, 0, 0);
    }

void ZoneDrawer::drawConnections(ZoneDrawOptions const &drawOptions)
    {
    if(drawOptions.mDrawAllClasses)
	{
	std::vector<ZoneConnectIndices> sortedCons = getSortedConnections(
		mGraph->getConnections());
	drawSortedConnections(sortedCons, drawOptions);
	}
    else
	{
	ZoneConnections moduleConnections;
	for(auto const &item : mGraph->getConnections())
	    {
	    int m1 = mComponentChanges.getComponentIndex(item.first.mFirstIndex);
	    int m2 = mComponentChanges.getComponentIndex(item.first.mSecondIndex);
	    auto it = moduleConnections.find(ZoneConnectIndices(m1, m2));
	    if(it == moduleConnections.end())
		{
		moduleConnections.insertConnection(m1, m2, item.second);
		}
	    else
		{
		(*it).second = static_cast<eZoneDependencyDirections>(
			(*it).second | item.second);
		}
	    }
	std::vector<ZoneConnectIndices> sortedCons = getSortedConnections(
		moduleConnections);
	drawSortedConnections(sortedCons, drawOptions);
	}
    }

static double slope(GraphPoint p1, GraphPoint p2)
{
    if(p1.x-p2.x != 0)
        return(static_cast<double>(p1.y-p2.y)/(p1.x-p2.x));
    else
        return 1.0e+9;
}

// From p1, find the intersect with the p2 node.
GraphPoint findIntersect(GraphPoint p1, GraphPoint p2, int nodeSize)
{
    GraphPoint retP;
    /// @todo this calc is wrong.
    int halfNodeSize = (nodeSize*1.5)/2;
    double slopeNodePoint = slope(p2, GraphPoint(p2.x+halfNodeSize, p2.y-halfNodeSize));
    double slopeLine = slope(p1, p2);
    bool leftOfRect = (p1.x < p2.x);
    bool aboveRect = (p1.y < p2.y);
    if(fabs(slopeLine) > fabs(slopeNodePoint))
        {
	// Mainly vertical lines
        retP.y = aboveRect ? p2.y-halfNodeSize : p2.y+halfNodeSize;
        int xoff = (float)slopeNodePoint/slopeLine*halfNodeSize;
        retP.x = aboveRect ? p2.x+xoff : p2.x-xoff;
        }
    else
        {
	// Mainly horizontal lines
	retP.x = leftOfRect ? p2.x-halfNodeSize : p2.x+halfNodeSize;
        int yoff = (float)slopeLine/slopeNodePoint*halfNodeSize;
        retP.y = leftOfRect ? p2.y+yoff : p2.y-yoff;
	}
    return retP;
}

void ZoneDrawer::drawConnection(int firstNodeIndex, int secondNodeIndex,
	eZoneDependencyDirections dir, ZoneDrawOptions const &drawOptions)
    {
    GraphPoint firstPoint = getNodePosition(firstNodeIndex);
    GraphPoint secondPoint = getNodePosition(secondNodeIndex);

    int size = getNodeRadius();
    secondPoint = findIntersect(firstPoint, secondPoint, size);
    firstPoint = findIntersect(secondPoint, firstPoint, size);

    mDrawer->drawLine(firstPoint, secondPoint, false);
    if(drawOptions.mDrawDependencies)
	{
	if(dir == ZDD_Bidirectional)
	    {
	    drawConnectionArrow(firstPoint, secondPoint, ZDD_FirstIsClient);
	    drawConnectionArrow(secondPoint, firstPoint, ZDD_FirstIsClient);
	    }
	else
	    {
	    drawConnectionArrow(firstPoint, secondPoint, dir);
	    }
	}
    }

void ZoneDrawer::drawConnectionArrow(GraphPoint firstPoint, GraphPoint secondPoint,
	eZoneDependencyDirections dir)
    {
    GraphPoint producer;
    GraphPoint consumer;
    if(dir == ZDD_FirstIsClient)
	{
	consumer = firstPoint;
	producer = secondPoint;
	}
    else if(dir == ZDD_SecondIsClient)
	{
	producer = firstPoint;
	consumer = secondPoint;
	}
    int xdist = producer.x-consumer.x;
    int ydist = producer.y-consumer.y;
    double lineAngleRadians;
    if(ydist != 0)
	lineAngleRadians = atan2(xdist, ydist);
    else
	{
	if(producer.x > consumer.x)
	    lineAngleRadians = M_PI/2;
	else
	    lineAngleRadians = -M_PI/2;
	}
    int arrowtop = -12;
    const double triangleAngle = (.7 * M_PI) / arrowtop;
    GraphPoint p2;
    // calc left point of symbol
    p2.set(sin(lineAngleRadians-triangleAngle) * arrowtop,
	    cos(lineAngleRadians-triangleAngle) * arrowtop);
    mDrawer->drawLine(producer, p2+producer);
    // calc right point of symbol
    p2.set(sin(lineAngleRadians+triangleAngle) * arrowtop,
	cos(lineAngleRadians+triangleAngle) * arrowtop);
    mDrawer->drawLine(producer, p2+producer);
    }
