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

#include <iostream>
#include <unordered_map>

#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <libOTe/Tools/Coproto.h>

#include "Algorithms/BenesNetwork.h"

using namespace fuzzypc::algorithms;

namespace fuzzypc::protocols {
/**
 * @brief A class for permute and share receiver. The receiver has input vector
 * information.
 *
 */
class PnsReceiver {
   public:
    /**
     * @brief Construct a new PnsReceiver object
     *
     * @param numThreads
     * @param timer
     */
    PnsReceiver(size_t numThreads = 1, oc::Timer *timer = nullptr);

    /**
     * @brief
     *
     * @param numInputs
     */
    void configure(size_t numInputs, size_t inputByteSize) {
        mNetwork.buildConnection(numInputs);
        mNumSwitches = mNetwork.getNumSwitches();
        mInputByteSize = inputByteSize;
    }

    std::vector<std::byte> run(std::vector<std::byte> &input, oc::Socket &chl);

    /**
     * @brief
     *
     * @param chl
     */
    void sendRandomOTs(oc::Socket &chl);

   private:
    void genRandomWires();

    void sendMaskedInputs(std::vector<std::byte> &input, oc::Socket &chl);

    void sendCorrection(oc::Socket &chl);

    void recvCorrection(oc::Socket &chl);

    size_t mNumThreads = 1, mNumSwitches = 0, mInputByteSize = 0;

    std::vector<std::array<std::array<oc::block, 2>, 2>> mRandomTwoMessages,
        mTwoMessages;

    std::vector<oc::block> mInputWires;

    std::vector<oc::block> mOutputWires;

    BenesNetwork mNetwork;

    oc::PRNG mPrng;

    oc::Timer *mTimer;
};
}  // namespace fuzzypc::protocols