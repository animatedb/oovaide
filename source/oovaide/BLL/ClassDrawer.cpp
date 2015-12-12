/*
 * ClassDrawer.cpp
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassDrawer.h"
#include "Project.h"
#include <algorithm>
#include <utility>

static void getLeafPath(std::string &moduleStr)
    {
    size_t pos = FilePathGetPosLeftPathSep(moduleStr, moduleStr.length()-1, RP_RetPosFailure);
    if(pos != std::string::npos)
        {
        moduleStr.resize(pos);
        }

    OovString rootDir = Project::getSourceRootDirectory();
    int len = rootDir.length();
    if(moduleStr.compare(0, len, rootDir) == 0)
        {
        moduleStr.erase(0, len);
        }
    }

static void getStrings(const ClassNode &node,
    OovStringVec &nodeStrs, OovStringVec &attrStrs,
    OovStringVec &operStrs, std::vector<bool> &virtOpers)
    {
    const ModelType *type = node.getType();
    OovStringRef const typeName = type->getName();
    nodeStrs.push_back(typeName);
    if(node.getNodeOptions().drawPackageName)
        {
        const ModelClassifier *cls = ModelClassifier::getClass(node.getType());
        if(cls)
            {
            const ModelModule *module = cls->getModule();
            if(module)
                {
                std::string moduleStr = module->getModulePath();
                getLeafPath(moduleStr);
                nodeStrs.push_back(moduleStr);
                }
            }
        }
    const ModelClassifier *classifier = ModelClassifier::getClass(type);
    if(classifier)
        {
        if(node.getNodeOptions().drawAttributes)
            {
            for(const auto &attr : classifier->getAttributes())
                {
                OovString name = attr->getAccess().asUmlStr();
                if(node.getNodeOptions().drawAttrTypes)
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

        if(node.getNodeOptions().drawOperations)
            {
            for(const auto &oper : classifier->getOperations())
                {
                OovString operStr = oper->getAccess().asUmlStr();
                if(node.getNodeOptions().drawOperReturn)
                    {
                    ModelType const *type = oper->getReturnType().getDeclType();
                    if(type)
                        {
                        operStr += type->getName();
                        operStr += ' ';
                        }
                    }
                operStr += oper->getName();
                operStr += "(";
                if(node.getNodeOptions().drawOperParams)
                    {
                    bool firstParam = true;
                    for(const auto &opparam : oper->getParams())
                        {
                        if(firstParam)
                            firstParam = false;
                        else
                            operStr += ", ";
                        if(node.getNodeOptions().drawOperTypes)
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
                virtOpers.push_back(oper->isVirtual());
                }
            }
        }
    }

// This isn't exact, because the splitStrings function adds some leading
// spaces for split lines.
size_t getNumSegments(OovString const &str, size_t minLength, size_t maxLength)
    {
    size_t pos = 0;
    size_t numSplits = 0;
    while(pos < str.length())
        {
        pos = findSplitPos(str, pos+minLength, pos+maxLength);
        numSplits++;
        }
    return numSplits;
    }

// Some actual code lengths can be up to 400 characters (Ex: See IncDirDependencyMap)
// Attempt to split at certain characters, not in the middle of words.
/// @param originalOperStrIndices This will not be filled if no strings were
///         split.
static void splitClassStrings(OovStringVec &nodeStrs, OovStringVec &attrStrs,
    OovStringVec &operStrs, float fontHeight,
    std::vector<size_t> &originalOperStrIndices)
    {
    OovStringVec strs;
    strs = nodeStrs;
    strs.insert(strs.end(), attrStrs.begin(), attrStrs.end());
    strs.insert(strs.end(), operStrs.begin(), operStrs.end());

    size_t maxLength = 0;
    for(auto const &str : strs)
        {
        size_t len = str.length();
        if(len > maxLength)
            {
            maxLength = len;
            }
        }
    const int MIN_SPLIT=40;
    const int SPLIT_WIDTH=10;
    if(maxLength > MIN_SPLIT)
        {
        size_t bestLength = maxLength;
        size_t desiredSegments = 0;
        for(size_t length = maxLength; length > MIN_SPLIT/2; length -= SPLIT_WIDTH/2)
            {
            size_t totalSegments = 0;
            for(size_t i=0; i<strs.size(); i++)
                {
                totalSegments += getNumSegments(strs[i], length, length + SPLIT_WIDTH);
                }
            float ratio = length / totalSegments;
            if(ratio < 2 || length < MIN_SPLIT)
                {
                // Last time through with the desired segments will be
                // the shortest length
                if(desiredSegments != 0 && desiredSegments != totalSegments)
                    {
                    break;
                    }
                desiredSegments = totalSegments;
                }
            if(length < bestLength)
                {
                bestLength = length;
                }
            }
        if(bestLength != 0)
            {
            splitStrings(nodeStrs, bestLength, bestLength + SPLIT_WIDTH);
            splitStrings(attrStrs, bestLength, bestLength + SPLIT_WIDTH);
            splitStrings(operStrs, bestLength, bestLength + SPLIT_WIDTH,
                &originalOperStrIndices);
            }
        }
    }

static void addDrawString(OovString str, GraphPoint startPoint,
    float fontHeight, float pad, float &y, std::vector<DrawString> &drawStrings)
    {
    y += fontHeight + (pad*2);
    startPoint.y = y;
    OovString outStr;
    int xOffset = 0;
    if(str.compare(0, strlen(getSplitStringPrefix()), getSplitStringPrefix()) == 0)
        {
        xOffset += fontHeight;      // Using font height as indent in x
        outStr = &str.c_str()[2];
        }
    else
        {
        outStr = str;
        }
    GraphPoint drawPoint(startPoint.x + xOffset, y);
    drawStrings.push_back(DrawString(drawPoint, outStr));
    }

static void addDrawStrings(OovStringVec const &strs, GraphPoint startPoint,
        float fontHeight, float pad, float &y, std::vector<DrawString> &drawStrings)
    {
    for(auto const &str : strs)
        {
    	addDrawString(str, startPoint, fontHeight, pad, y, drawStrings);
        }
    }

GraphSize ClassDrawer::drawNode(const ClassNode &node)
    {
    if(node.getType())
        {
        GraphPoint startpos(node.getPosition().getZoomed(mActualZoom));
        float fontHeight = mDrawer.getTextExtentHeight("W");
        float pad = fontHeight / 7.f;
    //    if(pad < 1)
    //  pad = 1;
        float pad2 = pad*2;
        float padLine = pad*3;
        int line1 = startpos.y;
        int line2 = startpos.y;
        std::vector<DrawString> drawStrings;
        std::vector<DrawString> virtDrawStrings;
        OovStringVec nodeStrs;
        OovStringVec attrStrs;
        OovStringVec operStrs;
        std::vector<size_t> originalOperStrIndices;
        std::vector<bool> virtOpers;

        getStrings(node, nodeStrs, attrStrs, operStrs, virtOpers);
        splitClassStrings(nodeStrs, attrStrs, operStrs, fontHeight,
        	originalOperStrIndices);
        float y = startpos.y;
        for(auto const &str : nodeStrs)
            {
            y += fontHeight + pad2;
            drawStrings.push_back(DrawString(GraphPoint(startpos.x+pad, y), str));
            }
        y += padLine;   // Space for line.
        line1 = y;
        addDrawStrings(attrStrs, GraphPoint(startpos.x+pad, y), fontHeight,
	    pad, y, drawStrings);
        y += padLine;   // Space for line.
        line2 = y;
        // Keep the order of operators the same as the source.  This means
        // virtual methods are interspersed with normal methods.
        for(size_t i=0; i<operStrs.size(); i++)
            {
            size_t index = 0;
            if(originalOperStrIndices.size())
                {
                index = originalOperStrIndices[i];
                }
            else
                {
                index = i;
                }
            if(!virtOpers[index])
                {
                addDrawString(operStrs[i], GraphPoint(startpos.x+pad, y),
                fontHeight, pad, y, drawStrings);
                }
            else
                {
                addDrawString(operStrs[i], GraphPoint(startpos.x+pad, y),
                fontHeight, pad, y, virtDrawStrings);
                }
            }
        int maxWidth = 0;
        int wrapInc = 0;
        if(attrStrs.size() > 1 || operStrs.size() > 1)
            {
            wrapInc = pad;
            }
        for(const auto &dstr : drawStrings)
            {
            int width = mDrawer.getTextExtentWidth(dstr.str) + wrapInc;
            if(width > maxWidth)
                maxWidth = width;
            }
        for(const auto &dstr : virtDrawStrings)
            {
            int width = mDrawer.getTextExtentWidth(dstr.str) + wrapInc;
            if(width > maxWidth)
                maxWidth = width;
            }
        maxWidth += pad2;
        y += pad2;
        mDrawer.groupShapes(true, Color(0,0,0), Color(245,255,245));
        mDrawer.drawRect(GraphRect(startpos.x, startpos.y, maxWidth, y-startpos.y));
        mDrawer.drawLine(GraphPoint(startpos.x, line1), GraphPoint(startpos.x+maxWidth, line1));
        mDrawer.drawLine(GraphPoint(startpos.x, line2), GraphPoint(startpos.x+maxWidth, line2));
        mDrawer.groupShapes(false, 0, 0);
        mDrawer.groupText(true, false);
        for(const auto &dstr : drawStrings)
            {
            mDrawer.drawText(dstr.pos, dstr.str);
            }
        mDrawer.groupText(false, false);

        mDrawer.groupText(true, true);
        for(const auto &dstr : virtDrawStrings)
            {
            mDrawer.drawText(dstr.pos, dstr.str);
            }
        mDrawer.groupText(false, true);
        return GraphSize(maxWidth, y - startpos.y);
        }
    else
        {
        return drawRelationKey(node);
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
//      startPoint = drawInfo.baseFuncOffset - 14 * drawInfo.zoom;
//    else
//      startPoint = drawInfo.baseVisibilityOffset - 14 * drawInfo.zoom;
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
    if(connectType & ctFuncParam)
        {
        const int paramOffset = drawInfo.baseFuncOffset + drawInfo.quarterSymbolSize +
                drawInfo.eighthSymbolSize;
        p.set(drawInfo.consumer.x + sin(drawInfo.lineAngleRadians) * paramOffset,
                drawInfo.consumer.y + cos(drawInfo.lineAngleRadians) * paramOffset);
        drawer.drawCircle(p, drawInfo.eighthSymbolSize, color);
        }
    }

static void drawTemplateRelation(DiagramDrawer &drawer, RelationDrawInfo const &drawInfo,
        double zoom)
    {
    drawer.drawLine(drawInfo.consumer, drawInfo.producer, true);

    int xdist = drawInfo.consumer.x-drawInfo.producer.x;
    int ydist = drawInfo.consumer.y-drawInfo.producer.y;
    double lineAngleRadians;
    if(ydist != 0)
        lineAngleRadians = atan2(xdist, ydist);
    else
        {
        if(drawInfo.producer.x > drawInfo.consumer.x)
            lineAngleRadians = -M_PI/2;
        else
            lineAngleRadians = M_PI/2;
        }
    const double isAngle = (2 * M_PI) / 16;
    double isSize = 14 * zoom;

    // calc left point of arrow
    GraphPoint p(sin(lineAngleRadians-isAngle) * isSize,
            cos(lineAngleRadians-isAngle) * isSize);
    drawer.drawLine(drawInfo.producer, drawInfo.producer + p);
    // calc right point of arrow
    p.set(sin(lineAngleRadians+isAngle) * isSize,
            cos(lineAngleRadians+isAngle) * isSize);
    drawer.drawLine(drawInfo.producer, drawInfo.producer + p);
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
// Don't ever display func var with const symbol.
//            (connectItem.mConnectType & ctFuncVar)
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
    if(connectItem.mConnectType & ctTemplateDependency)
        {
        drawTemplateRelation(drawer, drawInfo, zoom);
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

void ClassDrawer::drawConnectionLine(const ClassNode &node1, const ClassNode &node2,
    bool dashed)
    {
    GraphRect rect1 = node1.getRect().getZoomed(mActualZoom);
    GraphRect rect2 = node2.getRect().getZoomed(mActualZoom);
    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);
    mDrawer.drawLine(p1e, p2e, dashed);
    }

void ClassDrawer::drawConnectionSymbols(const ClassRelationDrawOptions &options,
        const ClassNode &node1, const ClassNode &node2,
        const ClassConnectItem &connectItem)
    {
    GraphRect rect1 = node1.getRect().getZoomed(mActualZoom);
    GraphRect rect2 = node2.getRect().getZoomed(mActualZoom);

    GraphPoint p1e;
    GraphPoint p2e;
    rect1.findConnectPoints(rect2, p1e, p2e);

    if(options.drawOovSymbols)
        {
        drawOovSymbol(mDrawer, p1e, p2e, connectItem, mActualZoom);
        }
    // Draw the aggregation symbol after the oov symbols so that it can show up
    // over the const and function symbols.
    if(connectItem.mConnectType & ctAggregation)
        {
        // node1=owner, node2=owned.
        drawHasSymbol(mDrawer, p1e, p2e, connectItem.mRefer, mActualZoom);
        }
    if(connectItem.mConnectType & ctIneritance)
        {
        // node1=parent, node2=child.
        drawIsSymbol(mDrawer, p1e, p2e, mActualZoom);
        }
    }

GraphSize ClassDrawer::drawRelationKey(const ClassNode &node)
    {
    GraphPoint startpos(node.getPosition().getZoomed(mActualZoom));
    float fontHeight = mDrawer.getTextExtentHeight("W");
    float pad = fontHeight / 7.f;
    std::vector<std::string> strs = { "Relations Key",
            "Generalization", "Aggregation(reference)", "Composition(owned)"};

//    if(node.getDrawOptions().drawOovSymbols)
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
        float len = mDrawer.getTextExtentWidth(str);
        if(len > strLenPixels)
            strLenPixels = len;
        }

    int shapeWidth = (fontHeight * 2) * mActualZoom;
    int shapeHeight = (fontHeight * 2) * mActualZoom;
    int keyWidth = strLenPixels+shapeWidth+pad*4;
    int keyHeight = strs.size()*shapeHeight+pad*3;
    mDrawer.groupShapes(true, Color(0,0,0), Color(245,255,245));
    mDrawer.drawRect(GraphRect(startpos.x, startpos.y, keyWidth, keyHeight));
    GraphPoint p1 = startpos;
    mDrawer.drawLine(GraphPoint(p1.x, p1.y+shapeHeight-pad),
            GraphPoint(p1.x+keyWidth, p1.y+shapeHeight-pad));   // Draw key header separator line
    int startSymbolXPos = startpos.x + strLenPixels + pad * 2;
    p1.x = startSymbolXPos;
    p1.y += shapeHeight/2 + shapeHeight;        // Room for key header
    GraphPoint p2 = p1;
    p2.x += shapeWidth * 2;
    drawIsSymbol(mDrawer, p1, p2, mActualZoom);
    p1.y += shapeHeight;
    p2.y += shapeHeight;
    drawHasSymbol(mDrawer, p1, p2, true, mActualZoom);
    p1.y += shapeHeight;
    p2.y += shapeHeight;
    drawHasSymbol(mDrawer, p1, p2, false, mActualZoom);
//    if(options.drawOovSymbols)
        {
        p1.y += shapeHeight;
        p2.y += shapeHeight;
        RelationDrawInfo drawInfo(p1, p2, mActualZoom);
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
    mDrawer.groupShapes(false, Color(0,0,0), Color(245,245,255));

    mDrawer.groupText(true, false);
    p1 = startpos;
    p1.x += pad;
    p1.y += fontHeight + pad*2;
    for(auto const &str : strs)
        {
        mDrawer.drawText(p1, str);
        p1.y += shapeHeight;
        }
    mDrawer.groupText(false, false);
    return GraphSize(keyWidth, keyHeight);
    }

void ClassDrawer::setZoom(double desiredZoom)
    {
    mDrawer.setCurrentDrawingFontSize(desiredZoom * mDiagram.getDiagramBaseFontSize());
    mActualZoom = desiredZoom;
    }

class DrawnLines
    {
    public:
        /// Returns true if line was not present, and was added
        bool addLine(GraphPoint p1, GraphPoint p2)
            {
            if(!(p1 < p2))
                {
                std::swap(p1, p2);
                }
            return(mLines.insert(GraphLine(p1, p2)).second == true);
            }
    private:
        std::set<GraphLine> mLines;
    };

void ClassDrawer::drawDiagram(const ClassGraph &graph)
    {
    if(graph.getNodes().size() > 0)
        {
        mDrawer.setDiagramSize(graph.getGraphSize().getZoomed(mActualZoom));
        for(size_t ni1=0; ni1<graph.getNodes().size(); ni1++)
            {
            drawNode(graph.getNodes()[ni1]);
            }
        mDrawer.groupShapes(true, Color(0,0,0), Color(245,245,255));
        // Prevent duplicate dashed lines since multiple dashed lines can
        // draw as solid in some cases.
        DrawnLines drawnDashedLines;
        for(size_t ni1=0; ni1<graph.getNodes().size(); ni1++)
            {
            for(size_t ni2=ni1+1; ni2<graph.getNodes().size(); ni2++)
                {
                const ClassConnectItem *ci1 = graph.getNodeConnection(ni1, ni2);
                const ClassConnectItem *ci2 = graph.getNodeConnection(ni2, ni1);
                GraphPoint p1 = graph.getNodes()[ni1].getPosition();
                GraphPoint p2 = graph.getNodes()[ni2].getPosition();
                const ClassConnectItem *conn = (ci1 != nullptr) ? ci1 : ci2;
                if(conn)
                    {
                    bool drawSolid = false;
                    bool drawDashed = false;
                    // If there are relationships in addition to a template, then
                    // draw a solid line.
                    if(conn->mConnectType == ctTemplateDependency)
                        {
                        if(!drawnDashedLines.addLine(p1, p2))
                            {
                            drawDashed = true;
                            }
                        }
                    else
                        {
                        drawSolid = true;
                        }
                    if(drawSolid || drawDashed)
                        {
                        drawConnectionLine(graph.getNodes()[ni1], graph.getNodes()[ni2],
                            drawDashed);
                        }
                    }
                if(ci1)
                    {
                    drawConnectionSymbols(graph.getOptions(), graph.getNodes()[ni1],
                            graph.getNodes()[ni2], *ci1);
                    }
                if(ci2)
                    {
                    drawConnectionSymbols(graph.getOptions(), graph.getNodes()[ni2],
                            graph.getNodes()[ni1], *ci2);
                    }
                }
            }
        mDrawer.groupShapes(false, Color(0,0,0), Color(245,245,255));
        }
    }

