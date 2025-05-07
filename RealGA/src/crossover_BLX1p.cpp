#include "crossover.h"
#include "stat.h"

#include <vector>
#include <algorithm>
#include <iostream>


BLX1pCrossover::BLX1pCrossover(size_t chromosomeSize, float BLX_alpha) {
    mChromosomeSize = chromosomeSize;
    mBLX_alpha = BLX_alpha;
}

BLX1pCrossover::~BLX1pCrossover() {

}

void BLX1pCrossover::crossover(RealChromosome& a, RealChromosome& b, RealChromosome& offspring) {

    int num_nodes = mChromosomeSize / 2;
    std::vector<std::pair<float, float>> vec_a(num_nodes);
    std::vector<std::pair<float, float>> vec_b(num_nodes);

    for (int i = 0; i < num_nodes; ++i) {
        vec_a[i].first = a.gene[i * 2];
        vec_a[i].second = a.gene[i * 2 + 1];
        vec_b[i].first = b.gene[i * 2];
        vec_b[i].second = b.gene[i * 2 + 1];
    }

    std::sort(vec_a.begin(), vec_a.end(), [](pair<float, float> i, pair<float, float> j) {
        if (i.first != j.first) return i.first < j.first;
        else return i.second < j.second;
    });
    std::sort(vec_b.begin(), vec_b.end(), [](pair<float, float> i, pair<float, float> j) {
        if (i.first != j.first) return i.first < j.first;
        else return i.second < j.second;
    });

    int crossSite = Stat::randIndex(num_nodes);

    for (int i = 0; i < crossSite; ++i) {
        offspring.gene[i * 2] = vec_a[i].first;
        offspring.gene[i * 2 + 1] = vec_a[i].second;
    }
    for (int i = crossSite; i < num_nodes; ++i) {
        offspring.gene[i * 2] = vec_b[i].first;
        offspring.gene[i * 2 + 1] = vec_b[i].second;
    }

    for (int i = 0; i < num_nodes; ++i) {
        offspring.gene[i * 2] = offspring.gene[i * 2] + Stat::randUniform(-1 * mBLX_alpha, mBLX_alpha) * std::abs(vec_a[i].first - vec_b[i].first);
        offspring.gene[i * 2 + 1] = offspring.gene[i * 2 + 1] + Stat::randUniform(-1 * mBLX_alpha, mBLX_alpha) * std::abs(vec_a[i].second - vec_b[i].second);

        offspring.gene[i * 2] = (offspring.gene[i * 2] < 0) ? 0 : offspring.gene[i * 2];
        offspring.gene[i * 2] = (offspring.gene[i * 2] > 1199) ? 1199 : offspring.gene[i * 2];
        offspring.gene[i * 2 + 1] = (offspring.gene[i * 2 + 1] < 0) ? 0 : offspring.gene[i * 2 + 1];
        offspring.gene[i * 2 + 1] = (offspring.gene[i * 2 + 1] > 1199) ? 1199 : offspring.gene[i * 2 + 1];
    }
}
