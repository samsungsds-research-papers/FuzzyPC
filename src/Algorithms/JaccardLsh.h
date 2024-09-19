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
#include <unordered_set>

namespace fuzzypc::algorithms {
/**
 * @brief
 *
 */
class JaccardLsh {
   public:
    /**
     * @brief Construct a new JaccordLsh object
     *
     * @param hashBitSize
     * @param numHashPerBand
     * @param numBands
     */
    JaccardLsh(uint64_t hashBitSize, uint64_t numHashPerBand,
               uint64_t numBands);

    /**
     * @brief Destroy the JaccordLsh object
     *
     */
    ~JaccardLsh() = default;

    /**
     * @brief
     *
     * @param input
     * @param shingleLens
     * @param seed
     * @return std::vector<uint64_t>
     */
    std::vector<uint64_t> apply(const std::string &input,
                                const uint32_t &shingleLens,
                                const uint64_t &seed);
    
    /**
     * @brief 
     * 
     * @param input1 
     * @param input2 
     * @return double 
     */
    static double jaccord(const std::vector<std::string> &input1, const std::vector<std::string> &input2);

    /**
     * @brief 
     * 
     * @param input1 
     * @param input2 
     * @return double 
     */
    static double jaccord(const std::unordered_set<std::string> &input1, const std::unordered_set<std::string> &input2);

    /**
     * @brief
     *
     * @param input1
     * @param input2
     * @param shingleLen
     * @return double
     */
    static double jaccord(const std::string &input1, const std::string &input2,
                   const uint32_t shingleLen);

   private:
    /**
     * @brief
     *
     * @param input
     * @param shingleLen
     * @param seed
     * @return uint64_t
     */
    uint64_t minHash(const std::string &input, const uint32_t shingleLen,
                     const uint64_t &seed);

    uint64_t mHashBitSize = 0, mNumHashPerBand = 0, mNumBands = 0;
};
}  // namespace pprl::algorithms