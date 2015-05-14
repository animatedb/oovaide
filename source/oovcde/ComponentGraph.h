/*
 * ComponentGraph.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTGRAPH_H_
#define COMPONENTGRAPH_H_

#include <vector>
#include <set>
#include "Graph.h"	// For GraphRect
#include "Components.h"
#include "Packages.h"

struct ComponentDrawOptions
    {
    ComponentDrawOptions():
	drawImplicitRelations(false)
	{}
    bool drawImplicitRelations;
    };

/// This defines a component that can be in a component graph.
class ComponentNode
    {
    public:
	enum ComponentNodeTypes { CNT_Component, CNT_ExternalPackage };
	ComponentNode():
	    mComponentType(ComponentTypesFile::CT_Unknown),
	    mComponentNodeType(CNT_Component)
	    {}
	ComponentNode(OovStringRef const name, ComponentNodeTypes cnt,
		ComponentTypesFile::eCompTypes type=
			ComponentTypesFile::CT_Unknown):
	    mCompName(name), mComponentType(type), mComponentNodeType(cnt)
	    {
	    }
	void setPos(GraphPoint p)
	    { mRect.start = p;}
	void setSize(GraphSize s)
	    { mRect.size = s; }
	const GraphRect &getRect() const
	    { return mRect; }
	const OovString &getName() const
	    { return mCompName; }
	ComponentNodeTypes getComponentNodeType() const
	    { return mComponentNodeType; }
	ComponentTypesFile::eCompTypes getComponentType() const
	    { return mComponentType; }

    private:
	OovString mCompName;
	GraphRect mRect;
	ComponentTypesFile::eCompTypes mComponentType;
	ComponentNodeTypes mComponentNodeType;
    };

/// This defines a connection between two components.
class ComponentConnection
    {
    public:
	ComponentConnection(size_t nodeConsumer, size_t nodeSupplier):
	    mNodeConsumer(nodeConsumer), mNodeSupplier(nodeSupplier), mImpliedDependency(false)
	    {}
	size_t mNodeConsumer;
	size_t mNodeSupplier;
	uint32_t getAsInt() const
	    { return (mNodeConsumer<<16) + mNodeSupplier; }
	bool operator<(const ComponentConnection &np) const
	    { return getAsInt() < np.getAsInt(); }
	void setImpliedDependency(bool imp)
	    { mImpliedDependency = imp; }
	bool getImpliedDependency() const
	    { return mImpliedDependency; }

    private:
	bool mImpliedDependency;
    };

/// This defines functions used to interact with a component diagram. The
/// ComponentDiagram uses the ComponentDrawer to draw the ComponentGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ComponentGraph
    {
    public:
	ComponentGraph():
	    mModified(false)
	    {}
	void updateGraph(const ComponentDrawOptions &options);
	GraphSize getGraphSize() const;
	const std::vector<ComponentNode> &getNodes() const
	    { return mNodes; }
	std::vector<ComponentNode> &getNodes()
	    { return mNodes; }
	const std::set<ComponentConnection> &getConnections() const
	    { return mConnections; }
	ComponentNode *getNode(int x, int y);
	void removeNode(const ComponentNode &node, const ComponentDrawOptions &options);
	bool isModified() const
	    { return mModified; }
	void setModified()
	    { mModified = true; }

    private:
	std::vector<ComponentNode> mNodes;
	std::set<ComponentConnection> mConnections;
	bool mModified;
        static const size_t NO_INDEX = static_cast<size_t>(-1);

	// This finds include paths for all source files of each component.
	void updateConnections(const ComponentTypesFile &compFile,
		const ComponentDrawOptions &options);
	size_t getComponentIndex(OovStringVec const &compPaths,
		OovStringRef const dir);
	void pruneConnections();
	void findNumPaths(size_t consumerIndex, size_t supplierIndex, size_t &numPaths);
    };


#endif /* COMPONENTGRAPH_H_ */
