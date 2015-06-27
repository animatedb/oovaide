// \copyright 1992 DCBlaha.  Distributed under the GPL.

#include "FastGene.h"

#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <ctype.h>
#include <random>
#include <float.h>      // For DBL_MAX

/// Generate a random number including 0 to maxpossible
static size_t randmax(size_t maxpossible)
    {
    static std::default_random_engine generator;
    std::uniform_int_distribution<size_t> distribution(0, maxpossible);
    return distribution(generator);
    }

GeneValue GenePool::randRange(size_t min, size_t max)
    {
    return(static_cast<GeneValue>(randmax(max-min)+min));
    }

void GenePool::initialize(size_t genebytes, size_t numberofgenes, double crossoverrate,
    double mutaterate, GeneValue minrand, GeneValue maxrand)
    {
    // Copy the information we want to keep about all of the genes
    numgenes = numberofgenes;
    genesize = genebytes;
    muterate = mutaterate;
    min = minrand;
    max = maxrand;

    //  static_assert(crossoverrate < 0.5);     // Crossover rate must be less than .5, using .35.
    if(crossoverrate >= 0.5)
        {
        crossoverrate = 0.35;
        }
    size_t numbestgenes = static_cast<size_t>(numgenes * crossoverrate);
    if(numbestgenes & 1)        // Make numbestgenes an even number
        numbestgenes++;

    // Allocate space for the genes
    genes.resize((genesize + sizeof(QualityType)) * numgenes);
    // Allocate space to help in computing the crossover genes
    bestgenes.resize(numbestgenes);
    worstgenes.resize(numbestgenes);

    // Initialize the genes with random data
    for(size_t i=0; i<numgenes; i++)
        randomizeGene(i);
    }


void GenePool::randomizeGene(size_t index)
        {
        for(size_t i=0; i<genesize; i+=sizeof(GeneValue))
            {
            randomizeGeneValue(index, i);
            }
        }

void GenePool::randomizeGeneValue(size_t index, size_t offset)
    {
    offset = geneValueBoundary(offset);
    GeneValue val = randRange(min, max);
    setValue(index, offset, val);
    }

void GenePool::singleGeneration()
    {
    // Find quality values for all genes
    computeQuality();
    // Build lists of best and worst genes to aid on computing crossover
    buildBestWorstList();
    // Combine the genes that are the best and overwrite the worst ones
    crossover();
    // Mutate a certain number of genes
    mutate();
    }

void GenePool::computeQuality()
    {
    setupQualityEachGeneration();
    for(size_t i=0; i<numgenes; i++)
        {
        setGeneQuality(i, calculateSingleGeneQuality(i));
        }
    }

void GenePool::getQualityHistogram(QualityHistogram &qualities) const
    {
    QualityType maxquality = 0;
    for(size_t i=0; i<numgenes; i++)
        {
        QualityType q = getGeneQuality(i);
        if(q > maxquality)
            maxquality = q;
        }
    qualities.resize(maxquality+1);
    // Make a histogram of the qualities of the genes
    for(size_t i=0; i<numgenes; i++)
        {
        qualities[getGeneQuality(i)]++;
        }
    }

void GenePool::buildBestWorstList()
    {
    QualityType bestquality, worstquality;

    QualityHistogram qualities;
    getQualityHistogram(qualities);
    // Find the quality value to use to determine the worst genes
    size_t numQualityBins = 0;
    worstquality = static_cast<QualityType>(qualities.size() - 1);
    for (size_t i = 0; i < qualities.size() && worstquality == qualities.size() - 1; i++)
        {
        numQualityBins += qualities[i];
        if (static_cast<unsigned>(numQualityBins) > bestgenes.size())
            worstquality = static_cast<QualityType>(i);
        }

    // Find the quality value to use to determine the best genes
    numQualityBins = 0;
    bestquality = 0;
    for (int i = static_cast<int>(qualities.size() - 1);
        i >= 0 && bestquality == 0; i--)
        {
        numQualityBins += qualities[i];
        if (static_cast<unsigned>(numQualityBins) > bestgenes.size())
            {
            bestquality = static_cast<QualityType>(i);
            }
        }
//printf("worstcut %d bestcut %d\n", worstquality, bestquality);

    // Build a list of best genes
    int besti = 0;
    for (size_t i = 0; i < numgenes; i++)
        {
        if(getGeneQuality(i) >= bestquality)
            {
            if(static_cast<unsigned>(besti) < bestgenes.size())
                {
                bestgenes[besti++] = getGene(i);
                }
            else
                break;
            }
        }

    // Build a list of worst genes
    int worsti = 0;
    for (size_t i = 0; i < numgenes; i++)
        {
        if(getGeneQuality(i) <= worstquality)
            {
            if(static_cast<unsigned>(worsti) < worstgenes.size())
                {
                worstgenes[worsti++] = getGene(i);
                }
            else
                break;
            }
        }
    }

void GenePool::crossover()
    {
    size_t genesRemaining = bestgenes.size();
    size_t dstgene = 0;
    // Each time, take 2 good source genes, cross them, and put them
    // into the 2 worst genes.
    while(genesRemaining)
        {
        size_t splitpos = geneValueBoundary(randmax(
            static_cast<size_t>(genesize-1)));
        size_t srcgene1 = randmax(
            static_cast<size_t>(genesRemaining - 1));
        size_t srcgene2 = randmax(
            static_cast<size_t>(genesRemaining - 2));
        if (srcgene1 == srcgene2)
            {
            if (srcgene1 < genesRemaining - 1)
                ++srcgene1;
            else
                --srcgene1;
            }

        GenePtr srcgene1str = bestgenes[srcgene1];
        GenePtr srcgene2str = bestgenes[srcgene2];

        // Copy parts of 2 genes to 2 new genes.
        GenePtr dstgenestr = worstgenes[dstgene++];
        memcpy(dstgenestr, srcgene1str, splitpos);
        memcpy(dstgenestr + splitpos, srcgene2str + splitpos,
                genesize - splitpos);

        dstgenestr = worstgenes[dstgene++];
        memcpy(dstgenestr, srcgene2str, splitpos);
        memcpy(dstgenestr + splitpos, srcgene1str + splitpos,
                genesize - splitpos);

        /* Copy the genes at the end of the list into the genes that were
         * already crossed over since the list of remaining best genes is
         * getting shorter.
         */
        bestgenes[srcgene1] = bestgenes[genesRemaining - 1];
        bestgenes[srcgene2] = bestgenes[genesRemaining - 2];

        genesRemaining -= 2;
        }
    }

void GenePool::mutate()
    {
    size_t totalbytes = genesize * numgenes;
    size_t mutebits = static_cast<size_t>((muterate * 8) * totalbytes);
    for(size_t i = 0; i < mutebits; i++)
        {
        // Get the offset of a byte in any portion of any gene
        size_t gene = randmax(numgenes-1);
        size_t offset = randmax(genesize-1);
        randomizeGeneValue(gene, offset);

        /*
         // Get the address of a byte in any portion of any gene
         byteaddr = genes + (sizeof(GeneHeader) + genesize) *
         randmax(numgenes) + randmax(genesize) + sizeof(GeneHeader);

         // Invert one bit in the byte
         newval = (unsigned char)(*byteaddr ^ (1 << randmax(8)));
         if(newval >= min && newval <= max)
         *byteaddr = newval;
         */
        }
    }

size_t GenePool::getBestGeneIndex()
    {
    computeQuality();

    QualityHistogram qualities;
    getQualityHistogram(qualities);
    QualityType bestQ = 0;

    for(int i=static_cast<int>(qualities.size()-1); i>=0 && !bestQ; i--)
        {
        if(qualities[i])
            {
            bestQ = static_cast<QualityType>(i);
            }
        }
    // Most if the time, the first gene is the best.
    size_t bestGeneI = 0;
    for(size_t i=0; i<numgenes; i++)
        {
        if(getGeneQuality(i) == bestQ)
            {
            bestGeneI = i;
            break;
            }
        }
    return bestGeneI;
    }


#if(0)
#define GENEBYTES 10
// This will optimize for the gene with the highest average value of bytes.
// The randomizer, and crossover is working on GeneValue sized parts of each gene.
QualityType calculateQuality(int geneIndex)
    {
    int value = 0;
    for(size_t i=0; i<GENEBYTES; i++)
        value += getGene(geneIndex)[i];
    return(value / GENEBYTES);
    }
main(unsigned int argc, char *argv[])
    {
    double crossrate = 0.35;
    double muterate = 0.005;
    size_t iterations = 10;
    size_t numgenes = 100;
    GenePool genes;
    genes.initialize(GENEBYTES, numgenes, crossrate, muterate);

    for(size_t i=0; i<iterations; i++)
        {
        genes.singleGeneration();
        QualityHistogram qualities;
        genes.getQualityHistogram(qualities);
        size_t worst = 0, best = 0;

        for(size_t i=qualities.size()-1; i>=0 && !best; i--)
            {
            if(qualities[i])
                best = i;
            }
        for(size_t i=0; i<qualities.size()-1 && !worst; i++)
            {
            if(qualities[i])
                worst = i;
            }
        printf("worst %d best %d\n", worst, best);
        }
    }
#endif
