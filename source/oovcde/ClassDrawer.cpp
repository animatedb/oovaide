/*
 * ClassDrawer.cpp
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassDrawer.h"
#include <algorithm>	// For sort
#include "FilePath.h"

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

static void getLeafPath(std::string &moduleStr)
    {
    size_t pos = rfindPathSep(moduleStr.c_str());
    if(pos != std::string::npos)
	{
	moduleStr.resize(pos+1);
	pos = rfindPathSep(moduleStr.c_str(), pos-1);
	if(pos != std::string::npos)
	    {
	    moduleStr.erase(0, pos);
	    }
	}
    }

void ClassDrawer::getStrings(const ClassNode &node,
	std::vector<std::string> &nodeStrs, std::vector<std::string> &attrStrs,
	std::vector<std::string> &operStrs)
    {
    const ModelType *type = node.getType();
    char const * const typeName = type->getName().c_str();
    nodeStrs.push_back(typeName);
    if(node.getDrawOptions().drawPackageName)
	{
	const ModelClassifier *cls = ModelObject::getClass(node.getType());
	if(cls)
	    {
	    const ModelModule *module = cls->getModule();
	    if(module)
		{
		std::string moduleStr =	module->getModulePath();
		getLeafPath(moduleStr);
		nodeStrs.push_back(moduleStr);
		}
	    }
	}
    const ModelClassifier *classifier = ModelObject::getClass(type);
    if(classifier)
	{
	if(node.getDrawOptions().drawAttributes)
	    {
	    for(const auto &attr : classifier->getAttributes())
		{
		std::string name = attr->getAccess().asUmlStr();
		if(node.getDrawOptions().drawAttrTypes)
		    {
		    const ModelType *attp = attr->getDeclType();
		    if(attp)
			{
			name += attp->getName();
			name += " ";
			}
		    }
		name += attr->getName();
		attrStrs.push_back(name);
		}
	    }

	if(node.getDrawOptions().drawOperations)
	    {
	    for(const auto &oper : classifier->getOperations())
		{
		std::string operStr = oper->getAccess().asUmlStr();
		operStr += oper->getName().c_str();
		operStr += "(";
		if(node.getDrawOptions().drawOperParams)
		    {
		    bool firstParam = true;
		    for(const auto &opparam : oper->getParams())
			{
			if(firstParam)
			    firstParam = false;
			else
			    operStr += ", ";
			if(node.getDrawOptions().drawOperTypes)
			    {
			    const ModelType *optp = opparam->getDeclType();
			    if(optp)
				{
				operStr += optp->getName();
				operStr += " ";
				}
			    }
			operStr += opparam->getName();
			}
		    }
		operStr += ")";
		operStrs.push_back(operStr);
		}
	    }
	}
    }

void ClassDrawer::splitStrings(std::vector<std::string> &nodeStrs,
	std::vector<std::string> &attrStrs, std::vector<std::string> &operStrs)
    {
    std::vector<size_t> lengths;
    for(auto const &str : nodeStrs)
	{
	lengths.push_back(str.length());
	}
    for(auto const &str : attrStrs)
	{
	lengths.push_back(str.length());
	}
    for(auto const &str : operStrs)
	{
	lengths.push_back(str.length());
	}
    std::sort(lengths.begin(), lengths.end());
    float biggestRatio = 0;
    size_t biggestRatioIndex = 0;
    size_t maxLength = lengths[lengths.size()-1];
    for(size_t i=lengths.size()/2; i<lengths.size()-1; i++)
	{
	float ratio = (maxLength - lengths[i]) / (lengths.size() - i);
	if(ratio > biggestRatio)
	    {
	    biggestRatio = ratio;
	    biggestRatioIndex = i;
	    }
	}
    size_t biggestRatioLength = lengths[biggestRatioIndex];
    if(biggestRatio > 8)
	{
	// Split into a maximum of two lines.
	size_t minSplitLength = maxLength / 2;
	if(biggestRatioLength < minSplitLength)
	    biggestRatioLength = minSplitLength;
	for(size_t i=0; i<operStrs.size(); i++)
	    {
	    if(operStrs[i].length() > biggestRatioLength)
		{
		size_t pos = operStrs[i].find(' ', biggestRatioLength);
		if(pos == std::string::npos)
		    pos = biggestRatioLength;
		std::string temp = "   " + operStrs[i].substr(pos);
		operStrs[i].resize(pos);
		operStrs.insert(operStrs.begin()+i+1, temp);
		i++;	// Don't split next line.
		}
	    }
	}
    }

GraphSize ClassDrawer::drawNode(const ClassNode &node)
    {
    GraphPoint startpos = node.getPosition();
    int height = mDrawer.getTextExtentHeight("W");
    int pad = height / 10;
    if(pad < 1)
	pad = 1;
    int pad2 = pad*2;
    int padLine = pad*3;
    int line1 = startpos.y;
    int line2 = startpos.y;
    std::vector<DrawString> drawStrings;
    std::vector<std::string> nodeStrs;
    std::vector<std::string> attrStrs;
    std::vector<std::string> operStrs;

    getStrings(node, nodeStrs, attrStrs, operStrs);
    splitStrings(nodeStrs, attrStrs, operStrs);
    int y = startpos.y;
    for(auto const &str : nodeStrs)
	{
	y += height + pad2;
	drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str.c_str()));
	}
    y += padLine;	// Space for line.
    line1 = y;
    for(auto const &str : attrStrs)
	{
	y += height + pad2;
	drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str.c_str()));
	}
    y += padLine;	// Space for line.
    line2 = y;
    for(auto const &str : operStrs)
	{
	y += height + pad2;
	drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str.c_str()));
	}

    int maxWidth = 0;
    for(const auto &dstr : drawStrings)
	{
	int width = mDrawer.getTextExtentWidth(dstr.str.c_str());
	if(width > maxWidth)
	    maxWidth = width;
	}
    maxWidth += pad2;
    y += pad2;
    mDrawer.groupShapes(true, Color(245,245,255));
    mDrawer.drawRect(GraphRect(startpos.x, startpos.y, maxWidth, y-startpos.y));
    mDrawer.drawLine(GraphPoint(startpos.x, line1), GraphPoint(startpos.x+maxWidth, line1));
    mDrawer.drawLine(GraphPoint(startpos.x, line2), GraphPoint(startpos.x+maxWidth, line2));
    mDrawer.groupShapes(false, Color(245,245,255));
    mDrawer.groupText(true);
    for(const auto &dstr : drawStrings)
	mDrawer.drawText(dstr.pos, dstr.str.c_str());
    mDrawer.groupText(false);
    return GraphSize(maxWidth, y - startpos.y);
    }

static void drawPolyWithOffset(DiagramDrawer &drawer, const GraphPoint &offset,
	const OovPolygon &polygon, Color color)
    {
    OovPolygon polyWithOffset;
    for(const auto &pos : polygon)
	{
	GraphPoint p;
	p = pos;
	p.add(offset);
	polyWithOffset.push_back(p);
	}
    drawer.drawPoly(polyWithOffset, color);
    }

static void drawOovSymbol(DiagramDrawer &drawer, GraphPoint consumer,
	GraphPoint producer, const ClassConnectItem &connectItem)
    {
    const int halfSymbolSize = 11;
    // baseOffset is center of visibility circles
    const int baseOffset = 30;
    const int baseFuncParamOffset = baseOffset - halfSymbolSize;
//    const int totalOffset = baseOffset + (halfSymbolSize*2);
    const int quarterSymbolSize = 5;
    const int eighthSymbolSize = quarterSymbolSize/2;
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
//    if(abs(xdist) > totalOffset && abs(ydist) > totalOffset)
        {
        GraphPoint p;
        Color color;

        if(connectItem.mConnectType & ctAggregation ||
        	connectItem.mConnectType & ctFuncParam)
            {
            // Draw directional symbol
            if(connectItem.mConst)
        	{
        	// startPoint is Y position of point of V
                int startPoint;
                if(connectItem.mConnectType == ctFuncParam)
                    startPoint = baseFuncParamOffset - 14;
                else
                    startPoint = baseOffset - 14;
                int vtop = startPoint+10;
		const double triangleAngle = (2 * M_PI) / vtop;
		OovPolygon polygon;
		// Set start point (point of V)
		p.set(sin(lineAngleRadians) * startPoint, cos(lineAngleRadians) * startPoint);
		polygon.push_back(p);
		// calc left point of symbol
		p.set(sin(lineAngleRadians-triangleAngle) * vtop,
			cos(lineAngleRadians-triangleAngle) * vtop);
		polygon.push_back(p);

		p.set(sin(lineAngleRadians) * (startPoint+4),
		    cos(lineAngleRadians) * (startPoint+4));
		polygon.push_back(p);
		// calc right point of symbol
		p.set(sin(lineAngleRadians+triangleAngle) * vtop,
		    cos(lineAngleRadians+triangleAngle) * vtop);
		polygon.push_back(p);
		// Go back to start point
		p.set(sin(lineAngleRadians) * startPoint,
			cos(lineAngleRadians) * startPoint);
		polygon.push_back(p);

		color.set(255,255,255);
		drawPolyWithOffset(drawer, GraphPoint(consumer.x, consumer.y), polygon, color);
        	}
            }
        if(connectItem.mConnectType & ctFuncVar || connectItem.mConnectType & ctFuncParam)
            {
	    color.set(255,255,255);
	    // Set function circle center
	    // Make the func body param closer to more easily tell direction since the
	    // symbol doesn't indicate direction by itself.
	    int baseFuncOffset;
	    if(connectItem.mConnectType == ctFuncVar)
		baseFuncOffset = (baseOffset - quarterSymbolSize) - halfSymbolSize;
	    else
		baseFuncOffset = baseFuncParamOffset;
	    p.set(consumer.x + sin(lineAngleRadians) * baseFuncOffset,
		    consumer.y + cos(lineAngleRadians) * baseFuncOffset);
	    drawer.drawCircle(p, quarterSymbolSize, color);
	    if(connectItem.mConnectType == ctFuncParam)
		{
		const int baseFuncOffset = baseOffset - quarterSymbolSize;
		p.set(consumer.x + sin(lineAngleRadians) * baseFuncOffset,
			consumer.y + cos(lineAngleRadians) * baseFuncOffset);
		drawer.drawCircle(p, eighthSymbolSize, color);
		}
            }
        if(connectItem.hasAccess())
            {
	    color.set(255,255,255);
	    // Set circle center
	    p.set(consumer.x + sin(lineAngleRadians) * baseOffset,
		    consumer.y + cos(lineAngleRadians) * baseOffset);
	    if(connectItem.mAccess.getVis() == Visibility::Private)
		color.set(0,0,0);
	    drawer.drawCircle(p, quarterSymbolSize, color);
	    if(connectItem.mAccess.getVis() == Visibility::Protected)
		color.set(0,0,0);
	    drawer.drawCircle(p, eighthSymbolSize, color);
            }
        }
    }

static void drawHasSymbol(DiagramDrawer &drawer, GraphPoint owner,
	GraphPoint ownedMember, bool isRef)
    {
    int xdist = ownedMember.x-owner.x;
    int ydist = ownedMember.y-owner.y;
    double lineAngleRadians;
    if(ydist != 0)
        lineAngleRadians = atan2(xdist, ydist);
    else
        {
        if(ownedMember.x > owner.x)
            lineAngleRadians = M_PI/2;
        else
            lineAngleRadians = -M_PI/2;
        }
    const double hasAngle = (2 * M_PI) / 12;
    const double hasHalfSize = 9;

    OovPolygon polygon;
    // Do start point
    polygon.push_back(GraphPoint(0, 0));
    // calc left point of has symbol
    GraphPoint p(sin(lineAngleRadians-hasAngle) * hasHalfSize,
	    cos(lineAngleRadians-hasAngle) * hasHalfSize);
    polygon.push_back(p);
    // Calc end point
    p.set(sin(lineAngleRadians) * hasHalfSize * 2,
	    cos(lineAngleRadians) * hasHalfSize * 2);
    polygon.push_back(p);
    // calc right point of has symbol
    p.set(sin(lineAngleRadians+hasAngle) * hasHalfSize,
	    cos(lineAngleRadians+hasAngle) * hasHalfSize);
    polygon.push_back(p);

    // Go back to start point
    polygon.push_back(GraphPoint(0, 0));
    Color color;
    if(isRef)
	color.set(255,255,255);
    else
	color.set(0,0,0);
    drawPolyWithOffset(drawer, GraphPoint(owner.x, owner.y), polygon, color);
    }

static void drawIsSymbol(DiagramDrawer &drawer, GraphPoint parent, GraphPoint child)
    {
    int xdist = child.x-parent.x;
    int ydist = child.y-parent.y;
    double lineAngleRadians;
    if(ydist != 0)
        lineAngleRadians = atan2(xdist, ydist);
    else
        {
        if(parent.x > child.x)
            lineAngleRadians = -M_PI/2;
        else
            lineAngleRadians = M_PI/2;
        }
    const double isAngle = (2 * M_PI) / 16;
    const double isSize = 14;

    OovPolygon polygon;
    // Do start point
    polygon.push_back(GraphPoint(0, 0));
    // calc left point of is symbol
    GraphPoint p(sin(lineAngleRadians-isAngle) * isSize,
	    cos(lineAngleRadians-isAngle) * isSize);
    polygon.push_back(p);
    // calc right point of is symbol
    p.set(sin(lineAngleRadians+isAngle) * isSize,
	    cos(lineAngleRadians+isAngle) * isSize);
    polygon.push_back(p);

    // Go back to start point
    polygon.push_back(GraphPoint(0, 0));
    drawPolyWithOffset(drawer, GraphPoint(parent.x, parent.y), polygon, Color(255,255,255));
    }


void ClassDrawer::drawConnectionLine(const ClassNode &node1, const ClassNode &node2)
    {
    GraphRect rect1;
    GraphRect rect2;
    node1.getRect(rect1);
    node2.getRect(rect2);
    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);
    mDrawer.drawLine(p1e, p2e);
    }

void ClassDrawer::drawConnectionSymbols(const ClassRelationDrawOptions &options,
	const ClassNode &node1, const ClassNode &node2,
	const ClassConnectItem &connectItem)
    {
    GraphRect rect1;
    GraphRect rect2;
    node1.getRect(rect1);
    node2.getRect(rect2);

    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);

    switch(connectItem.mConnectType & ctObjectRelationMask)
	{
	case ctAggregation:
	    // node1=owner, node2=owned.
	    drawHasSymbol(mDrawer, p1e, p2e, connectItem.mRefer);
	    break;

	case ctIneritance:
	    // node1=parent, node2=child.
	    drawIsSymbol(mDrawer, p1e, p2e);
	    break;

	default:
	    break;
	}
    if(options.drawOovSymbols)
	{
	drawOovSymbol(mDrawer, p1e, p2e, connectItem);
	}
    }

void ClassDrawer::drawDiagram(const ClassGraph &graph,
	const ClassDrawOptions &options)
    {
    if(graph.getNodes().size() > 0)
	{
	mDrawer.setDiagramSize(graph.getGraphSize());
	for(size_t ni1=0; ni1<graph.getNodes().size(); ni1++)
	    {
	    drawNode(graph.getNodes()[ni1]);
	    }
	mDrawer.groupShapes(true, Color(245,245,255));
	for(size_t ni1=0; ni1<graph.getNodes().size(); ni1++)
	    {
	    for(size_t ni2=ni1+1; ni2<graph.getNodes().size(); ni2++)
		{
		const ClassConnectItem *ci1 = graph.getNodeConnection(ni1, ni2);
		const ClassConnectItem *ci2 = graph.getNodeConnection(ni2, ni1);
		if(ci1 || ci2)
		    {
		    drawConnectionLine(graph.getNodes()[ni1], graph.getNodes()[ni2]);
		    }
		if(ci1)
		    {
		    drawConnectionSymbols(options, graph.getNodes()[ni1],
			    graph.getNodes()[ni2], *ci1);
		    }
		if(ci2)
		    {
		    drawConnectionSymbols(options, graph.getNodes()[ni2],
			    graph.getNodes()[ni1], *ci2);
		    }
		}
	    }
	mDrawer.groupShapes(false, Color(245,245,255));
	}
    }

