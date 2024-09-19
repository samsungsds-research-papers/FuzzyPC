/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2022 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#pragma once

#include "Defines.h"

#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
// #include <libOTe/Tools/CoeffCtx.h>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>

namespace fuzzypc::protocols {
/**
 * @brief
 *
 */
class OprfReceiver {
   public:
    /**
     * @brief Construct a new OprfReceiver object
     *
     * @param numThreads
     * @param timer
     */
    OprfReceiver(size_t numThreads = 1, oc::Timer *timer = nullptr);

    /**
     * @brief
     *
     * @param chl
     */
    void recvVectorOLE(size_t numInputs, oc::Socket &chl);

    /**
     * @brief
     *
     * @param inputs
     * @param inputBitSize
     * @param outputBitSize
     * @param chl
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> recv(std::vector<std::byte> &inputs,
                                oc::u32 inputBitSize, oc::u32 outputBitSize,
                                oc::Socket &chl);

   private:
    size_t mNumThreads = 1, mGamma = STAT_SEC_PARAM + 10, mGenerated = 0;

    double mEpsilon = OKVS_EXPANSION;

    oc::PRNG mPrng;

    oc::CoeffCtxGF128::Vec<oc::block> mVoleA, mVoleC;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc::protocols