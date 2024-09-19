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

#include <coproto/coproto.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Network/Channel.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>

#include "Defines.h"

namespace fuzzypc::protocols {
class OPRFSender {
   public:
    /**
     * @brief Construct a new OPRFSender object
     *
     * @param numThreads
     * @param timer
     */
    OPRFSender(size_t numThreads = 1, oc::Timer *timer = nullptr);

    /**
     * @brief
     *
     * @param chl
     */
    void sendVectorOLE(oc::Socket &chl);

    /**
     * @brief
     *
     * @param inputs
     * @param inputBitSize
     * @param outputBitSize
     * @param chl
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> send(std::vector<std::byte> &inputs,
                                oc::u32 inputBitSize, oc::u32 outputBitSize,
                                oc::Socket &chl);

    /**
     * @brief
     *
     * @param chl
     */
    void send(oc::Socket &chl);

    /**
     * @brief
     *
     * @param inputs
     * @param inputBitSize
     * @param outputBitSize
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> encode(std::vector<std::byte> &inputs,
                                  oc::u32 inputBitSize, oc::u32 outputBitSize);

   private:
    size_t mNumThreads = 1, mGamma = STAT_SEC_PARAM + 10, mGenerated,
           mReceiverNumInputs;

    double mEpsilon = OKVS_EXPANSION;

    oc::PRNG mPrng;

    oc::Timer *mTimer = nullptr;

    oc::block mSeeds, mDelta;

    oc::CoeffCtxGF128::Vec<oc::block> mVoleB;

    std::vector<std::byte> mKeys;
};
}  // namespace fuzzypc::protocols