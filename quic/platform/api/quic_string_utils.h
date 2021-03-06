// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_PLATFORM_API_QUIC_STRING_UTILS_H_
#define QUICHE_QUIC_PLATFORM_API_QUIC_STRING_UTILS_H_

#include <utility>

#include "net/third_party/quiche/src/quic/platform/api/quic_string.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_string_piece.h"
#include "net/quic/platform/impl/quic_string_utils_impl.h"

namespace quic {

template <typename... Args>
inline void QuicStrAppend(QuicString* output, const Args&... args) {
  QuicStrAppendImpl(output, std::forward<const Args&>(args)...);
}

}  // namespace quic

#endif  // QUICHE_QUIC_PLATFORM_API_QUIC_STRING_UTILS_H_
