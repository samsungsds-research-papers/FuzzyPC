/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "cryptoTools/Common/MatrixView.h"

#include "Rr22Okvs.h"

using namespace std;
using namespace oc;
using namespace volePSI;

namespace fuzzypc::algorithms {
void Rr22Okvs::configure(size_t numKeys, size_t gamma, size_t fieldByteSize,
                         oc::block &seed, size_t numThreads) {
    fieldByteSize_ = fieldByteSize;
    mNumThreads = numThreads;
    mSeed = seed;
    mPrng.SetSeed(mSeed);
    paxos_ = make_shared<Baxos>();
    auto denseType = PaxosParam::Binary;
    paxos_->init(numKeys, 1 << 14, 3, gamma, denseType, seed);
}

void Rr22Okvs::encode(const std::vector<std::byte> &keys,
                      const std::vector<std::byte> &values, size_t keyByteSize,
                      std::vector<std::byte> &encoded) {
    // Compute Parameters
    size_t numKeys = keys.size() / keyByteSize;
    size_t numValues = values.size() / fieldByteSize_;

    // Convert to vector of blocks
    vector<block> keysAsBlock(numKeys, ZeroBlock);
    for (size_t i = 0; i < keysAsBlock.size(); i++) {
        memcpy(keysAsBlock[i].data(),
               reinterpret_cast<const uint8_t *>(keys.data()) + i * keyByteSize,
               keyByteSize);
    }

    // Convert to MatrixView
    osuCrypto::MatrixView<u8> valuesAsMatrixView((u8 *)values.data(), numValues,
                                                 fieldByteSize_);

    // Resize and Convert to MatrixView
    encoded.resize(paxos_->size() * fieldByteSize_);
    osuCrypto::MatrixView<u8> encodedAsMatrixView(
        (u8 *)encoded.data(), paxos_->size(), fieldByteSize_);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("okvs.encode.conversion");
    }

    // Solve
    paxos_->solve<u8>(keysAsBlock, valuesAsMatrixView, encodedAsMatrixView,
                      nullptr, mNumThreads);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("okvs.encode");
    }
}

void Rr22Okvs::decode(const std::vector<std::byte> &encoded,
                      const std::vector<std::byte> &keys, size_t keyByteSize,
                      std::vector<std::byte> &values) {
    // Compute Parameters
    size_t numKeys = keys.size() / keyByteSize;

    // Convert to vector of blocks
    vector<oc::block> keysAsBlock(numKeys, oc::ZeroBlock);
    for (size_t i = 0; i < keysAsBlock.size(); i++) {
        memcpy(keysAsBlock[i].data(),
               reinterpret_cast<const uint8_t *>(keys.data()) + i * keyByteSize,
               keyByteSize);
    }
    osuCrypto::MatrixView<u8> encodedAsMatrixView(
        (u8 *)encoded.data(), paxos_->size(), fieldByteSize_);
    values.resize(numKeys * fieldByteSize_);
    osuCrypto::MatrixView<u8> valuesAsMatrixView((u8 *)values.data(), numKeys,
                                                 fieldByteSize_);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("okvs.decode.conversion");
    }

    // Decode
    paxos_->decode<u8>(keysAsBlock, valuesAsMatrixView, encodedAsMatrixView,
                       mNumThreads);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("okvs.decode");
    }
}
}  // namespace pprl::algorithms