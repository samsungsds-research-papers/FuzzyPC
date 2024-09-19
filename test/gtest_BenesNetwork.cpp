#include "gtest/gtest.h"

#include <algorithm>
#include <numeric>

#include "Algorithms/BenesNetwork.h"

using namespace std;

class BenesNetworkTest : public ::testing::TestWithParam<size_t> {
   public:
};

TEST_P(BenesNetworkTest, CorrectnessTest) {
    size_t numInputs = GetParam();

    srand(unsigned(time(0)));
    fuzzypc::structures::IntegerPermutation permutation(0, numInputs - 1);
    permutation.randomShuffle();

    vector<uint64_t> in, out;
    in.resize(numInputs);
    iota(in.begin(), in.end(), 0);
    random_shuffle(in.begin(), in.end());

    fuzzypc::algorithms::BenesNetwork network;
    network.buildConnection(numInputs);
    network.routing(permutation);
    network.evaluate(in, out);

    for (size_t i = 0; i < numInputs; i++) {
        EXPECT_EQ(in[i], out[permutation.get(i)]);
    }
}

INSTANTIATE_TEST_SUITE_P(BenesNetworkTest, BenesNetworkTest,
                         testing::Values(10000, 100000, 1000000));