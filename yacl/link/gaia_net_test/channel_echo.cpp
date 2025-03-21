#include "channel.h"
// #include "// gaialog.h"
#include "gaianet.h"
#include "gaiarandom.h"
#include "memchannel.h"
#include <gflags/gflags.h>
#include <iostream>
#include <thread>

DEFINE_string(taskid, "channel_echo_", "task id, defalut is 'channel_echo_'");
DEFINE_int32(role, -1, "role, defalut value is -1, mean run all role");
DEFINE_string(type, "grpc", "channel type: 'grpc', 'mem', current default is 'grpc'");
DEFINE_int32(enable_cv, 1, "use condition variables instead spinlock");
DEFINE_string(redis_uri, "tcp://127.0.0.1:6379", "redis url, default is 'tcp://127.0.0.1:6379'");
DEFINE_string(server_addr, "127.0.0.1:9900", "grpc server addr, default is '127.0.0.1:9900'");
DEFINE_int32(data_size, 1024 * 16, "test data size");
DEFINE_int32(log_level, 3, "log level, 0-6, default is 0");

static inline std::string get_rand_string() {
    // int len = 4096 * 16 - 4;
    uint32_t seed = gaiarandom::fast_seed();
    int len = rand_r(&seed) % FLAGS_data_size;
    std::string ret;
    ret.reserve(len);
    for (int i = 0; i < len; ++i) {
        ret.push_back(rand_r(&seed));
    }
    return ret;
}

static inline std::unique_ptr<gaianet::IChannel> create_channel(const std::string& taskid, uint32_t from, uint32_t to) {
    if (FLAGS_type == "grpc") {
        return std::unique_ptr<gaianet::IChannel>(new gaianet::channel(from, to, taskid, FLAGS_server_addr, FLAGS_redis_uri));
    } else if (FLAGS_type == "mem") {
        return std::unique_ptr<gaianet::IChannel>(new gaianet::MemChannel(from, to, taskid, FLAGS_enable_cv != 0));
    } else {
        assert(false);
    }
}

static void test_client() {
    const std::string send_str = get_rand_string();
    auto chl = create_channel(FLAGS_taskid, 0, 1);
    chl->send(send_str.data(), send_str.length());

    std::string recv_str;
    chl->recv(recv_str);

    if (recv_str != send_str) {
        printf("channel test failed\n");
    } else {
        printf("channel test succeed\n");
    }
}

static void test_echo_server() {
    auto chl = create_channel(FLAGS_taskid, 1, 0);

    std::string recv_str;
    chl->recv(recv_str);
    chl->send(recv_str);
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    // gaialog::init("channel_echo", ".", true, FLAGS_log_level, 0, false);

    if (FLAGS_role == -1) {
        std::thread t0(test_client);
        std::thread t1(test_echo_server);
        t0.join();
        t1.join();
    } else if (FLAGS_role == 0) {
        test_client();
    } else if (FLAGS_role == 1) {
        test_echo_server();
    }
    gaianet::shutdown();
    // gaialog::shutdown();
    return 0;
}
