#pragma once

#include <set>
#include <string>
#include <thread>
#include <vector>

#include "Algorithms/AngularLsh.h"
#include "Algorithms/CountVectorizer.h"
#include "Algorithms/JaccardLsh.h"

using namespace std;
using namespace fuzzypc::algorithms;

vector<vector<uint64_t>> applyJaccardLSH(const vector<string> &concated,
                                         size_t numHashPerBand,
                                         size_t numBands) {
    size_t numRows = concated.size();
    vector<vector<uint64_t>> result(numRows);
    JaccardLsh lsh(32, numHashPerBand, numBands);

    int numThreads = 8;
    size_t numRowsInThread = (numRows + numThreads - 1) / numThreads;
    vector<thread> th;
    for (size_t i = 0; i < numThreads; i++) {
        th.push_back(thread([i, numRowsInThread, numRows, &concated, &result,
                             &lsh]() {
            size_t start = i * numRowsInThread;
            size_t end = start + numRowsInThread;
            end = std::min(numRows, end);
            for (size_t j = start; j < end; j++) {
                std::string jth = concated[j];
                jth.erase(std::remove(jth.begin(), jth.end(), ' '), jth.end());
                result[j] = lsh.apply(jth, 2, 0);
            }
        }));
    }
    for (size_t i = 0; i < numThreads; i++) {
        th[i].join();
    }
    return result;
}

vector<vector<uint64_t>> applyAngularLSH(const vector<string> &concated,
                                        size_t numHashPerBand,
                                        size_t numBands) {
    set<string> mergedKeys;
    string candidates = "abcdefghijklmnopqrstuvwzxy0123456789";
    string qgram = "00";
    for (size_t i = 0; i < candidates.size(); i++) {
        qgram[0] = candidates[i];
        for (size_t j = 0; j < candidates.size(); j++) {
            qgram[1] = candidates[j];
            mergedKeys.insert(qgram);
        }
    }

    CountVectorizer cv;
    cv.setKeys(mergedKeys);

    vector<SparseVector> transformed(concated.size());
    for (size_t i = 0; i < concated.size(); i++) {
        transformed[i] = cv.transformToSparse(concated[i]);
    }

    size_t numRows = concated.size();
    vector<vector<uint64_t>> result(numRows);
    AngularLsh lsh(mergedKeys.size(), numHashPerBand, numBands);
    lsh.generate(123123);

    int numThreads = 8;
    size_t numRowsInThread = (numRows + numThreads - 1) / numThreads;
    vector<thread> th;
    for (size_t i = 0; i < numThreads; i++) {
        th.push_back(thread(
            [i, numRowsInThread, numRows, &transformed, &result, &lsh]() {
                size_t start = i * numRowsInThread;
                size_t end = start + numRowsInThread;
                end = std::min(numRows, end);
                for (size_t j = start; j < end; j++) {
                    SparseVector jth = transformed[j];
                    result[j] = lsh.apply(jth, 456456);
                }
            }));
    }
    for (size_t i = 0; i < numThreads; i++) {
        th[i].join();
    }
    return result;
}