// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_CRYPTO_QUIC_ENCRYPTER_H_
#define QUICHE_QUIC_CORE_CRYPTO_QUIC_ENCRYPTER_H_

#include <cstddef>
#include <memory>

#include "net/third_party/quiche/src/quic/core/crypto/quic_crypter.h"
#include "net/third_party/quiche/src/quic/core/quic_packets.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_export.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_string_piece.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicEncrypter : public QuicCrypter {
 public:
  virtual ~QuicEncrypter() {}

  static std::unique_ptr<QuicEncrypter> Create(QuicTag algorithm);

  // Creates an IETF QuicEncrypter based on |cipher_suite| which must be an id
  // returned by SSL_CIPHER_get_id. The caller is responsible for taking
  // ownership of the new QuicEncrypter.
  static std::unique_ptr<QuicEncrypter> CreateFromCipherSuite(
      uint32_t cipher_suite);

  // Writes encrypted |plaintext| and a MAC over |plaintext| and
  // |associated_data| into output. Sets |output_length| to the number of
  // bytes written. Returns true on success or false if there was an error.
  // |packet_number| is appended to the |nonce_prefix| value provided in
  // SetNoncePrefix() to form the nonce. |output| must not overlap with
  // |associated_data|. If |output| overlaps with |plaintext| then
  // |plaintext| must be <= |output|.
  virtual bool EncryptPacket(uint64_t packet_number,
                             QuicStringPiece associated_data,
                             QuicStringPiece plaintext,
                             char* output,
                             size_t* output_length,
                             size_t max_output_length) = 0;

  // GetKeySize() and GetNoncePrefixSize() tell the HKDF class how many bytes
  // of key material needs to be derived from the master secret.
  // NOTE: the sizes returned by GetKeySize() and GetNoncePrefixSize() are
  // also correct for the QuicDecrypter of the same algorithm.

  // Returns the size in bytes of the fixed initial part of the nonce.
  virtual size_t GetNoncePrefixSize() const = 0;

  // Returns the maximum length of plaintext that can be encrypted
  // to ciphertext no larger than |ciphertext_size|.
  virtual size_t GetMaxPlaintextSize(size_t ciphertext_size) const = 0;

  // Returns the length of the ciphertext that would be generated by encrypting
  // to plaintext of size |plaintext_size|.
  virtual size_t GetCiphertextSize(size_t plaintext_size) const = 0;

  // For use by unit tests only.
  virtual QuicStringPiece GetKey() const = 0;
  virtual QuicStringPiece GetNoncePrefix() const = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_CRYPTO_QUIC_ENCRYPTER_H_
