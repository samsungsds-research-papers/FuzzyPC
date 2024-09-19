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

#include <random>

static const char alphaNumeric[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

namespace fuzzypc::common {
/**
 * @brief
 *
 * @return uint64_t
 */
inline uint64_t random64() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    return dist(gen);
}

/**
 * @brief
 *
 * @return uint8_t
 */
inline uint8_t random8() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint8_t> dist;
    return dist(gen);
}

/**
 * @brief
 *
 * @param start
 * @param length
 */
inline void randomBytes(std::byte *start, size_t length) {
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    std::random_device rd;
    std::mt19937 gen(rd());
    for (size_t i = 0; i < length; i++) {
        *(start + i) = static_cast<std::byte>(dist(gen));
    }
}

/**
 * @brief
 *
 * @param length
 * @return std::string
 */
inline std::string randomString(size_t length) {
    size_t numChar = 63;
    std::uniform_int_distribution<uint8_t> dist(0, numChar - 1);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::string result = "";
    for (size_t i = 0; i < length; i++) {
        result += alphaNumeric[dist(gen)];
    }
    return result;
}
}  // namespace fuzzypc::common