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

#include <math.h>
#include <map>
#include <unordered_map>
#include <vector>

namespace fuzzypc::structures {
/**
 * @brief
 *
 */
class SparseVector {
   public:
    /**
     * @brief Construct a new SparseVector object
     *
     */
    SparseVector() = default;

    /**
     * @brief Destroy the SparseVector object
     *
     */
    ~SparseVector() = default;

    /**
     * @brief
     *
     * @param index
     * @param value
     */
    void insert(size_t index, double value) { mNomZeroItems[index] = value; }

    /**
     * @brief
     *
     * @param index
     * @return true
     * @return false
     */
    bool find(size_t index) const {
        auto iter = mNomZeroItems.find(index);
        if (iter == mNomZeroItems.end()) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * @brief
     *
     * @param index
     * @return double
     */
    double at(size_t index) const { return mNomZeroItems.at(index); }

    /**
     * @brief
     *
     * @return double
     */
    double norm() const;

    /**
     * @brief
     *
     * @param rhs
     * @return double
     */
    double innerProduct(const SparseVector &rhs) const;

    /**
     * @brief
     *
     * @return std::vector<size_t>
     */
    std::vector<size_t> indices() const {
        std::vector<size_t> result;
        for (auto item : mNomZeroItems) {
            result.push_back(item.first);
        }
        return result;
    }

   private:
    std::unordered_map<size_t, double> mNomZeroItems;
};
}  // namespace fuzzypc::structures