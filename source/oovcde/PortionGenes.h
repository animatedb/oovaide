/*
 * PortionGenes.h
 * Created on: April 29, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTIONGENES_H_
#define PORTIONGENES_H_

#include "FastGene.h"
#include "PortionDrawer.h"

/// This defines functionality to use a genetic algorithm used to layout the
/// node positions for the portion diagram. The genetic algorithm will place
/// the objects with minimally overlapping lines.
/// @todo - and nodes that have more relations are closer to each other?
class PortionGenes:public GenePool
    {
    public:
	void initialize(PortionDrawer &drawer, int nodeHeight);
	// Do some iterations, and move the positions from the gene pool into
        // the drawer.
	void updatePositionsInDrawer();

    private:
	PortionDrawer *mDrawer;
	int mNodeHeight;
	int mMaxDrawingHeight;
	int mMaxDistanceQ;
	int mMaxOverlapQ;
	int mMaxHeightQ;
	void setupQualityEachGeneration() override;
	QualityType calculateSingleGeneQuality(int geneIndex) const override;
        GeneValue getYPosition(int geneIndex, int nodeIndex) const;
        void setYPosition(int geneIndex, int nodeIndex, GeneValue val);
        int getDrawingHeight(int geneIndex) const;
        int getNodeOverlapCount(int geneIndex) const;
        int getNodeYDistances(int geneIndex) const;
        int getMaxNodesInColumn() const;
        bool nodesOverlap(int geneIndex, int node1, int node2) const;
    };

#endif
