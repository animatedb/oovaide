/*
 * IncludeGenes.h
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDEGENES_H_
#define INCLUDEGENES_H_

#include "FastGene.h"
#include "IncludeDrawer.h"

/// This defines functionality to use a genetic algorithm used to layout the
/// node positions for the include diagram. The genetic algorithm will place
/// the objects with minimally overlapping lines.
/// @todo - and nodes that have more relations are closer to each other?
class IncludeGenes:public GenePool
    {
    public:
	void initialize(IncludeDrawer &drawer, size_t nodeHeight);
	// Do some iterations, and move the positions from the gene pool into
        // the drawer.
	void updatePositionsInDrawer();

    private:
	IncludeDrawer *mDrawer;
	size_t mNodeHeight;
	size_t mMaxDrawingHeight;
	size_t mMaxDistanceQ;
	size_t mMaxOverlapQ;
	size_t mMaxHeightQ;
	void setupQualityEachGeneration() override;
	QualityType calculateSingleGeneQuality(size_t geneIndex) const override;
        GeneValue getYPosition(size_t geneIndex, size_t nodeIndex) const;
        void setYPosition(size_t geneIndex, size_t nodeIndex, GeneValue val);
        size_t getDrawingHeight(size_t geneIndex) const;
        size_t getNodeOverlapCount(size_t geneIndex) const;
        size_t getNodeYDistances(size_t geneIndex) const;
        size_t getMaxNodesInColumn() const;
        bool nodesOverlap(size_t geneIndex, size_t node1, size_t node2) const;
    };

#endif
