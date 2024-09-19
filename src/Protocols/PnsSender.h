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
 * @brief A class for permute and share sender. The sender has permutation
 * information.
 *
 */
class PnsSender {
   public:
    /**
     * @brief Construct a new PnsSender object
     *
     * @param numThreads
     * @param timer
     */
    PnsSender(size_t numThreads = 1, oc::Timer *timer = nullptr);

    /**
     * @brief Configure with the given parameters
     *
     * @param numInputs the number of inputs
     * @param inputByteSize an input byte size
     */
    void configure(size_t numInputs, size_t inputByteSize) {
        mNetwork.buildConnection(numInputs);
        mNumSwitches = mNetwork.getNumSwitches();
        mInputByteSize = inputByteSize;
    }

    /**
     * @brief Run Permute And Share protocol
     *
     * @param permutation a permutation
     * @param chl a network socket for send and recv data
     * @return std::vector<std::byte> the result of protocol
     */
    std::vector<std::byte> run(IntegerPermutation &permutation,
                               oc::Socket &chl);

    /**
     * @brief Do random OT protocol which is independent to input
     *
     * @param chl
     */
    void recvRandomOTs(oc::Socket &chl);

   private:
    /**
     * @brief Do correction for choices
     *
     * @param chl
     */
    void sendCorrection(oc::Socket &chl);

    /**
     * @brief Do correction for messages
     *
     * @param chl
     */
    void recvCorrection(oc::Socket &chl);

    /**
     * @brief Oblivious evaluation of permutation for the given inputs as vector
     * of blocks
     *
     * @param input
     * @param output
     */
    void evaluateBlock(std::vector<oc::block> &input,
                       std::vector<oc::block> &output);

    /**
     * @brief Oblivious evaluation of permutation for the given inputs as vector
     * of bytes
     *
     * @param input
     * @param output
     */
    void evaluateBytes(std::vector<std::byte> &input,
                       std::vector<std::byte> &output);

    size_t mNumThreads = 1, mNumSwitches = 0, mInputByteSize = 0;

    oc::BitVector mRandomChoices;

    std::vector<std::array<oc::block, 2>> mRandomMessages, mMessages;

    BenesNetwork mNetwork;

    oc::PRNG mPrng;

    oc::Timer *mTimer;
};
}  // namespace fuzzypc::protocols