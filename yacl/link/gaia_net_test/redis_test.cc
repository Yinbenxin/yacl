
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <gtest/gtest.h>
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>
using namespace sw::redis;

TEST(redis_client, init) {



    try {
        // Create an Redis object, which is movable but NOT copyable.
        auto redis = Redis("tcp://127.0.0.1:6379");

        // ***** STRING commands *****

        redis.set("key", "val");
        auto val = redis.get("key");    // val is of type OptionalString. See 'API Reference' section for details.
        if (val) {
            // Dereference val to get the returned value of std::string type.
            std::cout << *val << std::endl;
        }   // else key doesn't exist.

        // ***** LIST commands *****

        // std::vector<std::string> to Redis LIST.
        std::vector<std::string> vec = {"a", "b", "c"};
	    redis.del("list");
        redis.rpush("list", vec.begin(), vec.end());

        // std::initializer_list to Redis LIST.
        redis.rpush("list", {"a", "b", "c"});

        // Redis LIST to std::vector<std::string>.
        vec.clear();
        redis.lrange("list", 0, -1, std::back_inserter(vec));
        for (auto item: vec){
            std::cout << item << std::endl;
        }

        // ***** HASH commands *****

    } catch (const Error &e) {
        // Error handling.
        std::cout << "catch error " << e.what() << std::endl;
    }
}

TEST(redis_client, sentinel) {
    try {
        auto password = getenv("REDIS_CLUSTER_PASSWORD");
        auto master_name = getenv("REDIS_SENTINEL_MASTER");
        auto redis_nodes = getenv("REDIS_CLUSTER_NODES");
        SentinelOptions sentinel_opts;
        if (redis_nodes){
            std::cout << "use  redis_cluster redis_nodes {}  from env value " <<redis_nodes << std::endl;
        } else {
            std::cout<<"get redis_cluster redis_nodes from env value failed" <<std::endl;
        }
        std::vector<std::pair<std::string, int>> nodes;

        nlohmann::json jsonArray = nlohmann::json::parse(redis_nodes);
        for (const auto& element : jsonArray) {
            std::string host = element["host"];
            int port = element["port"];
            nodes.push_back(std::make_pair(host, port));
        }

	sentinel_opts.password = "redis123";
        sentinel_opts.nodes = nodes;
        sentinel_opts.nodes = {{"10.100.66.69", 26379},{"10.100.66.69", 26380},{"10.100.66.69", 26381}};
	sentinel_opts.connect_timeout = std::chrono::milliseconds(200);
	sentinel_opts.socket_timeout = std::chrono::milliseconds(200);

        auto sentinel = std::make_shared<Sentinel>(sentinel_opts);
        ConnectionOptions connection_opts;
        if (password) {
            std::cout<<"use  redis_cluster password  from env value" <<std::endl;
        } else {
            std::cout<<"get redis_cluster password from env value failed" <<std::endl;
        }

        if (master_name) {
            std::cout << "use  redis_cluster master_name  from env value " <<master_name << std::endl;
            } else {
            std::cout <<"get redis_cluster master_name s from env value failed" <<std::endl;
        }

        connection_opts.password = "redis123";  // Optional. No password by default.
        connection_opts.connect_timeout = std::chrono::milliseconds(100);   // Required.
        connection_opts.socket_timeout = std::chrono::milliseconds(100);    // Required.
	ConnectionPoolOptions pool_opts;
	pool_opts.size = 3; // Optional. The default size is 1.
        auto redis = Redis(sentinel, master_name, Role::MASTER, connection_opts, pool_opts);
        redis.set("key", "value");
    } catch (const Error &e) {
    // Error handling.
    std::cout << "catch error " << e.what() << std::endl;
    }
}
