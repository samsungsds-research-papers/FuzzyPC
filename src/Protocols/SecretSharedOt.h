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

#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <libOTe/Tools/Coproto.h>

#include "Common/Random.h"
#include "Defines.h"

namespace fuzzypc::protocols {
/**
 * @brief A class for "Secret-shared OT" protocol. To reduce the number of base
 * OTs, this class should be re-used at several "run" function call. For that,
 * enought number of random OTs should be generated before.
 *
 */
class SecretSharedOT {
   public:
    /**
     * @brief Construct a new Secret Shared OT object
     *
     * @param isSender
     * @param numThreads
     * @param timer
     */
    SecretSharedOT(bool isSender, size_t numThreads = 1,
                   oc::Timer *timer = nullptr)
        : mIsSender(isSender), mNumThreads(numThreads), mTimer(timer) {
        mPrng.SetSeed(oc::toBlock(fuzzypc::common::random64()));
    };

    std::vector<std::byte> run(
        const oc::BitVector &choiceShares,
        const std::array<std::vector<std::byte>, 2> &messageShares,
        size_t messageByteSize, oc::Socket &chl);

    /**
     * @brief Generate random OTs. OT messages as sender and receiver are both
     * needed for a single secret shared OT call.
     *
     * @param numOTs The number of required secret shared OTs
     * @param chl a network socekt
     */
    void preprocess(size_t numOTs, oc::Socket &chl);

   private:
    bool mIsSender;

    size_t mNumThreads = 1, mRandomOtIdx = 0, mRemainRandomOt = 0;

    oc::PRNG mPrng;

    std::vector<std::array<oc::block, 2>> mRandomTwoMessages;

    oc::BitVector mRandomChoices;

    std::vector<oc::block> mRandomMessages;

    oc::Timer *mTimer = nullptr;

    /**
     * @brief Partial OT process in secret shared OT. In this partial OTs, only
     * the choices are secret shared status. By using this process twice, secret
     * shared OT can be acheived.
     *
     * @param choiceShares
     * @param msgs
     * @param msgByteSize
     * @param outputs
     * @param chl
     */
    void sendPartialOTs(const oc::BitVector &choiceShares,
                        const std::vector<std::array<oc::block, 2>> &msgs,
                        size_t msgByteSize, std::vector<oc::block> &outputs,
                        oc::Socket &chl);

    /**
     * @brief Partial OT process in secret shared OT. In this partial OTs, only
     * the choices are secret shared status. By using this process twice, secret
     * shared OT can be acheived.
     *
     * @param choiceShares
     * @param msgByteSize
     * @param outputs
     * @param chl
     */
    void recvPartialOTs(const oc::BitVector &choiceShares, size_t msgByteSize,
                        std::vector<oc::block> &outputs, oc::Socket &chl);

    /**
     * @brief Send numOTs number of random OTs
     *
     * @param numOTs
     * @param chl
     */
    void sendRandomOTs(size_t numOTs, oc::Socket &chl);

    /**
     * @brief Receive numOTs number of random OTs. The randomChoices are also
     * random which can not be selected by user.
     *
     * @param numOTs
     * @param chl
     */
    void recvRandomOTs(size_t numOTs, oc::Socket &chl);
};
}  // namespace fuzzypc::protocols