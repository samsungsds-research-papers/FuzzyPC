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

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "cryptoTools/Common/block.h"

namespace fuzzypc::common {
/**
 * @brief
 *
 * @param s
 * @return std::string
 */
std::string remainAlphaAndDigit(std::string s);

/**
 * @brief
 *
 * @param table
 * @param numRows
 * @return std::map<std::string, std::vector<std::string>>
 */
std::map<std::string, std::vector<std::string>> random(
    std::map<std::string, std::vector<std::string>> &table, size_t numRows,
    bool firstRows = true);

/**
 * @brief
 *
 * @param column
 * @param hashByteLen
 * @param dummy
 * @return std::vector<std::byte>
 */
std::vector<std::byte> hash(const std::vector<std::string> &column,
                            const uint32_t hashByteLen,
                            const std::vector<std::byte> &dummy);

/**
 * @brief
 *
 * @param table the input table
 * @param features the vector of feature names
 * @param removeNonAlphanumeric whether remove non alphanumeric charicters
 * @param spreadNan whether nan string "" is spreaded during comcatenation
 * @return std::vector<std::string>
 */
std::vector<std::string> concatenate(
    std::map<std::string, std::vector<std::string>> &table,
    std::vector<std::string> features, bool removeNonAlphanumeric = true,
    bool spreadNan = true);

/**
 * @brief
 *
 * @param table
 * @param features
 * @return std::vector<std::string>
 */
std::vector<std::vector<std::string>> concatenate(
    std::map<std::string, std::vector<std::string>> &table,
    std::vector<std::vector<std::string>> features,
    bool removeNonAlphanumeric = true, bool spreadNan = true);

/**
 * @brief
 *
 * @param column
 * @param deduplicated
 * @param indexMapFromDeduplicated
 */
void deduplication(const std::vector<std::string> &column,
                   std::vector<std::string> &deduplicated,
                   std::vector<size_t> &indexMapFromDeduplicated);

/**
 * @brief
 *
 * @param column
 * @param deduplicated
 * @param indexMapFromDeduplicated
 */
void deduplication(const std::vector<uint64_t> &column,
                   std::vector<uint64_t> &deduplicated,
                   std::vector<size_t> &indexMapFromDeduplicated);

/**
 * @brief
 *
 * @param trueLink
 * @param link
 * @param N
 */
void accuracy(std::unordered_map<size_t, size_t> &trueLink,
              std::unordered_map<size_t, size_t> &link, size_t N);

/**
 * @brief
 *
 * @param c
 * @return char
 */
char soundexEncodedChar(char c);

/**
 * @brief
 *
 * @return std::string
 */
std::string soundex(const std::string &input);

/**
 * @brief
 *
 * @param input1
 * @param input2
 * @return size_t
 */
size_t equality(const std::vector<std::string> &input1,
                const std::vector<std::string> &input2);

/**
 * @brief
 *
 * @param input1
 * @param input2
 * @return std::unordered_map<size_t, size_t>
 */
std::unordered_map<size_t, size_t> equality(
    const std::vector<std::vector<std::string>> &input1,
    const std::vector<std::vector<std::string>> &input2);

/**
 * @brief
 *
 * @param input1
 * @param input2
 * @return std::unordered_map<size_t, size_t>
 */
std::unordered_map<size_t, size_t> equality(
    const std::vector<std::vector<uint64_t>> &input1,
    const std::vector<std::vector<uint64_t>> &input2);

/**
 * @brief
 *
 * @param input1
 * @param input2
 * @return std::unordered_map<size_t, size_t>
 */
std::unordered_map<size_t, size_t> threshold1(
    const std::vector<std::vector<uint64_t>> &input1,
    const std::vector<std::vector<uint64_t>> &input2);

void preprocessColumn(const std::vector<std::byte> &rawItemsInByte,
                      const uint32_t itemSizeInByte,
                      std::vector<std::byte> &processedIntemsInByte,
                      std::vector<uint64_t> &idxItemToRow,
                      const std::vector<std::byte> &dummy);

std::map<std::string, std::vector<std::string>> readTable(
    const std::string &fileName, const char delimiter = ',',
    const bool hasHeader = true, const uint32_t maxNumRows = 0);

// void writeTable(
//     const std::vector<std::vector<uint64_t>> &table,
//     const std::string &fileName,
//     const std::vector<std::string> &header = std::vector<std::string>(),
//     const char delimiter = ',');

std::vector<std::string> concate1(
    const std::vector<std::string> &features,
    std::map<std::string, std::vector<std::string>> &rawTable);

std::vector<std::string> concate2(
    const std::vector<std::string> &features,
    std::map<std::string, std::vector<std::string>> &rawTable);

std::vector<std::byte> hashWithDummy(const std::vector<std::string> &column,
                                     const uint32_t hashSizeInByte,
                                     const osuCrypto::block dummyBlock);

std::unordered_map<uint32_t, uint32_t> findLink(
    const std::vector<std::string> &fromIDs,
    const std::vector<std::string> &toIDs);

std::vector<std::string> simulationPairWiseWithThreshold1(
    const std::vector<std::vector<std::string>> &myTable,
    const std::vector<std::vector<std::string>> &theirTable,
    const std::vector<std::string> &theirIDs);

std::vector<std::string> simulationAlg1WithThreshold1(
    const std::vector<std::vector<std::string>> &myTable,
    const std::vector<std::vector<std::string>> &theirTable,
    const std::vector<std::string> &theirIDs);

void checkAccuracy(std::unordered_map<uint32_t, uint32_t> &link,
                   const std::vector<std::string> &derivedIDs,
                   const std::vector<std::string> &theirIDs);
}  // namespace fuzzypc::common