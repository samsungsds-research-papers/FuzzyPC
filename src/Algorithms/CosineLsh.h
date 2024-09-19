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

#include <cstdint>
#include <string>
#include <vector>

#include "DataStructures/SparseVector.h"

using namespace fuzzypc::structures;

namespace fuzzypc::algorithms {
/**
 * @brief
 *
 */
class CosineLsh {
   public:
    /**
     * @brief Construct a new CosineLsh object
     *
     * @param vectorLen
     * @param numHashPerBand
     * @param numBands
     */
    CosineLsh(size_t vectorLen, size_t numHashPerBand, size_t numBands);

    /**
     * @brief Destroy the CosineLSH object
     *
     */
    ~CosineLsh() = default;

    /**
     * @brief 
     * 
     * @param seed 
     */
    void generate(uint64_t seed);

    /**
     * @brief
     *
     * @param vec
     * @param seed
     * @return std::vector<uint64_t>
     */
    std::vector<uint64_t> apply(const std::vector<double> vec, uint64_t seed);

    /**
     * @brief
     *
     * @param vec
     * @param seed
     * @return std::vector<uint64_t>
     */
    std::vector<uint64_t> apply(const SparseVector &vec, uint64_t seed);

    /**
     * @brief
     *
     * @param mat
     * @param seed
     * @return std::vector<std::vector<uint64_t>>
     */
    std::vector<std::vector<uint64_t>> apply(
        const std::vector<std::vector<double>> mat, uint64_t seed);

    /**
     * @brief
     *
     * @param vec1
     * @param vec2
     * @return double
     */
    static double cosine(const std::vector<double> &vec1,
                         const std::vector<double> &vec2);

    /**
     * @brief
     *
     * @param vec1
     * @param vec2
     * @return double
     */
    static double cosine(const SparseVector &vec1, const SparseVector &vec2);

   private:
    /**
     * @brief
     *
     * @param vec1
     * @param vec2
     * @return std::string
     */
    std::string projection(const std::vector<double> &vec1,
                           const std::vector<double> &vec2);
    
    /**
     * @brief 
     * 
     * @param vec1 
     * @param vec2 
     * @return std::string 
     */
    std::string projection(const SparseVector &vec1,
                           const std::vector<double> &vec2);

    size_t mVectorLen = 0, mNumHashPerBand = 0, mNumBands = 0, mNumThreads = 1;

    std::vector<std::vector<double>> mRandomVectors;
};
}  // namespace fuzzypc::algorithms