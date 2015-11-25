/*
 * IncludeGraph.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ModelObjects.h"
#include "OovString.h"
#include "IncludeMap.h"


// An include graph is an graph of include dependencies.

#ifndef INCLUDE_GRAPH_H
#define INCLUDE_GRAPH_H


class IncludeDrawOptions
    {
    };

enum IncludeNodeTypes
    {
    INT_Project
//    INT_System
    };

class IncludeNode
    {
    public:
        IncludeNode(OovStringRef name, IncludeNodeTypes type):
            mName(name), mNodeType(type)
            {}
        OovString const &getName() const
            { return mName; }
        IncludeNodeTypes getNodeType() const
            { return mNodeType; }

    private:
        OovString mName;
        IncludeNodeTypes mNodeType;
    };

struct IncludeConnection
    {
    IncludeConnection(size_t supplierNodeIndex, size_t consumerNodeIndex):
        mSupplierNodeIndex(supplierNodeIndex), mConsumerNodeIndex(consumerNodeIndex)
        {}
    size_t mSupplierNodeIndex;
    size_t mConsumerNodeIndex;
    };

typedef std::vector<IncludeConnection> IncludeConnections;

class IncludeGraph
    {
    public:
        IncludeGraph():
            mIncludeMap(nullptr)
            {}
        void clearGraph()
            {
            mNodes.clear();
            mConnections.clear();
            }
        // The source must exist for the lifetime of the graph.
        void setGraphDataSource(const IncDirDependencyMapReader &incMap)
            { mIncludeMap = &incMap; }
        void clearAndAddInclude(OovStringRef incName);
        void addSuppliers(OovStringRef incName);
        void addConsumers(OovStringRef incName);
        void removeNode(OovStringRef incName);
        IncludeNode const *getNode(OovStringRef name, IncludeNodeTypes nt) const;
        size_t getNodeIndex(IncludeNode const *node) const
            { return(static_cast<size_t>(node - &mNodes[0])); }
        std::vector<IncludeNode> const &getNodes() const
            { return mNodes; }
        IncludeConnections const &getConnections() const
            { return mConnections; }
// DEAD CODE
//        bool isModified() const;
//        IncludeDrawOptions const &getDrawOptions() const
//            { return mDrawOptions; }
//        void setDrawOptions(IncludeDrawOptions const &options);

    private:
        std::vector<IncludeNode> mNodes;
        IncludeConnections mConnections;
        const IncDirDependencyMapReader *mIncludeMap;
        IncludeDrawOptions mDrawOptions;

        static const size_t NO_INDEX = static_cast<size_t>(-1);
        size_t getNodeIndex(OovStringRef name) const;
        void addNode(OovStringRef name);
        void updateConnections();
    };

#endif
