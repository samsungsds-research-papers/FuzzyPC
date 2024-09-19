/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "CosineLsh.h"

#include <assert.h>
#include <math.h>
#include <xxhash.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <thread>

#include "Common/Random.h"

using namespace std;

inline vector<double> getRandomVector(uint64_t seed, int size) {
    mt19937 gen;
    gen.seed(seed);
    normal_distribution<double> dist{0.0, 1.0};
    vector<double> res(size);
    double norm = 0.0;
    for (int i = 0; i < size; i++) {
        res[i] = dist(gen);
        norm += res[i] * res[i];
    }
    norm = sqrt(norm);
    for (int i = 0; i < size; i++) {
        res[i] /= norm;
    }
    return res;
}

namespace fuzzypc::algorithms {
CosineLsh::CosineLsh(size_t vectorLen, size_t numHashPerBand, size_t numBands)
    : mVectorLen(vectorLen),
      mNumHashPerBand(numHashPerBand),
      mNumBands(numBands) {}

void CosineLsh::generate(uint64_t seed) {
    mRandomVectors.resize(mNumHashPerBand * mNumBands);
    for (size_t i = 0; i < mRandomVectors.size(); i++) {
        mRandomVectors[i] = getRandomVector(seed + i, mVectorLen);
    }
}

vector<uint64_t> CosineLsh::apply(const vector<double> vec, uint64_t seed) {
    vector<uint64_t> res;
    for (size_t i = 0; i < mNumBands; i++) {
        string hashIn;
        for (size_t j = 0; j < mNumHashPerBand; j++) {
            hashIn += projection(vec, mRandomVectors[i * mNumHashPerBand + j]);
        }
        res.push_back(XXH64(hashIn.c_str(), hashIn.size(), seed));
    }
    return res;
}

vector<uint64_t> CosineLsh::apply(const SparseVector& vec, uint64_t seed) {
    vector<uint64_t> res;
    for (size_t i = 0; i < mNumBands; i++) {
        string hashIn;
        for (size_t j = 0; j < mNumHashPerBand; j++) {
            hashIn += projection(vec, mRandomVectors[i * mNumHashPerBand + j]);
        }
        res.push_back(XXH64(hashIn.c_str(), hashIn.size(), seed));
    }
    return res;
}

vector<vector<uint64_t>> CosineLsh::apply(const vector<vector<double>> mat,
                                          uint64_t seed) {
    size_t numRows = mat.size();
    size_t numRowsInThread = (numRows + mNumThreads - 1) / mNumThreads;

    vector<vector<uint64_t>> result(numRows);
    vector<thread> worker;
    for (size_t i = 0; i < mNumThreads; i++) {
        worker.push_back(thread([&]() {
            size_t start = i * numRowsInThread;
            size_t end = start + numRowsInThread;
            end = min(numRows, end);
            for (size_t j = start; j < end; j++) {
                result[i] = apply(mat[j], seed);
            }
        }));
    }
    for (size_t i = 0; i < mNumThreads; i++) {
        worker[i].join();
    }

    return result;
}

double CosineLsh::cosine(const vector<double>& vec1,
                         const vector<double>& vec2) {
    if (vec1.size() != vec2.size()) {
        invalid_argument("vec1.size() != vec2.size()");
    }
    double norm1 = 0.0, norm2 = 0.0, ip = 0.0;
    for (size_t i = 0; i < vec1.size(); i++) {
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
        ip += vec1[i] * vec2[i];
    }
    norm1 = sqrt(norm1);
    norm2 = sqrt(norm2);
    return ip / (norm1 * norm2);
}

double CosineLsh::cosine(const SparseVector& vec1, const SparseVector& vec2) {
    double ip = vec1.innerProduct(vec2);
    double norm1 = vec1.norm();
    double norm2 = vec2.norm();
    return ip / (norm1 * norm2);
}

string CosineLsh::projection(const vector<double>& vec1,
                             const vector<double>& vec2) {
    if (vec1.size() != vec2.size()) {
        invalid_argument("vec1.size() != vec2.size()");
    }
    double ip = 0.0;
    for (size_t i = 0; i < vec1.size(); i++) {
        ip += vec1[i] * vec2[i];
    }
    if (ip > 0) {
        return "1";
    } else {
        return "0";
    }
}

std::string CosineLsh::projection(const SparseVector& vec1,
                                  const std::vector<double>& vec2) {
    double ip = 0.0;
    auto indices = vec1.indices();
    for (auto index : indices) {
        ip += vec1.at(index) * vec2[index];
    }
    if (ip > 0) {
        return "1";
    } else {
        return "0";
    }
}

}  // namespace fuzzypc::algorithms