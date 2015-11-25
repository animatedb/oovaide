/*
 * IncludeGraph.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeGraph.h"
#include "Debug.h"
#include "Project.h"
#include <algorithm>


IncludeNode const *IncludeGraph::getNode(OovStringRef name, IncludeNodeTypes nt) const
    {
    IncludeNode const *retNode = nullptr;
    for(auto const &node : mNodes)
        {
        if(node.getNodeType() == nt && OovString(node.getName()) == name.getStr())
            {
            retNode = &node;
            break;
            }
        }
    return retNode;
    }

void IncludeGraph::clearAndAddInclude(OovStringRef incName)
    {
    clearGraph();
    addSuppliers(incName);
    }

size_t IncludeGraph::getNodeIndex(OovStringRef name) const
    {
    size_t nodeIndex = NO_INDEX;
    for(size_t i=0; i<mNodes.size(); i++)
        {
        if(mNodes[i].getName().compare(name) == 0)
            {
            nodeIndex = i;
            break;
            }
        }
    return nodeIndex;
    }

void IncludeGraph::addNode(OovStringRef name)
    {
    if(getNodeIndex(name) == NO_INDEX)
        {
        mNodes.push_back(IncludeNode(name, INT_Project));
        }
    }

void IncludeGraph::removeNode(OovStringRef name)
    {
    size_t index = getNodeIndex(name);
    if(index != NO_INDEX)
        {
        mNodes.erase(mNodes.begin() + index);
        }
    updateConnections();
    }

void IncludeGraph::addSuppliers(OovStringRef consName)
    {
    std::set<IncludedPath> incFiles;
    mIncludeMap->getImmediateIncludeFilesUsedBySourceFile(consName, incFiles);
    addNode(consName);
    for(auto const &incFile : incFiles)
        {
        addNode(incFile.getFullPath());
        }
    updateConnections();
    }

void IncludeGraph::addConsumers(OovStringRef incName)
    {
    addNode(incName);
    for(auto const &consFile : mIncludeMap->getNameValues())
        {
        std::set<IncludedPath> supFiles;
        mIncludeMap->getImmediateIncludeFilesUsedBySourceFile(
            consFile.first, supFiles);
        for(auto const &supFile : supFiles)
            {
            if(supFile.getFullPath().compare(incName) == 0)
                {
                addNode(consFile.first);
                break;
                }
            }
        }
    updateConnections();
    }

void IncludeGraph::updateConnections()
    {
    mConnections.clear();
    for(auto const &consNode : mNodes)
        {
        std::set<IncludedPath> incFiles;
        mIncludeMap->getImmediateIncludeFilesUsedBySourceFile(
            consNode.getName(), incFiles);
        for(auto const &supNode : incFiles)
            {
            size_t consIndex = getNodeIndex(consNode.getName());
            size_t supIndex = getNodeIndex(supNode.getFullPath());
            if(supIndex != NO_INDEX && consIndex != NO_INDEX)
                {
                IncludeConnection conn(supIndex, consIndex);
                mConnections.push_back(conn);
                }
            }
        }
    }
