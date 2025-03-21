#include "protos/gaia_proxy.grpc.pb.h"
#include <gflags/gflags.h>
#include <google/protobuf/stubs/common.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>

using DataTransferService = com::be::cube::core::networking::proxy::DataTransferService;
using VersionRequest = com::be::cube::core::networking::proxy::VersionRequest;
using VersionResponse = com::be::cube::core::networking::proxy::VersionResponse;
using grpc::ClientContext;

using Packet = com::be::cube::core::networking::proxy::Packet;
using ServiceType = com::be::cube::core::networking::proxy::ServiceType;
using ServerSummary = com::be::cube::core::networking::proxy::ServerSummary;

void test_func(int loop_cnt, const std::string& remote_addr) {
    for (int i = 0; i < loop_cnt; ++i) {

        grpc::ChannelArguments channel_args;
        channel_args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, -1);
        channel_args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, -1);

        channel_args.SetCompressionAlgorithm(grpc_compression_algorithm::GRPC_COMPRESS_GZIP);

        std::shared_ptr<grpc::Channel> chl = grpc::CreateCustomChannel(remote_addr, grpc::InsecureChannelCredentials(), channel_args);

        unsigned int client_connection_timeout = 10000;

        std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(client_connection_timeout);

        bool ret = chl->WaitForConnected(deadline);
        assert(ret);

        std::unique_ptr<DataTransferService::Stub> stub = DataTransferService::NewStub(chl);

        {
            if (true) {
                ClientContext ctx_version;
                VersionRequest req;
                VersionResponse resp;

                auto status = stub->get_version(&ctx_version, req, &resp);
                if (!status.ok()) {
                    std::cout << "call get_version failed: error_code:" << status.error_code() << ",error_message" << status.error_message()
                              << ",error_details=" << status.error_details() << std::endl;
                }
            }

            if (true) {
                ClientContext ctx;
                ServerSummary sm;
                auto v1_sender = stub->push(&ctx, &sm);

                std::string taskid = "test_taskid";
                int from = 0;
                int to = 1;

                Packet packet;
                packet.mutable_header()->set_taskid(std::move(taskid));
                packet.mutable_header()->set_frompartyid(std::to_string(from));
                packet.mutable_header()->set_topartyid(std::to_string(to));
                packet.mutable_header()->set_servicetype(ServiceType::TENSOR);

                auto flag = v1_sender->Write(packet);
                assert(flag);
                flag = v1_sender->WritesDone();
                assert(flag);
                auto status = v1_sender->Finish();
                if (!status.ok()) {
                    std::cout << "call v1_send failed: error_code:" << status.error_code() << ",error_message" << status.error_message()
                              << ",error_details=" << status.error_details() << std::endl;
                }
            }

            if (true) {
                ClientContext ctx;
                std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(client_connection_timeout);
                ctx.set_deadline(deadline);

                auto v2_sender = stub->push_v2(&ctx);

                std::string taskid = "test_taskid";
                int from = 0;
                int to = 1;

                Packet packet;
                packet.mutable_header()->set_taskid(std::move(taskid));
                packet.mutable_header()->set_frompartyid(std::to_string(from));
                packet.mutable_header()->set_topartyid(std::to_string(to));
                packet.mutable_header()->set_servicetype(ServiceType::TENSOR);

                auto flag = v2_sender->Write(packet);
                assert(flag);

                auto rthread = std::thread([&v2_sender] {
                    Packet resp;
                    for (;;) {
                        auto flag = v2_sender->Read(&resp);
                        if (!flag) {
                            break;
                        } else {
                        }
                    }
                });

                flag = v2_sender->WritesDone();
                assert(flag);
                auto status = v2_sender->Finish();
                if (!status.ok()) {
                    std::cout << "call v2_send failed: error_code:" << status.error_code() << ",error_message" << status.error_message()
                              << ",error_details=" << status.error_details() << std::endl;
                }

                rthread.join();
            }
        }

        deadline = std::chrono::system_clock::now() + std::chrono::seconds(client_connection_timeout);
        ret = chl->WaitForStateChange(grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN, deadline);

        assert(ret);
    }
}

int main(int argc, char** argv) {

    int thread_cnt = 1;
    if (argc >= 2) {
        thread_cnt = std::atoi(argv[1]);
    }

    int loop_cnt = 1;
    if (argc >= 3) {
        loop_cnt = std::atoi(argv[2]);
    }

    std::string remote_addr = "127.0.0.1:9900";
    if (argc >= 4) {
        remote_addr = argv[3];
    }

    if (thread_cnt == 1) {
        test_func(loop_cnt, remote_addr);
    } else {
        std::vector<std::thread> all_threads;
        all_threads.reserve(thread_cnt);
        for (int i = 0; i < thread_cnt; ++i) {
            all_threads.emplace_back(test_func, loop_cnt, remote_addr);
        }

        for (auto& t : all_threads) {
            t.join();
        }
    }

    google::protobuf::ShutdownProtobufLibrary();
    gflags::ShutDownCommandLineFlags();
    return 0;
}