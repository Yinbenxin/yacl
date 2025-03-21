//
// Created by imayuxiang on 6/8/21.
//

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <random>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <thread>


//#include "../src/protos/gaianet.grpc.pb.h"
#include "../src/grpc_async_server.h"
#include "../src/grpc_client.h"
#include "../src/rcvthread.h"
#include "../src/sndthread.h"
#include "../src/GLogHelper.h"
#include "../src/channel.h"
#include "unordered_map"
#include "../src/serverthread.h"

using namespace grpc;
using namespace gaianet;
using std::chrono::system_clock;
using namespace std;

void RunServer() {

    std::string server_address("0.0.0.0:50051");
    RpcServer service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

//    server->c_server();
    server->Wait();
    std::this_thread::sleep_for(
            std::chrono::milliseconds(5000));
    server->Shutdown();

}


TEST(ConnectTest, init) {


    std::string server_address("0.0.0.0:7766");
    ServerThread * serverThread = new ServerThread(server_address);
    serverThread->Start();
//    std::thread t_server = std::thread([&](){
        /**
        RpcServer service;
        ServerBuilder builder;
//        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());

        std::cout << "Server listening on " << server_address << std::endl;
        // responsible for shutting down the server for this call to ever return.
        server->Wait();
         */

//    });

//    std::thread t_client = std::thread([&](){
        string target_str = "localhost:7766";

        std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
        if (!chl->WaitForConnected(std::chrono::system_clock::now() +
                                   std::chrono::milliseconds(10000))) {
            std::cout<< "try to connect server " << target_str << " failed" << std::endl;
        } else {
            std::cout<< " connect server " << target_str << " succeed" << std::endl;
        }

        RpcClient greeter(
                chl, 1);
        std::string request_content("hello gaia-server, im client");

//        greeter.Send(1, request_content);

//    });

//    t_server.join();
//    t_client.join();
//    delete serverThread;
}


TEST(ConnectTest, server) {

    string target_address = "0.0.0.0:7768";
    std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());

    std::string server_address("0.0.0.0:7766");
    ServerThread * serverThread = new ServerThread(server_address);
    serverThread->Start();
//    serverThread->Wait();

//    std::this_thread::sleep_for(
//            std::chrono::milliseconds(5000));
    LOG(INFO) << "server thread started and wait";

    delete serverThread;

    /*
    std::string server_address("0.0.0.0:50051");
    RpcServer service;

//    server->c_server();
    std::unique_ptr<Server> server;
    std::thread t = std::thread([&](){
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = unique_ptr<Server>(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        server->Wait();
    });
    t.join();
    server->Shutdown();
    */
}



TEST(ConnectTest, submitrequest ) {

    GLogHelper log("test", ".");
    std::thread t_server = std::thread([&](){
//        RunServer();
        std::string server_address("0.0.0.0:50051");
        RpcServer service;

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;

//    server->c_server();
        server->Wait();
        std::this_thread::sleep_for(
                std::chrono::milliseconds(5000));
//        server->Shutdown();
    });

    std::thread t_client = std::thread([&](){
        string target_str = "localhost:50051";

        std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
        if (!chl->WaitForConnected(std::chrono::system_clock::now() +
                                   std::chrono::milliseconds(10000))) {
            std::cout<< "try to connect server " << target_str << " failed" << std::endl;
        } else {
            std::cout<< " connect server " << target_str << " succeed" << std::endl;
        }

        RpcClient greeter(
                chl, 1);
        std::string request_content("hello gaia-server, im client");
        greeter.Send(1, request_content);
        greeter.finish();
//        server->Shutdown();
    });

    t_server.join();
    t_client.join();

}


TEST(ConnectTest, sndrcvthread ) {

    using namespace std;
    GLogHelper log("test", "gaia-net");

    int partied = 2;
    std::vector<std::thread> partied_threads(partied);
    std::string server_address[]{"0.0.0.0:50051", "0.0.0.0:50052"};


    for (int i =0; i < partied; i ++) {

        partied_threads[i] = std::thread([&, i](){

            RpcClient * client;

            SndThread  * sndThread;
            RcvThread * rcvThread;

            ServerThread * serverThread = new ServerThread(server_address[i]);
            serverThread->Start();

            string target_str = server_address[partied-1-i];
            std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());

            CLock * lock = new CLock();

            if (!chl->WaitForConnected(std::chrono::system_clock::now() +
                                               std::chrono::milliseconds(10000))) {
                LOG(INFO)<< "try to connect server " << target_str << " failed" << std::endl;
            } else {
                LOG(INFO)<< " connect server " << target_str << " succeed" << std::endl;
            }

            client = new RpcClient(
                    chl, i);
            LOG(INFO) << "client started";

            rcvThread = new RcvThread(partied-1-i, serverThread->get_server());
            LOG(INFO) << "rcv thread started";
            rcvThread->Start();

            sndThread = new SndThread(client, lock);
            LOG(INFO) << "snd thread started";
            sndThread->Start();

            uint32_t task_size = 10;
            vector<std::thread>task_threads(task_size);

            for (int j = 0; j < task_size ; j++) {
                task_threads[j] = std::thread([&, j](){
                    channel *cchl = new channel(j, rcvThread, sndThread);
                    string send_buf = "party " + to_string(i) + " send test " + to_string(j);
                    cchl->send(send_buf);
                    string recv_buf;
                    cchl->recv(recv_buf);
                    LOG(INFO) << "party " << i << " cchl " << j <<  " recved : " << recv_buf;
                    delete cchl;
                });
            }

            for (int j = 0; j < task_size ; j++) {
                task_threads[j].join();
            }

            delete sndThread;
            delete rcvThread;
            delete lock;
            delete client;
            delete serverThread;

        });
    }

    for (int i =0; i < partied; i ++) {
        partied_threads[i].join();
    }
}



TEST(ConnectTest, multi_party) {

    using namespace std;
    GLogHelper log("test", "gaia-net");

    int parties = 3;
    std::vector<std::thread> partied_threads(parties);
    std::string server_address[]{"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053"};


    for (int i =0; i < parties; i ++) {


        partied_threads[i] = std::thread([&, i](){

            RpcServer * service = new RpcServer();

            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            ServerBuilder builder;
            builder.AddListeningPort(server_address[i], grpc::InsecureServerCredentials());
            builder.RegisterService(service);

            std::unique_ptr<Server>server(builder.BuildAndStart());
            std::cout << "Server listening on " << server_address[i] <<std::endl;
            LOG(INFO)<< "Server listening on " << server_address[i];


            vector<RcvThread*> rcvthreads(parties);
            for (int k=0; k<parties; k++){
                if (i != k) {
                    rcvthreads[k] = new RcvThread(k, service);
                    rcvthreads[k]->Start();
                }
            }
            LOG(INFO) << "rcv thread started";

            std::thread t_server = std::thread([&](){
                server->Wait();
            });

            vector<RpcClient*>clients(parties);
            std::unordered_map<int, std::shared_ptr<grpc::Channel>> chls;
            vector<SndThread*> sndthreads(parties);
            vector<CLock*>locks(parties);
            vector<channel*> cchls(parties);

            for (int k=0; k<parties; k++) {
                if (i != k) {
                    string target_str = server_address[k];
                    chls[k] = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());

                    std::thread t_client = std::thread([&, i]() {

                        if (!chls[k]->WaitForConnected(std::chrono::system_clock::now() +
                                                       std::chrono::milliseconds(10000))) {
                            LOG(INFO) << "try to connect server " << target_str << " failed" << std::endl;
                        } else {
                            LOG(INFO) << " connect server " << target_str << " succeed" << std::endl;
                        }

                    });
                    t_client.join();

                    clients[k] = new RpcClient(
                            chls[k], i);


                    locks[k] = new CLock();

                    sndthreads[k] = new SndThread(clients[k], locks[k]);
                    LOG(INFO) << "snd thread started";
                    sndthreads[k]->Start();
                }
            }

            uint32_t task_size = 1000;
            vector<std::thread>task_threads(task_size);

            for (int j = 0; j < task_size ; j++) {
                task_threads[j] = std::thread([&, j](){
                    for (int k=0; k < parties; k++){
                        if (i != k){
                            cchls[k] = new channel(j, rcvthreads[k], sndthreads[k]);
                            string send_buf = "party " + to_string(i) + " send test " + to_string(j);
                            LOG(INFO) << "party " << i << " cchl " << k <<  " send : " << send_buf;
                            cchls[k]->send(send_buf);

                            string recv_buf;
                            cchls[k]->recv(recv_buf);
                            LOG(INFO) << "party " << i << " cchl " << k <<  " recved : " << recv_buf;
                        }
                    }
                });
            }

            for (int j = 0; j < task_size ; j++) {
                task_threads[j].join();
            }

            t_server.join();

        });
    }

    for (int i =0; i < parties; i ++) {
        partied_threads[i].join();
    }
}


TEST(ConnectTest, role0_test ) {

    using namespace std;
    GLogHelper log("test", "gaia-net");

    int partied = 2;
    std::vector<std::thread> partied_threads(partied);
    std::string server_address[]{"0.0.0.0:50061", "0.0.0.0:50062"};

    int role = 0;
    int party_role = 1;
    RpcClient * client;

    SndThread  * sndThread;
    RcvThread * rcvThread;

    ServerThread * serverThread = new ServerThread(server_address[role]);
    serverThread->Start();

    string target_str = server_address[party_role];
    std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());

    CLock * lock = new CLock();

    if (!chl->WaitForConnected(std::chrono::system_clock::now() +
                               std::chrono::milliseconds(10000))) {
        LOG(INFO)<< "try to connect server " << target_str << " failed" << std::endl;
    } else {
        LOG(INFO)<< " connect server " << target_str << " succeed" << std::endl;

    client = new RpcClient(
            chl, role);

    rcvThread = new RcvThread(party_role, serverThread->get_server());
    LOG(INFO) << "rcv thread started";
    rcvThread->Start();

    sndThread = new SndThread(client, lock);
    LOG(INFO) << "snd thread started";
    sndThread->Start();

    uint32_t task_size = 1;
//    vector<std::thread>task_threads(task_size);

//    for (int j = 0; j < task_size ; j++) {
//        task_threads[j] = std::thread([&, j](){
        channel *cchl = new channel(0, rcvThread, sndThread);
        string send_buf = "party " + to_string(role) + " send test " + to_string(0);
        cchl->send(send_buf);
        uint32_t number = 6660;
        cchl->send(&number, sizeof(number));
        string recv_buf;
        cchl->recv(recv_buf);
        uint32_t recv_num;
        cchl->recv(&recv_num, sizeof(recv_num));
        LOG(INFO) << "party " << role << " cchl " << 0 <<  " recved : " << recv_buf;
        LOG(INFO) << "party " << role << " cchl " << 0 <<  " recved : " << recv_num;
            delete cchl;
//        });
//    }

//    for (int j = 0; j < task_size ; j++) {
//        task_threads[j].join();
//    }

    }
    delete sndThread;
    delete rcvThread;
    delete lock;
    delete client;
    delete serverThread;

}

TEST(ConnectTest, role1_test ) {

    using namespace std;
    GLogHelper log("test", "gaia-net");

    int partied = 2;
    std::vector<std::thread> partied_threads(partied);
//    std::string server_address[]{"0.0.0.0:50051", "0.0.0.0:50052"};
    std::string server_address[]{"0.0.0.0:50061", "0.0.0.0:50062"};

    int role = 1;
    int party_role = 0;
    RpcClient * client;

    SndThread  * sndThread;
    RcvThread * rcvThread;

    ServerThread * serverThread = new ServerThread(server_address[role]);
    serverThread->Start();

    string target_str = server_address[party_role];
    std::shared_ptr<grpc::Channel> chl = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());

    CLock * lock = new CLock();

    if (!chl->WaitForConnected(std::chrono::system_clock::now() +
                               std::chrono::milliseconds(10000))) {
        LOG(INFO)<< "try to connect server " << target_str << " failed" << std::endl;
    } else {
        LOG(INFO)<< " connect server " << target_str << " succeed" << std::endl;

    client = new RpcClient(
            chl, role);
    LOG(INFO) << "client started";

    rcvThread = new RcvThread(party_role, serverThread->get_server());
    LOG(INFO) << "rcv thread started";
    rcvThread->Start();

    sndThread = new SndThread(client, lock);
    LOG(INFO) << "snd thread started";
    sndThread->Start();

    uint32_t task_size = 1;
//    vector<std::thread>task_threads(task_size);

//    for (int j = 0; j < task_size ; j++) {
//        task_threads[j] = std::thread([&, j](){
            channel *cchl = new channel(0, rcvThread, sndThread);
            string send_buf = "party " + to_string(role) + " send test " + to_string(0);
            cchl->send(send_buf);
            uint32_t number = 6661;
            cchl->send(&number, sizeof(number));
            string recv_buf;
            cchl->recv(recv_buf);
            uint32_t recv_num;
            cchl->recv(&recv_num, sizeof(recv_num));
            LOG(INFO) << "party " << role << " cchl " << 0 <<  " recved : " << recv_buf;
        LOG(INFO) << "party " << role << " cchl " << 0 <<  " recved : " << recv_num;
        cout << "party " << role << " cchl " << 0 <<  " recved : " << recv_num;
        delete cchl;
    }
//        });
//    }

//    for (int j = 0; j < task_size ; j++) {
//        task_threads[j].join();
//    }

    delete sndThread;
    delete rcvThread;
    delete lock;
    delete client;
    delete serverThread;

}


