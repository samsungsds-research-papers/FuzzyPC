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

#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuzzypc::common {
/**
 * @brief 
 * 
 * @param filePath path to the file
 * @param table 
 * @param columnNameIndex zero started row index of column names (default: 0)
 * @param rowNameIndex zero started column index of row names (default: -1)
 * @param delimiter delimiter
 * @return size_t the number of rows
 */
size_t readTable(const std::string &filePath,
                 std::map<std::string, std::vector<std::string>> &table,
                 const int columnNameIndex = 0, const int rowNameIndex = -1,
                 const char delimiter = ',');

/**
 * @brief
 *
 * @param table
 * @param filePath
 * @param header
 * @param delimiter
 */
void writeTable(const std::vector<std::vector<uint64_t>> &table,
                const std::string &filePath,
                const std::vector<std::string> &header,
                const char delimiter = ',');

/**
 * @brief
 *
 * @param table
 * @param filePath
 * @param header
 * @param delimiter
 */
void writeTable(const std::vector<std::vector<double>> &table,
                const std::string &filePath,
                const std::vector<std::string> &header,
                const char delimiter = ',');

}  // namespace fuzzypc::common