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

#include "Algorithms/Okvs.h"
#include "VolePsi/Paxos.h"

namespace fuzzypc::algorithms {
/**
 * @brief 
 * 
 */
class Rr22Okvs : public Okvs {
   public:
    Rr22Okvs(oc::Timer *timer = nullptr) : mTimer(timer) {}

    void configure(size_t numKeys, size_t gamma, size_t fieldByteSize,
                   oc::block &seed, size_t numThreads = 1) override;

    size_t size() { return paxos_->size(); }

    oc::block getSeed() { return mSeed; }

    void encode(const std::vector<std::byte> &keys,
                const std::vector<std::byte> &values, size_t keyByteSize,
                std::vector<std::byte> &encoded) override;

    void decode(const std::vector<std::byte> &encoded,
                const std::vector<std::byte> &keys, size_t keyByteSize,
                std::vector<std::byte> &values) override;

    bool randomizePaxos = false;

   private:
    size_t fieldByteSize_ = 0, mNumThreads = 0;

    oc::block mSeed;

    oc::PRNG mPrng;

    std::shared_ptr<volePSI::Baxos> paxos_;

    oc::Timer *mTimer;
};
}  // namespace pprl::algorithms