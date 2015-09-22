/*
 * PortionDiagram.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionDiagram.h"
#include "DiagramStorage.h"
#include "algorithm"


void PortionDiagram::initialize(const ModelData &modelData)
    {
    mModelData = &modelData;
    mPortionGraph.setGraphDataSource(modelData);
    mPortionDrawer.setGraphSource(mPortionGraph);
    }

void PortionDiagram::clearGraphAndAddClass(DiagramDrawer &drawer,
        OovStringRef className)
    {
    mCurrentClassName = className;
    mPortionGraph.clearAndAddClass(className);
    mPortionDrawer.updateNodePositions(drawer);
    }

void PortionDiagram::drawDiagram(DiagramDrawer &diagDrawer)
    {
    mPortionDrawer.drawGraph(diagDrawer);
    }

bool PortionDiagram::saveDiagram(File &file)
    {
    NameValueFile nameValFile;

    CompoundValue names;
    CompoundValue xPositions;
    CompoundValue yPositions;
    OovString drawingName;
    std::vector<PortionNode> const &nodes = getNodes();
    for(size_t i=0; i<nodes.size(); i++)
        {
        names.addArg(nodes[i].getName());

        OovString num;
        num.appendInt(getNodePosition(i).x);
        xPositions.addArg(num);

        num.clear();
        num.appendInt(getNodePosition(i).y);
        yPositions.addArg(num);
        }

    DiagramStorage::setDrawingHeader(nameValFile, DST_Portion, getCurrentClassName());
    nameValFile.setNameValue("Names", names.getAsString());
    nameValFile.setNameValue("XPositions", xPositions.getAsString());
    nameValFile.setNameValue("YPositions", yPositions.getAsString());
    return nameValFile.writeFile(file);
    }

bool PortionDiagram::loadDiagram(File &file, DiagramDrawer &diagDrawer)
    {
    NameValueFile nameValFile;
    bool success = nameValFile.readFile(file);
    if(success)
        {
        CompoundValue names;
        names.parseString(nameValFile.getValue("Names"));
        CompoundValue xPositions;
        xPositions.parseString(nameValFile.getValue("XPositions"));
        CompoundValue yPositions;
        yPositions.parseString(nameValFile.getValue("YPositions"));

        eDiagramStorageTypes drawingType;
        OovString drawingName;
        DiagramStorage::getDrawingHeader(nameValFile, drawingType, drawingName);
        clearGraphAndAddClass(diagDrawer, drawingName);
        for(size_t i=0; i<names.size(); i++)
            {
            std::vector<PortionNode> const &nodes = getNodes();
            OovString name = names[i];
            auto nodeIter = std::find_if(nodes.begin(), nodes.end(),
                [&name](PortionNode const &node)
                { return(name == node.getName()); });
            if(nodeIter != nodes.end())
                {
                int i = nodeIter - nodes.begin();
                int x=0;
                int y=0;
                xPositions[i].getInt(0, INT_MAX, x);
                yPositions[i].getInt(0, INT_MAX, y);
                setPosition(i, GraphPoint(x, y));
                }
            }
        }
    return success;
    }
