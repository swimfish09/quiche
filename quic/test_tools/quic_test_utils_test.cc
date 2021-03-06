// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/test_tools/quic_test_utils.h"

#include "testing/base/public/gunit-spi.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_test.h"

namespace quic {
namespace test {

class QuicTestUtilsTest : public QuicTest {};

TEST_F(QuicTestUtilsTest, ConnectionId) {
  EXPECT_NE(EmptyQuicConnectionId(), TestConnectionId());
  EXPECT_NE(EmptyQuicConnectionId(), TestConnectionId(1));
  EXPECT_EQ(TestConnectionId(), TestConnectionId());
  EXPECT_EQ(TestConnectionId(33), TestConnectionId(33));
  EXPECT_NE(TestConnectionId(0xdead), TestConnectionId(0xbeef));
  EXPECT_EQ(0x1337u, TestConnectionIdToUInt64(TestConnectionId(0x1337)));
  EXPECT_NE(0xdeadu, TestConnectionIdToUInt64(TestConnectionId(0xbeef)));
}

TEST_F(QuicTestUtilsTest, BasicApproxEq) {
  ExpectApproxEq(10, 10, 1e-6f);
  ExpectApproxEq(1000, 1001, 0.01f);
  EXPECT_NONFATAL_FAILURE(ExpectApproxEq(1000, 1100, 0.01f), "");

  ExpectApproxEq(64, 31, 0.55f);
  EXPECT_NONFATAL_FAILURE(ExpectApproxEq(31, 64, 0.55f), "");
}

TEST_F(QuicTestUtilsTest, QuicTimeDelta) {
  ExpectApproxEq(QuicTime::Delta::FromMicroseconds(1000),
                 QuicTime::Delta::FromMicroseconds(1003), 0.01f);
  EXPECT_NONFATAL_FAILURE(
      ExpectApproxEq(QuicTime::Delta::FromMicroseconds(1000),
                     QuicTime::Delta::FromMicroseconds(1200), 0.01f),
      "");
}

TEST_F(QuicTestUtilsTest, QuicBandwidth) {
  ExpectApproxEq(QuicBandwidth::FromBytesPerSecond(1000),
                 QuicBandwidth::FromBitsPerSecond(8005), 0.01f);
  EXPECT_NONFATAL_FAILURE(
      ExpectApproxEq(QuicBandwidth::FromBytesPerSecond(1000),
                     QuicBandwidth::FromBitsPerSecond(9005), 0.01f),
      "");
}

// Ensure that SimpleRandom does not change its output for a fixed seed.
TEST_F(QuicTestUtilsTest, SimpleRandomStability) {
  SimpleRandom rng;
  rng.set_seed(UINT64_C(0x1234567800010001));
  EXPECT_EQ(UINT64_C(14865409841904857791), rng.RandUint64());
  EXPECT_EQ(UINT64_C(12139094019410129741), rng.RandUint64());
}

}  // namespace test
}  // namespace quic
