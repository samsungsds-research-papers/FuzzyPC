#include "gtest/gtest.h"

#include <iostream>

#include "cryptoTools/Common/Timer.h"

#include "Algorithms/Rr22Okvs/Rr22Okvs.h"
#include "Common/Random.h"

using namespace std;
using namespace fuzzypc::common;
using namespace fuzzypc::algorithms;

class OkvsTest : public ::testing::TestWithParam<pair<size_t, size_t>> {
   protected:
    uint64_t fieldByteSize = 32;
    uint64_t keyByteSize = 16;
    uint64_t gamma = 40;
};

TEST_P(OkvsTest, CorrectnessTest) {
    auto param = GetParam();
    size_t numKeys = param.first, numCommonKeys = param.second;

    vector<byte> keys1(numKeys * keyByteSize), keys2(numKeys * keyByteSize),
        values(numKeys * fieldByteSize);
    randomBytes(keys1.data(), keys1.size());
    randomBytes(keys2.data(), keys2.size());
    randomBytes(values.data(), values.size());
    for (size_t i = 0; i < numCommonKeys * keyByteSize; i++) {
        keys1[i] = keys2[i];
    }
    vector<byte> encoded, decoded;

    Rr22Okvs okvs;
    oc::block seed = oc::toBlock(123123, 456456);
    okvs.configure(numKeys, gamma, fieldByteSize, seed, 1);
    okvs.encode(keys1, values, keyByteSize, encoded);
    okvs.decode(encoded, keys2, keyByteSize, decoded);

    for (size_t i = 0; i < numKeys; i++) {
        bool flag = true;
        for (size_t j = 0; j < fieldByteSize; j++) {
            if (values[i * fieldByteSize + j] !=
                decoded[i * fieldByteSize + j]) {
                flag = false;
                break;
            }
        }
        if (i < numCommonKeys) {
            EXPECT_TRUE(flag);
        } else {
            EXPECT_FALSE(flag);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(OkvsTest, OkvsTest,
                         testing::Values(make_pair(10000, 100),
                                         make_pair(100000, 1000),
                                         make_pair(1000000, 10000)));