/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#pragma once

#include <assert.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <stack>
#include <stdexcept>
#include <vector>

#include "DataStructures/IntegerPermutation.h"

using namespace fuzzypc::structures;

namespace fuzzypc::algorithms {
/**
 * @brief
 *
 */
class BenesNetwork {
   public:
    /**
     * @brief
     *
     */
    struct Switch {
        size_t in0 = 0, in1 = 0, out0 = 0, out1 = 0;
        int isSwitched = -1;
    };

    /**
     * @brief Construct a new Benes Network object
     *
     */
    BenesNetwork() = default;

    /**
     * @brief
     *
     * @param numInputs
     */
    void buildConnection(size_t numInputs) {
        numInputs_ = numInputs;
        numCols_ = 2 * ceil(log2(numInputs)) - 1;
        switches_.resize(numCols_);
        for (size_t i = 0; i < switches_.size(); i++) {
            switches_[i].resize(numInputs / 2);
        }
        buildConnectionInner(numInputs, 0, numCols_, 0, 0);
    }

    /**
     * @brief Get the number of valid switches
     *
     * @return size_t
     */
    size_t getNumSwitches() {
        size_t count = 0;
        for (size_t i = 0; i < switches_.size(); i++) {
            for (size_t j = 0; j < switches_[i].size(); j++) {
                if (switches_[i][j].isSwitched >= 0) {
                    count++;
                }
            }
        }
        return count;
    }

    /**
     * @brief
     *
     * @param perm
     */
    void routing(IntegerPermutation &perm) {
        auto permInv = perm.inverse();
        routingInner(perm, permInv, numInputs_, 0, numCols_, 0, 0);
    }

    /**
     * @brief
     *
     * @param in
     * @param out
     */
    void evaluate(const std::vector<uint64_t> &in, std::vector<uint64_t> &out) {
        out = in;
        for (size_t i = 0; i < switches_.size(); i++) {
            auto tmp = out;
            for (size_t j = 0; j < switches_[i].size(); j++) {
                if (switches_[i][j].isSwitched == 1) {
                    out[switches_[i][j].out1] = tmp[switches_[i][j].in0];
                    out[switches_[i][j].out0] = tmp[switches_[i][j].in1];
                } else if (switches_[i][j].isSwitched == 0) {
                    out[switches_[i][j].out0] = tmp[switches_[i][j].in0];
                    out[switches_[i][j].out1] = tmp[switches_[i][j].in1];
                }
            }
        }
    }

    /**
     * @brief
     *
     * @param numInputs
     * @param colIdxStart
     * @param colIdxEnd
     * @param inputOffset
     * @param switchOffset
     */
    void buildConnectionInner(size_t numInputs, size_t colIdxStart,
                              size_t colIdxEnd, size_t inputOffset,
                              size_t switchOffset);

    /**
     * @brief
     *
     * @param perm
     * @param permInv
     * @param numInputs
     * @param colIdxStart
     * @param colIdxEnd
     * @param inputOffset
     * @param switchOffset
     */
    void routingInner(IntegerPermutation &perm, IntegerPermutation &permInv,
                      size_t numInputs, size_t colIdxStart, size_t colIdxEnd,
                      size_t inputOffset, size_t switchOffset);

    size_t numInputs_ = 0, numCols_ = 0;

    std::vector<std::vector<Switch>> switches_;
};
}  // namespace fuzzypc::algorithms