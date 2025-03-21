// Copyright 2019 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "yacl/crypto/pke/sm2_enc.h"

#include "gtest/gtest.h"

#include "yacl/crypto/openssl_wrappers.h"

namespace yacl::crypto {

TEST(Sm2Enc, EncryptDecrypt_shouldOk) {
  // GIVEN
  auto [pk_buf, sk_buf] = GenSm2KeyPairToPemBuf();
  std::string m = "I am a plaintext.";

  // WHEN
  auto enc_ctx = Sm2Encryptor(pk_buf);
  auto dec_ctx = Sm2Decryptor(sk_buf);

  auto c = enc_ctx.Encrypt(m);
  auto m_check = dec_ctx.Decrypt(c);

  // THEN
  EXPECT_EQ(std::memcmp(m.data(), m_check.data(), m.size()), 0);
}

}  // namespace yacl::crypto
