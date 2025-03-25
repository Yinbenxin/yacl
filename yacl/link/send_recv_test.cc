// Copyright 2023 Ant Group Co., Ltd.
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

#include "yacl/link/factory.h"

#include <future>
#include <limits>
#include <unordered_map>
#include "ichannel.h"
#include "channel.h"
#include "memchannel.h"
#include "fmt/format.h"
#include "gtest/gtest.h"

#include "yacl/crypto/key_utils.h"
#include "yacl/link/context.h"
#include "yacl/link/link.h"

namespace yacl::link::test {

template <typename T, size_t MODE>
struct TestTypes {
  static size_t get_mode() { return MODE; }
  static T get_t_instance() { return T(); }
};

enum class SslMode {
  NONE,        // mode = 0
  RSA_SHA256,  // mode = 1
  SM2_SM3,     // mode = 2
};

inline std::pair<std::string, std::string> GenCertFiles(
    const std::string& prefix, const SslMode mode) {
  auto pk_path = fmt::format("{}_pk.pem", prefix);
  auto sk_path = fmt::format("{}_sk.pem", prefix);
  auto cert_path = fmt::format("{}.cer", prefix);

  if (mode == SslMode::RSA_SHA256) {
    auto key_pair = crypto::GenRsaKeyPair();
    crypto::ExportPublicKeyToPemFile(key_pair, pk_path);
    crypto::ExportSecretKeyToPemBuf(key_pair, sk_path);
    auto cert = crypto::MakeX509Cert(crypto::LoadKeyFromFile(pk_path),
                                     crypto::LoadKeyFromFile(sk_path),
                                     {
                                         {"C", "CN"},
                                         {"ST", "ZJ"},
                                         {"L", "HZ"},
                                         {"O", "TEE"},
                                         {"OU", "EGG"},
                                         {"CN", "demo.trustedegg.com"},
                                     },
                                     3, crypto::HashAlgorithm::SHA256);
    crypto::ExportX509CertToFile(cert, cert_path);
  } else if (mode == SslMode::SM2_SM3) {
    auto key_pair = crypto::GenSm2KeyPair();
    crypto::ExportPublicKeyToPemFile(key_pair, pk_path);
    crypto::ExportSecretKeyToPemBuf(key_pair, sk_path);
    auto cert = crypto::MakeX509Cert(crypto::LoadKeyFromFile(pk_path),
                                     crypto::LoadKeyFromFile(sk_path),
                                     {
                                         {"C", "CN"},
                                         {"ST", "ZJ"},
                                         {"L", "HZ"},
                                         {"O", "TEE"},
                                         {"OU", "EGG"},
                                         {"CN", "demo.trustedegg.com"},
                                     },
                                     3, crypto::HashAlgorithm::SM3);
    crypto::ExportX509CertToFile(cert, cert_path);
  } else {
    YACL_THROW("Unknown SSL mode.");
  }

  return {sk_path, cert_path};
}

inline ContextDesc MakeDesc(int count, const SslMode mode) {
  ContextDesc desc;
  desc.id = fmt::format("world_{}", count);
  desc.parties.push_back(ContextDesc::Party("alice", "127.0.0.1:63927"));
  desc.parties.push_back(ContextDesc::Party("bob", "127.0.0.1:63921"));
  if (mode != SslMode::NONE) {
    desc.enable_ssl = true;
    desc.server_ssl_opts.ciphers = "";  // auto detect

    // export rsa keys to files
    auto [server_sk_path, server_cer_path] = GenCertFiles("server", mode);
    auto [client_sk_path, client_cer_path] = GenCertFiles("client", mode);

    desc.server_ssl_opts.cert.certificate_path = server_cer_path;
    desc.server_ssl_opts.cert.private_key_path = server_sk_path;

    desc.client_ssl_opts.cert.certificate_path = client_cer_path;
    desc.client_ssl_opts.cert.private_key_path = client_sk_path;
  }
  return desc;
}

template <typename M>
class FactoryTest : public ::testing::Test {
 public:
  void SetUp() override {
    static int desc_count = 0;
    contexts_.resize(2);
    auto desc = MakeDesc(desc_count++, SslMode(M::get_mode()));

    auto create_brpc = [&](int self_rank) {
      contexts_[self_rank] = M::get_t_instance().CreateContext(desc, self_rank);
      contexts_[self_rank]->add_gaia_net();
    };

    std::vector<std::future<void>> creates;
    creates.push_back(std::async(create_brpc, 0));
    creates.push_back(std::async(create_brpc, 1));

    for (auto& f : creates) {
      f.get();
    }
  }

  void TearDown() override {
    auto wait = [&](int self_rank) {
      contexts_[self_rank]->WaitLinkTaskFinish();
    };

    std::vector<std::future<void>> waits;
    waits.push_back(std::async(wait, 0));
    waits.push_back(std::async(wait, 1));

    for (auto& f : waits) {
      f.get();
    }
  }

  std::vector<std::shared_ptr<Context>> contexts_;
};
using FactoryTestTypes =
    ::testing::Types<TestTypes<FactoryBrpc, 0>
// using FactoryTestTypes =
//     ::testing::Types<TestTypes<FactoryMem, 0>, TestTypes<FactoryBrpc, 0>,
//                      TestTypes<FactoryBrpc, 1>
#ifdef YACL_WITH_TONGSUO
                     ,
                     TestTypes<FactoryBrpc, 2>
#endif
                     >;

TYPED_TEST_SUITE(FactoryTest, FactoryTestTypes);

TYPED_TEST(FactoryTest, SendRecv) {
  auto test = [&](int self_rank) {
    if (self_rank == 0) {
      this->contexts_[0]->Send(1, "test", "test");
    } else {
      Buffer r = this->contexts_[1]->Recv(0, "test");
      EXPECT_EQ(std::string(r.data<const char>(), r.size()),
                std::string("test"));
    }
  };

  std::vector<std::future<void>> tests;
  tests.push_back(std::async(test, 0));
  tests.push_back(std::async(test, 1));

  for (auto& f : tests) {
    f.get();
  }
}

}  // namespace yacl::link::test