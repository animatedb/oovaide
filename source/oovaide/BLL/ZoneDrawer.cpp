/*
 * ZoneDiagram.cpp
 * Created on: Feb 9, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ZoneDrawer.h"
#include "FilePath.h"
#include "Project.h"
#include "Debug.h"
#include <math.h>       // for atan
#ifndef __linux__
#define M_PI 3.14159265
#endif


size_t ZoneComponentChanges::getComponentIndex(size_t classIndex) const
    {
    size_t moduleIndex = size()-1;
    for(size_t i=0; i<size(); i++)
        {
        if(at(i) > classIndex)
            {
            moduleIndex = i;
            break;
            }
        }
    return moduleIndex;
    }

size_t ZoneComponentChanges::getComponentChildClassIndex(size_t classIndex) const
    {
    size_t compIndex = getComponentIndex(classIndex);
    return(classIndex - getBaseComponentChildClassIndex(compIndex));
    }

size_t ZoneComponentChanges::getBaseComponentChildClassIndex(size_t componentIndex) const
    {
    size_t index = 0;
    if(componentIndex > 0)
        {
        if(componentIndex >= size())
            {
            DebugAssert(__FILE__, __LINE__);
            }
        index = at(componentIndex-1);
        }
    return index;
    }

size_t ZoneComponentChanges::getMiddleComponentChildClassIndex(
        size_t componentIndex) const
    {
    return(getBaseComponentChildClassIndex(componentIndex) +
            getNumClassesForComponent(componentIndex) / 2);
    }

size_t ZoneComponentChanges::getNumClassesForComponent(size_t componentIndex) const
    {
    size_t numNodes = 0;
    if(componentIndex > 0)
        {
        numNodes = at(componentIndex) - at(componentIndex-1);
        }
    else
        {
        numNodes = at(componentIndex);
        }
    return numNodes;
    }

std::string ZoneDrawer::getNodeComponentText(size_t nodeIndex) const
    {
    return mGraph->getNodes()[nodeIndex].getMappedComponentName(
            mGraph->getPathMap()).getStr();
//    return ZoneGraph::getComponentText(mGraph->getNodes()[nodeIndex].mType->getClass()->getModule());
    }

int ZoneDrawer::getNodeRadius(DiagramDrawer &drawer) const
    {
    int size = mScale/3;
    if(size < 1)
        size = 1;
    int charWidth = drawer.getPad();
    if(size > charWidth * 25)
        size = charWidth * 25;
    return size;
    }

/*
static double getAngleInDegrees(GraphPoint p1, GraphPoint p2)
    {
    int difx = p1.x - p2.x;
    int dify = p1.y - p2.y;
    return (atan2(-difx, dify) * 180.0 / M_PI) + 180;
    }
*/

ZoneNode const *ZoneDrawer::getZoneNode(GraphPoint userSelectedScreenPos) const
    {
    ZoneNode const *node = nullptr;
    if(mGraph)
        {
        size_t numNodes = mGraph->getNodes().size();
        // Cheat and make the distance somewhat based on the circle radius of all classes.
        int radius = getCircleRadius(numNodes);
        int distLimit = radius / 10;
        int minDist = distLimit * distLimit;
        int minIndex = -1;
        userSelectedScreenPos.sub(GraphDrawOffset);
        userSelectedScreenPos.x /= mZoom;
        userSelectedScreenPos.y /= mZoom;
        for(size_t i=0; i<numNodes; i++)
            {
            GraphPoint pos = mNodePositions[i];
            int distx = abs(pos.x - userSelectedScreenPos.x);
            int disty = abs(pos.y - userSelectedScreenPos.y);
            int dist = distx * distx + disty * disty ;
            if(dist < minDist)
                {
                minIndex = i;
                minDist = dist;
                }
            }
        if(minIndex != -1)
            {
            node = &mGraph->getNodes()[minIndex];
            }
        }
    return node;
    }

void ZoneDrawer::setPosition(ZoneNode const *node, GraphPoint origScreenPos,
        GraphPoint screenPos)
    {
    // Only allow movement in child circle mode
    if(mGraph->getDrawOptions().mDrawChildCircles)
        {
        if(node)
            {
            size_t nodeIndex = mGraph->getNodeIndex(node);
            GraphPoint screenOffset = screenPos;
            screenOffset.sub(origScreenPos);
            screenOffset.x /= mZoom;
            screenOffset.y /= mZoom;
            size_t compIndex = mComponentChanges.getComponentIndex(nodeIndex);
            size_t baseIndex = mComponentChanges.getBaseComponentChildClassIndex(compIndex);
            size_t numCompClasses = mComponentChanges.getNumClassesForComponent(compIndex);
            for(size_t ci=0; ci<numCompClasses; ci++)
                {
                size_t i = baseIndex + ci;
                GraphPoint newPos = mNodePositions[i];
                newPos.add(screenOffset);
                mNodePositions[i] = newPos;
                }
            }
        else
            {
            DebugAssert(__FILE__, __LINE__);
            }
        }
    }

GraphPoint ZoneDrawer::getCirclePos(float nodeIndex, int numNodes, int radiusOffset) const
    {
    double angleRadians = nodeIndex * M_PI / numNodes * 2;
    float radius = getCircleRadius(numNodes);
    // Put index 0 at the top(y) and move clockwise
    int x = (radius + radiusOffset) * sin(angleRadians);
    int y = -((radius + radiusOffset) * cos(angleRadians));
    return GraphPoint(x, y);
    }

GraphPoint ZoneDrawer::getUnzoomedMasterCirclePos(float nodeIndex, int radiusOffset) const
    {
    GraphPoint pos = getCirclePos(nodeIndex, mGraph->getNodes().size(), radiusOffset);
    float radius = getCircleRadius(mGraph->getNodes().size());
    pos.x += getMasterCircleCenterPosX(radius);
    pos.y += getMasterCircleCenterPosY(radius);
    return pos;
    }

GraphPoint ZoneDrawer::getMasterCirclePos(float nodeIndex, int radiusOffset) const
    {
    GraphPoint pos = getCirclePos(nodeIndex, mGraph->getNodes().size(), radiusOffset);
    float radius = getCircleRadius(mGraph->getNodes().size());
    pos.x += getMasterCircleCenterPosX(radius);
    pos.y += getMasterCircleCenterPosY(radius);
    pos.x *= mZoom;
    pos.y *= mZoom;
    return pos;
    }

GraphPoint ZoneDrawer::getChildNodeCircleOffset(double nodeIndex) const
    {
    int compIndex = mComponentChanges.getComponentIndex(nodeIndex);
    int numChildNodes = mComponentChanges.getNumClassesForComponent(compIndex);
    int childIndex = mComponentChanges.getComponentChildClassIndex(nodeIndex);
    return getCirclePos(childIndex, numChildNodes);
    }

void ZoneDrawer::updateNodePositions()
    {
    size_t numNodes = mGraph->getNodes().size();
    mNodePositions.resize(numNodes);
    GraphPoint pos;
    for(size_t nodeIndex=0; nodeIndex<numNodes; nodeIndex++)
        {
        int masterNodeIndex = 0;
        if(mGraph->getDrawOptions().mDrawChildCircles)
            {
            int compIndex = mComponentChanges.getComponentIndex(nodeIndex);
            masterNodeIndex = mComponentChanges.getMiddleComponentChildClassIndex(compIndex);
            }
        else
            {
            masterNodeIndex = nodeIndex;
            }
        // For child circles, this is the same for the whole child circle.
        pos = getUnzoomedMasterCirclePos(masterNodeIndex);
        if(mGraph->getDrawOptions().mDrawChildCircles)
            {
            GraphPoint offset = getChildNodeCircleOffset(nodeIndex);
            pos.add(offset);
            }
        mNodePositions[nodeIndex] = pos;
        }
    }

GraphPoint ZoneDrawer::getNodePosition(DiagramDrawer &drawer, double nodeIndex,
        RelPositions relPos) const
    {
    GraphPoint pos = mNodePositions[nodeIndex];
    if(relPos == RL_TopLeftPos)
        {
        int nodeRadius = getNodeRadius(drawer);
        pos.x -= nodeRadius;
        pos.y -= nodeRadius;
        }
    pos.x *= mZoom;
    pos.y *= mZoom;
    return pos;
    }

ZoneComponentChanges ZoneDrawer::getComponentChanges() const
    {
    ZoneComponentChanges moduleChanges;
    FilePath lastModule("", FP_File);
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
//      FilePath module(getNodeComponentText(i), FP_File);
        FilePath module(mGraph->getNodes()[i].getMappedComponentName(mGraph->getPathMap()), FP_File);
        if(i == 0)
            {
            lastModule = module;
            }
        if(module != lastModule)
            {
            moduleChanges.push_back(i);
            lastModule = module;
            }
        }
    moduleChanges.push_back(mGraph->getNodes().size());
    return moduleChanges;
    }

GraphRect ZoneDrawer::getNodeRect(DiagramDrawer &drawer, size_t nodeIndex)
    {
    GraphPoint pos = getNodePosition(drawer, nodeIndex);
    int radius = getNodeRadius(drawer);
    int diameter = radius * 2;
    return GraphRect(pos.x-radius, pos.y-radius, diameter, diameter);
    }

GraphRect ZoneDrawer::getComponentTextRect(DiagramDrawer &drawer,
        double nodeIndex) const
    {
    OovString modName = getNodeComponentText(nodeIndex);
    return getTextRect(drawer, nodeIndex, modName.getStr(), true);
    }

GraphRect ZoneDrawer::getNodeTextRect(DiagramDrawer &drawer,
        double nodeIndex) const
    {
    OovString name = mGraph->getNodes()[nodeIndex].mType->getName();
    return getTextRect(drawer, nodeIndex, name.getStr(), false);
    }

static float getCircleRadius(int numNodes, double scale)
    {
    float radius = numNodes / 2.0 / M_PI;
    return(radius * scale);
    }

float ZoneDrawer::getCircleRadius(int numNodes) const
    {
    return ::getCircleRadius(numNodes, mScale);
    }

// Return is y offset.
// Linearly spread the nodes out near the top and bottom of the circle.
static int spreadCircle(int nodeIndex, int numNodes, int proposedY, float scale, int charWidth)
    {
    int yOffset = 0;
    float halfNodes = numNodes / 2.0f;
    float quartNodes = halfNodes / 2;
    // linY ranges from -1 to 1, 0 is side, -1 is top, +1 is bottom.
    float halfIndex = nodeIndex;
    if(halfIndex >= halfNodes)
        halfIndex = numNodes - nodeIndex;
    float linY = (halfIndex - quartNodes) / quartNodes;
    // Scale to screen pos.
    int circleRadius = getCircleRadius(numNodes, scale);
    int addedSize = (charWidth * 2);
    int screenLinY = linY * (circleRadius + addedSize);
    int circleY = proposedY - circleRadius;
    if(abs(screenLinY) > abs(circleY))
        {
        yOffset = screenLinY - circleY;
        }
    return yOffset;
    }

GraphRect ZoneDrawer::getTextRect(DiagramDrawer &drawer, double nodeIndex,
        const char *text, bool component) const
    {
    GraphPoint nodePos = getNodePosition(drawer, nodeIndex);
    int charWidth = drawer.getPad();
    int textWidth = drawer.getTextExtentWidth(text);
    int textHeight = drawer.getTextExtentHeight(text);
    int numNodes = mGraph->getNodes().size();
    if(component)
        {
        if(mGraph->getDrawOptions().mDrawChildCircles)
            {
            // If drawing child circles, put the component text below the
            // child circle.
            nodePos.y += getNodeRadius(drawer) + charWidth * 2;
            }
        else
            {
            // Scale to move text farther from center
            nodePos = getMasterCirclePos(nodeIndex, charWidth * 5);
            }
        }
    else
        {
        // If the nodes are too small and close together, the node text
        // won't be drawn, and this code isn't called.
        //
        // If the nodes are too small for the text to fit inside, put the
        // text outside of the circle.
        int radius = getNodeRadius(drawer);
        if(radius < charWidth * 4)
            {
            // If drawing child circles, the text is placed to be left or right
            // around the child circle.
            int yOffset = 0;
            if(mGraph->getDrawOptions().mDrawChildCircles)
                {
                int compIndex = mComponentChanges.getComponentIndex(nodeIndex);
                numNodes = mComponentChanges.getNumClassesForComponent(compIndex);
                nodeIndex = mComponentChanges.getComponentChildClassIndex(nodeIndex);
//              int proposedY = getCirclePos(nodeIndex, numNodes).y;
//              yOffset = spreadCircle(nodeIndex, numNodes, proposedY, mScale, mCharWidth);
                }
            else
                {
                yOffset = spreadCircle(nodeIndex, numNodes, nodePos.y, mScale, charWidth);
                nodePos.y += yOffset;
                }
            }
        }
    // On the right side of the circle, the text is moved a bit right
    // of the node, on the left side of the circle, the text is moved
    // so all of the test is left of the circle.
    bool moveLeft = (nodeIndex > numNodes/2);
    if(moveLeft)
        nodePos.x -= textWidth;
    return GraphRect(nodePos.x, nodePos.y, textWidth, textHeight);
    }

GraphRect ZoneDrawer::getGraphObjectsSize(DiagramDrawer &drawer)
    {
    GraphRect graphRect;
    for(size_t i=0; i<mComponentChanges.size(); i++)
        {
        int nodeIndex = getNodeIndexFromComponentIndex(i);
        GraphRect textRect = getComponentTextRect(drawer, nodeIndex);
        graphRect.unionRect(textRect);
        }
    if(mDrawNodeText)
        {
        for(size_t i=0; i<mGraph->getNodes().size(); i++)
            {
            graphRect.unionRect(getNodeTextRect(drawer, i));
            }
        }
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
        graphRect.unionRect(getNodeRect(drawer, i));
        }
    return graphRect;
    }

void ZoneDrawer::updateGraph(DiagramDrawer &drawer, ZoneGraph const &graph,
        bool resetPositions)
    {
    mGraph = &graph;
    if(mGraph->getNodes().size() > 0)
        {
        mComponentChanges = getComponentChanges();
        float radius = mGraph->getNodes().size() / M_PI;
        mScale = (600 / (radius * 2));  /// @todo - get screen size?
        if(resetPositions)
            {
            updateNodePositions();
            }
        }
    GraphRect graphRect = getGraphObjectsSize(drawer);
    mDrawNodeText = (getNodeRadius(drawer)*3 > drawer.getTextExtentHeight("W"));

    // Add margins to the graph
    const int margin = 20;
    GraphDrawOffset.x = -(graphRect.start.x - margin);
    GraphDrawOffset.y = -(graphRect.start.y - margin);
    mDrawingSize.x = graphRect.size.x + GraphDrawOffset.x + margin;
    mDrawingSize.y = graphRect.size.y + GraphDrawOffset.y + margin;
    }

GraphSize ZoneDrawer::getDrawingSize() const
    {
    return mDrawingSize;
    }

void ZoneDrawer::drawGraph(DiagramDrawer &drawer)
    {
    if(mGraph)
        {
        // This must be set for svg
        drawer.setDiagramSize(getDrawingSize());
        drawer.setCurrentDrawingFontSize(mDiagram.getDiagramBaseFontSize());

        drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        drawComponentBarriers(drawer);
        drawer.groupShapes(false, 0, 0);
        drawConnections(drawer);
        drawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        if(mGraph->getDrawOptions().mDrawAllClasses)
            {
            for(size_t i=0; i<mGraph->getNodes().size(); i++)
                {
                drawNode(drawer, i);
                }
            }
        else
            {
            for(size_t i=0; i<mComponentChanges.size(); i++)
                {
                drawNode(drawer, getNodeIndexFromComponentIndex(i));
                }
            }
        drawer.groupShapes(false, 0, 0);
        drawer.groupText(true, false);
        drawComponentText(drawer);
        if(mDrawNodeText && mGraph->getDrawOptions().mDrawAllClasses)
            {
            drawNodeText(drawer);
            }
        drawer.groupText(false, false);
        }
    }

void ZoneDrawer::drawNode(DiagramDrawer &drawer, size_t nodeIndex)
    {
    GraphPoint pos = getNodePosition(drawer, nodeIndex);
    pos.add(GraphDrawOffset);
    int size = getNodeRadius(drawer);
    drawer.drawCircle(GraphPoint(pos.x, pos.y), size, Color(255,255,255));
    }

void ZoneDrawer::drawComponentBarriers(DiagramDrawer &drawer)
    {
    if(!mGraph->getDrawOptions().mDrawChildCircles)
        {
        for(auto const &changeIndex : mComponentChanges)
            {
//          GraphPoint p1 = getNodePosition(drawer, changeIndex - .5, 5, 5);
            GraphPoint p1 = getMasterCirclePos(changeIndex - .5, 5);
            int size = getNodeRadius(drawer) + 25;
//          GraphPoint p2 = getNodePosition(drawer, changeIndex - .5, size, size);
            GraphPoint p2 = getMasterCirclePos(changeIndex - .5, size);
            p1.add(GraphDrawOffset);
            p2.add(GraphDrawOffset);
            drawer.drawLine(p1, p2, false);
            }
        }
    }

size_t ZoneDrawer::getNodeIndexFromComponentIndex(size_t componentIndex)
    {
    return mComponentChanges.getMiddleComponentChildClassIndex(componentIndex);
    }

void ZoneDrawer::drawComponentText(DiagramDrawer &drawer)
    {
    for(size_t i=0; i<mComponentChanges.size(); i++)
        {
        size_t nodeIndex = getNodeIndexFromComponentIndex(i);
        GraphPoint textPos = getComponentTextRect(drawer, nodeIndex).start;
        textPos.add(GraphDrawOffset);
        drawer.drawText(textPos, getNodeComponentText(nodeIndex));
        }
    }

void ZoneDrawer::drawNodeText(DiagramDrawer &drawer)
    {
    for(size_t i=0; i<mGraph->getNodes().size(); i++)
        {
        GraphRect rect = getNodeTextRect(drawer, i);
        rect.start.add(GraphDrawOffset);
        drawer.drawText(rect.start, mGraph->getNodes()[i].mType->getName());
        }
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

void ZoneDrawer::drawSortedConnections(DiagramDrawer &drawer,
        std::vector<ZoneConnectIndices> const &sortedCons)
    {
    int lastCompIndex = -1;
    bool hadLines = false;
    ZoneDrawOptions const &drawOptions = mGraph->getDrawOptions();
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
                drawer.groupShapes(false, 0, 0);
                hadLines = true;
                }
            Color lineColor = DistinctColors::getColor(compIndex % DistinctColors::getNumColors());
            drawer.groupShapes(true, lineColor, Color(245,245,255));
            lastCompIndex = compIndex;
            }
        int nodeIndex1 = item.mFirstIndex;
        int nodeIndex2 = item.mSecondIndex;
        if(!drawOptions.mDrawAllClasses)
            {
            nodeIndex1 = getNodeIndexFromComponentIndex(item.mFirstIndex);
            nodeIndex2 = getNodeIndexFromComponentIndex(item.mSecondIndex);
            }
        drawConnection(drawer, nodeIndex1, nodeIndex2, ZDD_SecondIsClient);
        }
    if(hadLines)
        {
        drawer.groupShapes(false, 0, 0);
        }
    drawer.groupShapes(true, Color(0xc1,0,0x20), Color(245,245,255));
    for(auto const &item : mGraph->getConnections())
        {
        if(item.second == ZDD_Bidirectional)
            {
            drawConnection(drawer, item.first.mFirstIndex, item.first.mSecondIndex,
                    item.second);
            }
        }
    drawer.groupShapes(false, 0, 0);
    }

void ZoneDrawer::drawConnections(DiagramDrawer &drawer)
    {
    if(mGraph->getDrawOptions().mDrawAllClasses)
        {
        std::vector<ZoneConnectIndices> sortedCons = getSortedConnections(
                mGraph->getConnections());
        drawSortedConnections(drawer, sortedCons);
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
        drawSortedConnections(drawer, sortedCons);
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

void ZoneDrawer::drawConnection(DiagramDrawer &drawer,
        int firstNodeIndex, int secondNodeIndex,
        eZoneDependencyDirections dir)
    {
    GraphPoint firstPoint = getNodePosition(drawer, firstNodeIndex);
    GraphPoint secondPoint = getNodePosition(drawer, secondNodeIndex);
    firstPoint.add(GraphDrawOffset);
    secondPoint.add(GraphDrawOffset);

    int size = getNodeRadius(drawer);
    secondPoint = findIntersect(firstPoint, secondPoint, size);
    firstPoint = findIntersect(secondPoint, firstPoint, size);

    drawer.drawLine(firstPoint, secondPoint, false);
    if(mGraph->getDrawOptions().mDrawDependencies)
        {
        if(dir == ZDD_Bidirectional)
            {
            drawConnectionArrow(drawer, firstPoint, secondPoint, ZDD_FirstIsClient);
            drawConnectionArrow(drawer, secondPoint, firstPoint, ZDD_FirstIsClient);
            }
        else
            {
            drawConnectionArrow(drawer, firstPoint, secondPoint, dir);
            }
        }
    }

void ZoneDrawer::drawConnectionArrow(DiagramDrawer &drawer, GraphPoint firstPoint,
        GraphPoint secondPoint, eZoneDependencyDirections dir)
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

    DiagramArrow arrow(producer, consumer, 12);
    drawer.drawLine(producer, producer + arrow.getLeftArrowPosition());
    drawer.drawLine(producer, producer + arrow.getRightArrowPosition());
    }
