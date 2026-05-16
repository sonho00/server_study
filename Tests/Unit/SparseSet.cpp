#include "Network/Common/Pool/SparseSet.hpp"

#include <gtest/gtest.h>

TEST(DataStructureTest, SparseSetV2) {
	SparseSet<10, 3> set;

	uint64_t h1 = set.Pop(1);
	uint64_t h2 = set.Pop(2);

	EXPECT_TRUE(set.IsValid(h1));
	EXPECT_EQ(set.GetIndicesInState(1).size(), 1);
	EXPECT_EQ(set.GetIndicesInState(2).size(), 1);

	LOG_INFO("Test 1 Passed: Acquire");

	set.MoveToState(h1, 2);
	EXPECT_TRUE(set.GetIndicesInState(1).empty());
	EXPECT_EQ(set.GetIndicesInState(2).size(), 2);

	LOG_INFO("Test 2 Passed: Transition");

	set.Push(h1);
	LOG_INFO("Checking invalidity of handle after release...");
	EXPECT_FALSE(set.IsValid(h1));

	LOG_INFO("Test 3 Passed: Release & Reuse");

	uint64_t h1_new = set.Pop(2);
	EXPECT_NE(h1_new, h1);

	for (int i = 0; i < 8; ++i) {
		uint64_t h = set.Pop(1);
		EXPECT_NE(h, (SparseSet<10, 3>::kInvalidHandle));
	}

	LOG_INFO("Checking capacity limit...");
	EXPECT_EQ(set.Pop(1), (SparseSet<10, 3>::kInvalidHandle));

	LOG_INFO("Test 4 Passed: Capacity check");
}
