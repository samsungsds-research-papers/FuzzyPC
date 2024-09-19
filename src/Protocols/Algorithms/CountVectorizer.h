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

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "DataStructures/SparseVector.h"

using namespace fuzzypc::structures;

namespace fuzzypc::algorithms {
/**
 * @brief
 *
 */
class CountVectorizer {
   public:
    /**
     * @brief Construct a new Count Vectorizer object
     *
     */
    CountVectorizer() = default;

    /**
     * @brief Get the Keys object
     *
     * @return std::set<std::string>
     */
    std::set<std::string> getKeys();

    /**
     * @brief Set the Keys object
     *
     * @param keys
     */
    void setKeys(std::set<std::string> &keys);

    /**
     * @brief
     *
     * @param inputs
     */
    void fit(std::vector<std::string> inputs);

    /**
     * @brief
     *
     * @param input
     * @return std::vector<double>
     */
    std::vector<double> transform(std::string input);

    /**
     * @brief
     *
     * @param input
     * @return SparseVector
     */
    SparseVector transformToSparse(std::string input);

   private:
    std::set<std::string> mKeys;
};
}  // namespace fuzzypc::algorithms