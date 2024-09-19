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

#include <string>
#include <vector>

namespace fuzzypc {
class FuzzyPc {
   public:
    /**
     * @brief
     *
     * @param column
     * @param hashByteSize
     * @param deduplicatedToRows
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> dedupAndHash(
        const std::vector<std::string>& column, const size_t hashByteSize,
        std::vector<size_t>& deduplicatedToRows);

    /**
     * @brief
     *
     * @param column
     * @param hashByteSize
     * @param deduplicatedToRows
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> dedupAndHash(
        const std::vector<uint64_t>& column, const size_t hashByteSize,
        std::vector<size_t>& deduplicatedToRows);
};
}  // namespace fuzzypc