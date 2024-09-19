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

#include <coproto/coproto.h>
#include <cryptoTools/Network/Channel.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>

namespace fuzzypc::protocols {
/**
 * @brief Class for equality check between two parties.
 *
 */
class EqualityCheck {
   public:
    /**
     * @brief Construct a new EqualityCheck object
     *
     * @param isSender true if this party is sender at random OT protocol
     * @param numThreads a number of threads for slient OT protocol
     * @param timer a timer to check timing results
     */
    EqualityCheck(bool isSender, size_t numThreads = 1,
                  oc::Timer *timer = nullptr);

    /**
     * @brief Run equality check protocol between party 0 and 1
     *
     * @param inputs a vector of bytes that contains all inputs
     * @param inputBitSize a bit size of single input
     * @param chl a socket for network
     * @return oc::BitVector
     */
    oc::BitVector run(std::vector<std::byte> &inputs, size_t inputBitSize,
                      oc::Socket &chl);

    /**
     * @brief Get the number of OTs
     *
     * @param numInputs
     * @param inputBitSize
     * @return size_t
     */
    size_t getNumOTs(size_t numInputs, size_t inputBitSize) {
        return 2 * (inputBitSize - 1) * numInputs;
    }

    /**
     * @brief
     *
     * @param numOTs
     * @param chl
     * @return coproto::task<>
     */
    void sendRandomOTs(size_t numOTs, oc::Socket &chl);

    /**
     * @brief
     *
     * @param numOTs
     * @param chl
     * @return coproto::task<>
     */
    void recvRandomOTs(size_t numOTs, oc::Socket &chl);

   private:
    /**
     * @brief Compute AND gate using random OTs. Let in1 and in2 is the input of
     * OT sender and receiver. When the result of OT sender and receiver are
     * out1 and out2, out1 ^ out2 is in1 & in2.
     *
     * @param input a vector of bits
     * @param output a vector of bits
     * @param chl a socket for network
     * @return coproto::task<>
     */
    void computeAND(const oc::BitVector &input, oc::BitVector &output,
                    oc::Socket &chl);

    bool mIsSender;

    oc::PRNG mPrng;

    std::shared_ptr<oc::SilentOtExtSender> mSilentOtExtSenderPtr;

    std::shared_ptr<oc::SilentOtExtReceiver> mSilentOtExtReceiverPtr;

    std::array<oc::BitVector, 2> mRandomTwoMessages;  // if mIsSender = true

    oc::BitVector mRandomChoices, mRandomMessages;  // if mIsSender = false

    size_t mNumThreads = 1, mRandomOtIdx = 0, mRemainRandomOt = 0;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc::protocols