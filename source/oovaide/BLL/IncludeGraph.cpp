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

size_t IncludeGraph::addOrGetNode(OovStringRef name)
    {
    size_t index = getNodeIndex(name);
    if(index == NO_INDEX)
        {
        mNodes.push_back(IncludeNode(name, INT_Project));
        index = getNodeIndex(name);
        }
    return index;
    }

void IncludeGraph::addSuppliers(OovStringRef consName)
    {
    size_t consIndex = addOrGetNode(consName);
    std::set<IncludedPath> incFiles;
    mIncludeMap->getImmediateIncludeFilesUsedBySourceFile(consName, incFiles);
    for(auto const &incFile : incFiles)
        {
        size_t supIndex = addOrGetNode(incFile.getFullPath());
        if(supIndex != NO_INDEX)
            {
            IncludeConnection conn(supIndex, consIndex);
            mConnections.push_back(conn);
            }
        }
    }
