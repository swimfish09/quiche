// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/core/quic_crypto_server_stream.h"

#include <memory>

#include "net/third_party/quiche/src/quic/core/crypto/crypto_protocol.h"
#include "net/third_party/quiche/src/quic/core/crypto/crypto_utils.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_crypto_server_config.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_random.h"
#include "net/third_party/quiche/src/quic/core/proto/cached_network_parameters.proto.h"
#include "net/third_party/quiche/src/quic/core/quic_config.h"
#include "net/third_party/quiche/src/quic/core/quic_crypto_server_handshaker.h"
#include "net/third_party/quiche/src/quic/core/quic_packets.h"
#include "net/third_party/quiche/src/quic/core/quic_session.h"
#include "net/third_party/quiche/src/quic/core/tls_server_handshaker.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_flag_utils.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_flags.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_logging.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_ptr_util.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_string.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_string_piece.h"

namespace quic {

QuicCryptoServerStreamBase::QuicCryptoServerStreamBase(QuicSession* session)
    : QuicCryptoStream(session) {}

// TODO(jokulik): Once stateless rejects support is inherent in the version
// number, this function will likely go away entirely.
// static
bool QuicCryptoServerStreamBase::DoesPeerSupportStatelessRejects(
    const CryptoHandshakeMessage& message) {
  QuicTagVector received_tags;
  QuicErrorCode error = message.GetTaglist(kCOPT, &received_tags);
  if (error != QUIC_NO_ERROR) {
    return false;
  }
  for (const QuicTag tag : received_tags) {
    if (tag == kSREJ) {
      return true;
    }
  }
  return false;
}

QuicCryptoServerStream::QuicCryptoServerStream(
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache,
    bool use_stateless_rejects_if_peer_supported,
    QuicSession* session,
    Helper* helper)
    : QuicCryptoServerStreamBase(session),
      use_stateless_rejects_if_peer_supported_(
          use_stateless_rejects_if_peer_supported),
      peer_supports_stateless_rejects_(false),
      crypto_config_(crypto_config),
      compressed_certs_cache_(compressed_certs_cache),
      helper_(helper) {
  DCHECK_EQ(Perspective::IS_SERVER, session->connection()->perspective());
}

QuicCryptoServerStream::~QuicCryptoServerStream() {}

void QuicCryptoServerStream::CancelOutstandingCallbacks() {
  if (handshaker()) {
    handshaker()->CancelOutstandingCallbacks();
  }
}

bool QuicCryptoServerStream::GetBase64SHA256ClientChannelID(
    QuicString* output) const {
  return handshaker()->GetBase64SHA256ClientChannelID(output);
}

void QuicCryptoServerStream::SendServerConfigUpdate(
    const CachedNetworkParameters* cached_network_params) {
  handshaker()->SendServerConfigUpdate(cached_network_params);
}

uint8_t QuicCryptoServerStream::NumHandshakeMessages() const {
  return handshaker()->NumHandshakeMessages();
}

uint8_t QuicCryptoServerStream::NumHandshakeMessagesWithServerNonces() const {
  return handshaker()->NumHandshakeMessagesWithServerNonces();
}

int QuicCryptoServerStream::NumServerConfigUpdateMessagesSent() const {
  return handshaker()->NumServerConfigUpdateMessagesSent();
}

const CachedNetworkParameters*
QuicCryptoServerStream::PreviousCachedNetworkParams() const {
  return handshaker()->PreviousCachedNetworkParams();
}

bool QuicCryptoServerStream::UseStatelessRejectsIfPeerSupported() const {
  return use_stateless_rejects_if_peer_supported_;
}

bool QuicCryptoServerStream::PeerSupportsStatelessRejects() const {
  return peer_supports_stateless_rejects_;
}

bool QuicCryptoServerStream::ZeroRttAttempted() const {
  return handshaker()->ZeroRttAttempted();
}

void QuicCryptoServerStream::SetPeerSupportsStatelessRejects(
    bool peer_supports_stateless_rejects) {
  peer_supports_stateless_rejects_ = peer_supports_stateless_rejects;
}

void QuicCryptoServerStream::SetPreviousCachedNetworkParams(
    CachedNetworkParameters cached_network_params) {
  handshaker()->SetPreviousCachedNetworkParams(cached_network_params);
}

bool QuicCryptoServerStream::ShouldSendExpectCTHeader() const {
  return handshaker()->ShouldSendExpectCTHeader();
}

QuicLongHeaderType QuicCryptoServerStream::GetLongHeaderType(
    QuicStreamOffset offset) const {
  return handshaker()->GetLongHeaderType(offset);
}

bool QuicCryptoServerStream::encryption_established() const {
  if (!handshaker()) {
    return false;
  }
  return handshaker()->encryption_established();
}

bool QuicCryptoServerStream::handshake_confirmed() const {
  if (!handshaker()) {
    return false;
  }
  return handshaker()->handshake_confirmed();
}

const QuicCryptoNegotiatedParameters&
QuicCryptoServerStream::crypto_negotiated_params() const {
  return handshaker()->crypto_negotiated_params();
}

CryptoMessageParser* QuicCryptoServerStream::crypto_message_parser() {
  return handshaker()->crypto_message_parser();
}

void QuicCryptoServerStream::OnSuccessfulVersionNegotiation(
    const ParsedQuicVersion& version) {
  DCHECK_EQ(version, session()->connection()->version());
  CHECK(!handshaker_);
  switch (session()->connection()->version().handshake_protocol) {
    case PROTOCOL_QUIC_CRYPTO:
      handshaker_ = QuicMakeUnique<QuicCryptoServerHandshaker>(
          crypto_config_, this, compressed_certs_cache_, session(), helper_);
      break;
    case PROTOCOL_TLS1_3:
      handshaker_ = QuicMakeUnique<TlsServerHandshaker>(
          this, session(), crypto_config_->ssl_ctx(),
          crypto_config_->proof_source());
      break;
    case PROTOCOL_UNSUPPORTED:
      QUIC_BUG << "Attempting to create QuicCryptoServerStream for unknown "
                  "handshake protocol";
  }
}

QuicCryptoServerStream::HandshakerDelegate* QuicCryptoServerStream::handshaker()
    const {
  return handshaker_.get();
}

}  // namespace quic
