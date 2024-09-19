/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "CountVectorizer.h"

#include <math.h>

#include "Common/Utils.h"

using namespace std;
using namespace fuzzypc::common;

namespace fuzzypc::algorithms {
std::set<std::string> CountVectorizer::getKeys() { return mKeys; }

void CountVectorizer::setKeys(std::set<std::string>& keys) { mKeys = keys; }

void CountVectorizer::fit(vector<string> inputs) {
    size_t numRows = inputs.size();
    for (size_t i = 0; i < numRows; i++) {
        string remained = remainAlphaAndDigit(inputs[i]);
        size_t remainedSize = remained.size();
        if (remainedSize > 0) {
            size_t numNgram = remainedSize - 1;
            for (size_t j = 0; j < numNgram; j++) {
                string ngram(remained.data() + j, 2);
                mKeys.insert(ngram);
            }
        }
    }
}

std::vector<double> CountVectorizer::transform(std::string input) {
    size_t idx = 0;
    unordered_map<string, size_t> map;
    for (auto key : mKeys) {
        map[key] = idx;
        idx++;
    }
    if (input == "") {
        return vector<double>(idx, 0.0);
    }
    vector<double> res(idx, 0.0);
    string removed = remainAlphaAndDigit(input);
    size_t removedSize = removed.size();
    size_t numNgram = removedSize - 1;
    for (size_t i = 0; i < numNgram; i++) {
        string ngram(removed.data() + i, 2);
        res[map[ngram]] += 1.0;
    }
    return res;
}

SparseVector CountVectorizer::transformToSparse(std::string input) {
    size_t idx = 0;
    unordered_map<string, size_t> map;
    for (auto key : mKeys) {
        map[key] = idx;
        idx++;
    }
    if (input == "") {
        return SparseVector();
    }
    vector<double> count(idx, 0.0);
    string removed = remainAlphaAndDigit(input);
    size_t removedSize = removed.size();
    size_t numNgram = removedSize - 1;
    for (size_t i = 0; i < numNgram; i++) {
        string ngram(removed.data() + i, 2);
        count[map[ngram]] += 1.0;
    }
    SparseVector result;
    for (size_t i = 0; i < count.size(); i++) {
        if (count[i] != 0.0) {
            result.insert(i, count[i]);
        }
    }
    return result;
}
}  // namespace fuzzypc::algorithms