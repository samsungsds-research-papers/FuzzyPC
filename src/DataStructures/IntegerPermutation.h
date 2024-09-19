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

#include <vector>

namespace fuzzypc::structures {
/**
 * @brief
 *
 */
class IntegerPermutation {
   public:
    /**
     * @brief Construct a new IntegerPermutation object
     *
     * @param minElement
     * @param maxElement
     */
    IntegerPermutation(size_t minElement, size_t maxElement);

    /**
     * @brief
     *
     * @param other
     * @return IntegerPermutation&
     */
    IntegerPermutation &operator=(const IntegerPermutation &other) = default;

    /**
     * @brief
     *
     * @return size_t
     */
    size_t size() const { return mMaxElement - mMinElement + 1; }

    /**
     * @brief
     *
     * @param from
     * @return size_t
     */
    size_t get(size_t from) const;

    /**
     * @brief
     *
     * @param from
     * @param to
     */
    void set(size_t from, size_t to);

    /**
     * @brief
     *
     * @return IntegerPermutation
     */
    IntegerPermutation inverse() const;

    /**
     * @brief
     *
     */
    void randomShuffle();

   private:
    size_t mMinElement, mMaxElement;

    std::vector<size_t> mData;
};
}  // namespace pprl::structures