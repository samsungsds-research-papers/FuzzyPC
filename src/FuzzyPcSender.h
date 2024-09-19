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
#include <libOTe/Tools/Coproto.h>
#include <chrono>
#include <memory>

#include "Common/Random.h"
#include "Defines.h"
#include "FuzzyPc.h"
#include "Protocols/CpsiSender.h"
#include "Protocols/PnsReceiver.h"
#include "Protocols/SecretSharedOt.h"

namespace fuzzypc {
/**
 * @brief
 *
 */
class FuzzyPcSender : public FuzzyPc {
   public:
    /**
     * @brief Construct a new FuzzyPcSender object
     *
     * @param numRows
     * @param numBins
     * @param hashByteSize
     * @param payloadByteSize
     * @param numThreads
     * @param timer
     */
    FuzzyPcSender(uint64_t numRows, uint64_t numBins, uint32_t hashByteSize,
               uint32_t payloadByteSize, size_t numThreads = 1,
               oc::Timer *timer = nullptr)
        : mNumRows(numRows),
          mNumBins(numBins),
          mHashByteSize(hashByteSize),
          mPayloadByteSize(payloadByteSize),
          mNumThreads(numThreads),
          mTimer(timer) {
        mRandomIds.resize(numRows * payloadByteSize);
        fuzzypc::common::randomBytes(mRandomIds.data(), mRandomIds.size());
    };

    /**
     * @brief
     *
     * @param deduplicated
     * @param deduplicatedToRows
     * @param membershipShares
     * @param payloadShares
     * @param featureIdx
     * @param socket
     */
    void runCpsiAndPns(const std::vector<std::byte> &deduplicated,
                       const std::vector<uint64_t> &deduplicatedToRows,
                       oc::BitVector &membershipShares,
                       std::vector<std::byte> &payloadShares, size_t featureIdx,
                       oc::Socket &socket);

    /**
     * @brief
     *
     * @param input
     * @param socket
     */
    void run(const std::vector<std::vector<std::string>> &input,
             oc::Socket &socket);

    /**
     * @brief 
     * 
     * @param input 
     * @param socket 
     */
    void run(const std::vector<std::vector<uint64_t>> &input,
             oc::Socket &socket);

    /**
     * @brief Get the Ids In Byte object
     *
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> getPayloadsInByte() { return mRandomIds; };

    /**
     * @brief Get the Item Byte Size object
     *
     * @return uint32_t
     */
    uint32_t getItemByteSize() { return mHashByteSize; };

    /**
     * @brief Get the Id Byte Size object
     *
     * @return uint32_t
     */
    uint32_t getPayloadByteSize() { return mPayloadByteSize; };

    /**
     * @brief
     *
     * @param maxNumItems
     * @param numFeatures
     * @param idByteSize
     * @param chl
     */
    void preprocess(size_t maxNumItems, size_t numFeatures, size_t idByteSize,
                    oc::Socket &chl);

    /**
     * @brief
     *
     */
    void printTime() {
        std::cout << "\t* setup time : " << mSetupTime << " ms" << std::endl;
        std::cout << "\t* online time : " << mCpsiTime + mPnsTime + mAsTime
                  << " ms" << std::endl;
        std::cout << "\t\t> CPSI time : " << mCpsiTime << " ms" << std::endl;
        std::cout << "\t\t> PnS time : " << mPnsTime << " ms" << std::endl;
        std::cout << "\t\t> AS time : " << mAsTime << " ms" << std::endl;
        std::cout << "\t* total time : "
                  << mSetupTime + mCpsiTime + mPnsTime + mAsTime << " ms"
                  << std::endl;
    }

   private:
    std::chrono::steady_clock::time_point mStart, mEnd;

    double mSetupTime = 0.0, mCpsiTime = 0.0, mPnsTime = 0.0, mAsTime = 0.0;

    size_t mNumThreads, mNumRows, mNumBins, mHashByteSize, mPayloadByteSize;

    bool mGlobalIndexFixed = false;

    oc::PRNG mPrng{oc::CCBlock};

    std::vector<std::byte> mRandomIds;

    std::vector<std::shared_ptr<fuzzypc::protocols::CPSISender>>
        mCpsiSenderPtrs;

    std::vector<std::shared_ptr<fuzzypc::protocols::PnsReceiver>>
        mPnsReceiverPtrs;

    std::shared_ptr<fuzzypc::protocols::SecretSharedOT> mSecretSharedOtPtr;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc
