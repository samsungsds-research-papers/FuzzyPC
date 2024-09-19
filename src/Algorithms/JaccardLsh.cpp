/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "JaccardLsh.h"

#include <assert.h>
#include <xxhash.h>
#include <algorithm>
#include <set>

using namespace std;

namespace fuzzypc::algorithms {
JaccardLsh::JaccardLsh(uint64_t hashBitSize, uint64_t numHashPerBand,
                       uint64_t numBands)
    : mHashBitSize(hashBitSize),
      mNumHashPerBand(numHashPerBand),
      mNumBands(numBands) {
    if (hashBitSize > 64) {
        throw invalid_argument("hash bit size is too large");
    }
}

vector<uint64_t> JaccardLsh::apply(const string &input,
                                   const uint32_t &shingleLen,
                                   const uint64_t &seed) {
    vector<uint64_t> res(mNumBands);
    vector<uint64_t> h(mNumHashPerBand);
    for (size_t i = 0; i < mNumBands; i++) {
        for (size_t j = 0; j < mNumHashPerBand; j++) {
            h[j] = minHash(input, shingleLen, seed + i * mNumHashPerBand + j);
        }
        res[i] = XXH3_64bits(h.data(), 8 * mNumHashPerBand);
    }
    return res;
}

double JaccardLsh::jaccord(const std::vector<std::string> &input1,
                           const std::vector<std::string> &input2) {
    vector<string> result(input1.size() + input2.size());
    auto iter = set_intersection(input1.begin(), input1.end(), input2.begin(),
                                 input2.end(), result.begin());
    size_t numItemsInIntersection = iter - result.begin();
    size_t numItemsInUnion =
        input1.size() + input2.size() - numItemsInIntersection;
    return (double)numItemsInIntersection / (double)numItemsInUnion;
}

double JaccardLsh::jaccord(const unordered_set<string> &input1,
                           const unordered_set<string> &input2) {
    auto unionSet = input1;
    unionSet.insert(input2.begin(), input2.end());
    size_t numItemsInUnion = unionSet.size();
    size_t numItemsInIntersection =
        input1.size() + input2.size() - numItemsInUnion;
    return (double)numItemsInIntersection / (double)numItemsInUnion;
}

double JaccardLsh::jaccord(const string &input1, const string &input2,
                           const uint32_t shingleLen) {
    set<string> set1, set2;
    for (size_t i = 0; i < input1.length() - shingleLen + 1; i++) {
        set1.insert(input1.substr(i, shingleLen));
    }
    for (size_t i = 0; i < input2.length() - shingleLen + 1; i++) {
        set2.insert(input2.substr(i, shingleLen));
    }
    vector<string> unionSet(set1.size() + set2.size());
    auto it = set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                        unionSet.begin());
    unionSet.resize(it - unionSet.begin());
    size_t numItemsInUnion = unionSet.size();
    size_t numItemsInIntersection = set1.size() + set2.size() - unionSet.size();
    return (double)numItemsInIntersection / (double)numItemsInUnion;
}

uint64_t JaccardLsh::minHash(const string &input, const uint32_t shingleLen,
                             const uint64_t &seed) {
    uint64_t maxHash = (1ULL << mHashBitSize) - 1;
    int numShingles = input.length() - shingleLen + 1;
    uint64_t res = maxHash;
    for (size_t i = 0; i < numShingles; i++) {
        uint64_t h =
            XXH3_64bits_withSeed(&input[i], shingleLen, seed) & maxHash;
        if (h < res) {
            res = h;
        }
    }
    return res;
}
}  // namespace fuzzypc::algorithms