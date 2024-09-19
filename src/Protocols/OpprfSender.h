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

#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <libOTe/Tools/Coproto.h>
#include <vector>

#include "Defines.h"
#include "OprfSender.h"

namespace fuzzypc::protocols {
/**
 * @brief A class for OPPRF protocol (sender)
 *
 */
class OPPRFSender {
   public:
    /**
     * @brief Construct a new OPPRFSender object
     *
     * @param numThreads
     * @param timer
     */
    OPPRFSender(size_t numThreads = 1, oc::Timer *timer = nullptr);

    /**
     * @brief
     *
     * @param inputs
     * @param values
     * @param inputByteSize
     * @param valueByteSize
     * @param chl
     */
    void send(std::vector<std::byte> &inputs, std::vector<std::byte> &values,
              size_t inputByteSize, size_t valueByteSize, oc::Socket &chl);

    void sendVectorOLE(oc::Socket &chl);

   private:
    size_t mGamma = STAT_SEC_PARAM + 10, mNumThreads = 1;

    double mEpsilon = OKVS_EXPANSION;

    std::vector<std::byte> mKeys, mMaskedOkvs;

    osuCrypto::block mSeed;

    osuCrypto::PRNG mPrng;

    osuCrypto::Timer *mTimer;

    std::shared_ptr<OPRFSender> mOprfSenderPtr;
};
}  // namespace fuzzypc::protocols