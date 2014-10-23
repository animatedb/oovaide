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

static void getStrings(const ClassNode &node,
	std::vector<std::string> &nodeStrs, std::vector<std::string> &attrStrs,
	std::vector<std::string> &operStrs)
    {
    const ModelType *type = node.getType();
    char const * const typeName = type->getName().c_str();
    nodeStrs.push_back(typeName);
    if(node.getDrawOptions().drawPackageName)
	{
	const ModelClassifier *cls = node.getType()->getClass();
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
    const ModelClassifier *classifier = type->getClass();
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

void splitClassStrings(std::vector<std::string> &nodeStrs,
	std::vector<std::string> &attrStrs, std::vector<std::string> &operStrs,
	float fontWidth, float fontHeight)
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
    size_t desiredLength = 0;
    size_t maxLength = lengths[lengths.size()-1];
    if(lengths.size() == 1)
	{
	desiredLength = 50;
	int len = lengths[0] / 50;
	if(len == 0)
	    len = lengths[0];
	else
	    desiredLength = 50;
	}
    else
	{
        float biggestRatio = 0;
        size_t biggestRatioIndex = 0;
        for(int numSplits=1; numSplits<2; numSplits++)
            {
	    for(size_t i=lengths.size()/2; i<lengths.size()-1; i++)
		{
		float ratio = (maxLength - lengths[i]) / (lengths.size() - i);
		if(ratio > biggestRatio)
		    {
		    biggestRatio = ratio;
		    biggestRatioIndex = i;
		    }
		}
            }
	if(biggestRatio > 8)
	    desiredLength = lengths[biggestRatioIndex];
	}
    if(desiredLength != 0)
	{
	// Split into a maximum of four lines.
	size_t minSplitLength = maxLength / 4;
	if(desiredLength < minSplitLength)
	    desiredLength = minSplitLength;

	splitStrings(nodeStrs, desiredLength);
	splitStrings(attrStrs, desiredLength);
	splitStrings(operStrs, desiredLength);
	}
    }

GraphSize ClassDrawer::drawNode(const ClassNode &node, const ClassDrawOptions &options)
    {
    if(node.getType())
	{
	GraphPoint startpos(node.getPosition().getZoomed(mActualZoomX, mActualZoomY));
	float fontHeight = mDrawer.getTextExtentHeight("W");
	float fontWidth = mDrawer.getTextExtentWidth("W");
	float pad = fontHeight / 7.f;
    //    if(pad < 1)
    //	pad = 1;
	float pad2 = pad*2;
	float padLine = pad*3;
	int line1 = startpos.y;
	int line2 = startpos.y;
	std::vector<DrawString> drawStrings;
	std::vector<std::string> nodeStrs;
	std::vector<std::string> attrStrs;
	std::vector<std::string> operStrs;

	getStrings(node, nodeStrs, attrStrs, operStrs);
	splitClassStrings(nodeStrs, attrStrs, operStrs, fontWidth, fontHeight);
	float y = startpos.y;
	for(auto const &str : nodeStrs)
	    {
	    y += fontHeight + pad2;
	    drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str.c_str()));
	    }
	y += padLine;	// Space for line.
	line1 = y;
	for(auto const &str : attrStrs)
	    {
	    y += fontHeight + pad2;
	    drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str.c_str()));
	    }
	y += padLine;	// Space for line.
	line2 = y;
	for(auto const &str : operStrs)
	    {
	    y += fontHeight + pad2;
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
    else
	{
	return drawRelationKey(node, options);
	}
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

class RelationDrawInfo
    {
    public:
	RelationDrawInfo(GraphPoint diconsumer, GraphPoint diproducer, int dizoom):
	    zoom(dizoom), consumer(diconsumer), producer(diproducer)
	    {
	    halfSymbolSize = 11 * dizoom;
	    // baseOffset is center of visibility circles
	    baseVisibilityOffset = 30 * dizoom;
	    quarterSymbolSize = 5 * dizoom;
	    baseFuncOffset = (baseVisibilityOffset - quarterSymbolSize) - halfSymbolSize;
	    eighthSymbolSize = quarterSymbolSize/2;
	    xdist = producer.x-consumer.x;
	    int ydist = producer.y-consumer.y;
	    if(ydist != 0)
		lineAngleRadians = atan2(xdist, ydist);
	    else
		{
		if(producer.x > consumer.x)
		    lineAngleRadians = M_PI/2;
		else
		    lineAngleRadians = -M_PI/2;
		}
	    }
	void setPoints(GraphPoint diconsumer, GraphPoint diproducer)
	    {
	    consumer = diconsumer;
	    producer = diproducer;
	    }
	int halfSymbolSize;
	int baseVisibilityOffset;
	int baseFuncOffset;
	int quarterSymbolSize;
	int eighthSymbolSize;
	int xdist;
	int zoom;
	GraphPoint consumer;
	GraphPoint producer;
	double lineAngleRadians;
    };

static void drawConst(DiagramDrawer &drawer, RelationDrawInfo const &drawInfo)
    {
    Color color;
    GraphPoint p;
    // startPoint is Y position of point of V
    int startPoint = 0;
//    if(connectType == ctFuncParam || connectType == ctFuncVar)
//	startPoint = drawInfo.baseFuncOffset - 14 * drawInfo.zoom;
//    else
//	startPoint = drawInfo.baseVisibilityOffset - 14 * drawInfo.zoom;
    int vtop = startPoint + 10 * drawInfo.zoom;
    const double triangleAngle = (2 * M_PI) / vtop;
    OovPolygon polygon;
    // Set start point (point of V)
    p.set(sin(drawInfo.lineAngleRadians) * startPoint,
	    cos(drawInfo.lineAngleRadians) * startPoint);
    polygon.push_back(p);
    // calc left point of symbol
    p.set(sin(drawInfo.lineAngleRadians-triangleAngle) * vtop,
	    cos(drawInfo.lineAngleRadians-triangleAngle) * vtop);
    polygon.push_back(p);

    p.set(sin(drawInfo.lineAngleRadians) * (startPoint + 4 * drawInfo.zoom),
	cos(drawInfo.lineAngleRadians) * (startPoint + 4 * drawInfo.zoom));
    polygon.push_back(p);
    // calc right point of symbol
    p.set(sin(drawInfo.lineAngleRadians+triangleAngle) * vtop,
	cos(drawInfo.lineAngleRadians+triangleAngle) * vtop);
    polygon.push_back(p);
    // Go back to start point
    p.set(sin(drawInfo.lineAngleRadians) * startPoint,
	    cos(drawInfo.lineAngleRadians) * startPoint);
    polygon.push_back(p);

    color.set(255,255,255);
    drawPolyWithOffset(drawer, GraphPoint(drawInfo.consumer.x, drawInfo.consumer.y),
	    polygon, color);
    }

static void drawFuncRelation(DiagramDrawer &drawer, RelationDrawInfo const &drawInfo,
	eDiagramConnectType connectType)
    {
    Color color;
    GraphPoint p;
    color.set(255,255,255);
    p.set(drawInfo.consumer.x + sin(drawInfo.lineAngleRadians) * drawInfo.baseFuncOffset,
	    drawInfo.consumer.y + cos(drawInfo.lineAngleRadians) * drawInfo.baseFuncOffset);
    drawer.drawCircle(p, drawInfo.quarterSymbolSize, color);
    if(connectType == ctFuncParam)
	{
	const int paramOffset = drawInfo.baseFuncOffset + drawInfo.quarterSymbolSize +
		drawInfo.eighthSymbolSize;
	p.set(drawInfo.consumer.x + sin(drawInfo.lineAngleRadians) * paramOffset,
		drawInfo.consumer.y + cos(drawInfo.lineAngleRadians) * paramOffset);
	drawer.drawCircle(p, drawInfo.eighthSymbolSize, color);
	}
    }

static void drawVisibility(DiagramDrawer &drawer, RelationDrawInfo const &drawInfo,
	Visibility vis)
    {
    Color color;
    GraphPoint p;
    color.set(255,255,255);
    // Set circle center
    p.set(drawInfo.consumer.x + sin(drawInfo.lineAngleRadians) * drawInfo.baseVisibilityOffset,
	    drawInfo.consumer.y + cos(drawInfo.lineAngleRadians) * drawInfo.baseVisibilityOffset);
    if(vis.getVis() == Visibility::Private)
	color.set(0,0,0);
    drawer.drawCircle(p, drawInfo.quarterSymbolSize, color);
    if(vis.getVis() == Visibility::Protected)
	color.set(0,0,0);
    drawer.drawCircle(p, drawInfo.eighthSymbolSize, color);
    }

static void drawOovSymbol(DiagramDrawer &drawer, GraphPoint consumer,
	GraphPoint producer, const ClassConnectItem &connectItem, double zoom)
    {
    RelationDrawInfo drawInfo(consumer, producer, zoom);
    if((connectItem.mConnectType & ctAggregation) ||
	    (connectItem.mConnectType & ctFuncParam))
	{
	// Draw const symbol
	if(connectItem.mConst)
	    {
	    drawConst(drawer, drawInfo);
	    }
	}
    if((connectItem.mConnectType & ctFuncVar) || (connectItem.mConnectType & ctFuncParam))
	{
	drawFuncRelation(drawer, drawInfo, connectItem.mConnectType);
	}
    if(connectItem.hasAccess())
	{
	drawVisibility(drawer, drawInfo, connectItem.mAccess);
	}
    }

static void drawHasSymbol(DiagramDrawer &drawer, GraphPoint owner,
	GraphPoint ownedMember, bool isRef, double zoom)
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
    double hasHalfSize = 9 * zoom;
    double hasSize = hasHalfSize * 2 - (4 * zoom);

    OovPolygon polygon;
    // Do start point
    polygon.push_back(GraphPoint(0, 0));
    // calc left point of has symbol
    GraphPoint p(sin(lineAngleRadians-hasAngle) * hasHalfSize,
	    cos(lineAngleRadians-hasAngle) * hasHalfSize);
    polygon.push_back(p);
    // Calc end point
    p.set(sin(lineAngleRadians) * hasSize, cos(lineAngleRadians) * hasSize);
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

static void drawIsSymbol(DiagramDrawer &drawer, GraphPoint parent, GraphPoint child, double zoom)
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
    double isSize = 14 * zoom;

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
    GraphRect rect1 = node1.getRect().getZoomed(mActualZoomX, mActualZoomY);
    GraphRect rect2 = node2.getRect().getZoomed(mActualZoomX, mActualZoomY);
    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);
    mDrawer.drawLine(p1e, p2e);
    }

void ClassDrawer::drawConnectionSymbols(const ClassRelationDrawOptions &options,
	const ClassNode &node1, const ClassNode &node2,
	const ClassConnectItem &connectItem)
    {
    GraphRect rect1 = node1.getRect().getZoomed(mActualZoomX, mActualZoomY);
    GraphRect rect2 = node2.getRect().getZoomed(mActualZoomX, mActualZoomY);

    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);

    if(options.drawOovSymbols)
	{
	drawOovSymbol(mDrawer, p1e, p2e, connectItem, mActualZoomY);
	}
    // Draw the aggregation symbol after the oov symbols so that it can show up
    // over the const and function symbols.
    switch(connectItem.mConnectType & ctObjectRelationMask)
	{
	case ctAggregation:
	    // node1=owner, node2=owned.
	    drawHasSymbol(mDrawer, p1e, p2e, connectItem.mRefer, mActualZoomY);
	    break;

	case ctIneritance:
	    // node1=parent, node2=child.
	    drawIsSymbol(mDrawer, p1e, p2e, mActualZoomY);
	    break;

	default:
	    break;
	}
    }

GraphSize ClassDrawer::drawRelationKey(const ClassNode &node,
	const ClassDrawOptions &options)
    {
    GraphPoint startpos(node.getPosition().getZoomed(mActualZoomX, mActualZoomY));
    float fontHeight = mDrawer.getTextExtentHeight("W");
    float pad = fontHeight / 7.f;
    std::vector<std::string> strs = { "Relations Key",
	    "Generalization", "Aggregation(reference)", "Composition(owned)"};

    if(options.drawOovSymbols)
	{
	strs.push_back("Const");
	strs.push_back("Method Variable");
	strs.push_back("Method Parameter");
	strs.push_back("Public");
	strs.push_back("Protected");
	strs.push_back("Private");
	}

    int strLenPixels = 0;
    for(auto const &str : strs)
	{
	float len = mDrawer.getTextExtentWidth(str.c_str());
	if(len > strLenPixels)
	    strLenPixels = len;
	}

    int shapeWidth = 18 * mActualZoomY;
    int shapeHeight = 14 * mActualZoomY;
    int keyWidth = strLenPixels+shapeWidth+pad*4;
    int keyHeight = strs.size()*shapeHeight+pad*3;
    mDrawer.groupShapes(true, Color(245,245,255));
    mDrawer.drawRect(GraphRect(startpos.x, startpos.y, keyWidth, keyHeight));
    GraphPoint p1 = startpos;
    mDrawer.drawLine(GraphPoint(p1.x, p1.y+shapeHeight-pad),
	    GraphPoint(p1.x+keyWidth, p1.y+shapeHeight-pad));	// Draw key header separator line
    int startSymbolXPos = startpos.x + strLenPixels + pad * 2;
    p1.x = startSymbolXPos;
    p1.y += shapeHeight/2 + shapeHeight;	// Room for key header
    GraphPoint p2 = p1;
    p2.x += shapeWidth * 2;
    drawIsSymbol(mDrawer, p1, p2, mActualZoomY);
    p1.y += shapeHeight;
    p2.y += shapeHeight;
    drawHasSymbol(mDrawer, p1, p2, true, mActualZoomY);
    p1.y += shapeHeight;
    p2.y += shapeHeight;
    drawHasSymbol(mDrawer, p1, p2, false, mActualZoomY);
    if(options.drawOovSymbols)
	{
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	RelationDrawInfo drawInfo(p1, p2, mActualZoomY);
	drawConst(mDrawer, drawInfo);
	p1.x = startSymbolXPos - drawInfo.baseFuncOffset;
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	drawInfo.setPoints(p1, p2);
	drawFuncRelation(mDrawer, drawInfo, ctFuncVar);
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	drawInfo.setPoints(p1, p2);
	drawFuncRelation(mDrawer, drawInfo, ctFuncParam);
	p1.x = startSymbolXPos - drawInfo.baseVisibilityOffset;
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	drawInfo.setPoints(p1, p2);
	drawVisibility(mDrawer, drawInfo, Visibility::Public);
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	drawInfo.setPoints(p1, p2);
	drawVisibility(mDrawer, drawInfo, Visibility::Protected);
	p1.y += shapeHeight;
	p2.y += shapeHeight;
	drawInfo.setPoints(p1, p2);
	drawVisibility(mDrawer, drawInfo, Visibility::Private);
	}
    mDrawer.groupShapes(false, Color(245,245,255));

    mDrawer.groupText(true);
    p1 = startpos;
    p1.x += pad;
    p1.y += fontHeight + pad*2;
    for(auto const &str : strs)
	{
	mDrawer.drawText(p1, str.c_str());
	p1.y += shapeHeight;
	}
    mDrawer.groupText(false);
    return GraphSize(keyWidth, keyHeight);
    }

void ClassDrawer::setZoom(double desiredZoom)
    {
    /*
    mDrawer.setFontSize(10);	// Set to default font size.
    double noZoomHeight = mDrawer.getTextExtentHeight("W");
//    double noZoomWidth = mDrawer.getTextExtentWidth("W");
    mDrawer.setFontSize(desiredZoom*10);
    double zoomHeight = mDrawer.getTextExtentHeight("W");
//    double zoomWidth = mDrawer.getTextExtentWidth("W");
    mActualZoomY = zoomHeight / noZoomHeight;
    mActualZoomX = mActualZoomY; // zoomWidth / noZoomWidth;
*/
    mDrawer.setFontSize(desiredZoom*10);
    mActualZoomX = desiredZoom;
    mActualZoomY = desiredZoom;
    }

void ClassDrawer::drawDiagram(const ClassGraph &graph,
	const ClassDrawOptions &options)
    {
    if(graph.getNodes().size() > 0)
	{
	mDrawer.setDiagramSize(graph.getGraphSize().getZoomed(mActualZoomX,
		mActualZoomY));
	for(size_t ni1=0; ni1<graph.getNodes().size(); ni1++)
	    {
	    drawNode(graph.getNodes()[ni1], options);
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

