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

#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <vector>

namespace fuzzypc::algorithms {
/**
 * @brief
 *
 */
class Okvs {
   public:
    /**
     * @brief
     *
     * @param numKeys
     * @param gamma
     * @param fieldByteSize
     * @param seed
     * @param numThreads
     */
    virtual void configure(size_t numKeys, size_t gamma, size_t fieldByteSize,
                           oc::block &seed, size_t numThreads) = 0;

    /**
     * @brief
     *
     * @param keys
     * @param values
     * @param keyByteSize
     * @param encoded
     */
    virtual void encode(const std::vector<std::byte> &keys,
                        const std::vector<std::byte> &values,
                        size_t keyByteSize,
                        std::vector<std::byte> &encoded) = 0;

    /**
     * @brief
     *
     * @param encoded
     * @param keys
     * @param keyByteSize
     * @param values
     */
    virtual void decode(const std::vector<std::byte> &encoded,
                        const std::vector<std::byte> &keys, size_t keyByteSize,
                        std::vector<std::byte> &values) = 0;
};
}  // namespace pprl::algorithms