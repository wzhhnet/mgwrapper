/*
 * Mongoose Wrapper
 * Implemented by C++
 *
 * Author wanch
 * Date 2025/10/19
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <condition_variable>
#include <fstream>

#ifndef private
#define private public
#define protected public
#endif

#include "client.h"
#include "server.h"

using namespace mg;

namespace test {

class ConnectTest : public ::testing::Test {

 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(ConnectTest, Socket) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  ConnectOptions opt = {.url = "tcpbin.com:4242",
                        .on_read =
                            [&](IConnect* c, std::string_view msg) {
                              LOGI("on_message body:%.*s", msg.size(),
                                   msg.data());
                              cv.notify_all();
                            },
                        .on_close =
                            [&](IConnect* c, std::string_view cause) {
                              LOGI("on_close:%s", cause.data());
                            },
                        .on_connect =
                            [](IConnect* c) {
                              LOGI("on_connect");
                              auto r = c->Send("this is a tcp request\n");
                              LOGI("r = %d", r);
                            }};
  client.Create<Socket>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}  // namespace test

TEST_F(ConnectTest, Timeout) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  HttpConnectOptions opt = {.method = "GET"};
  opt.timeout = 1000;
  opt.url = "http://invalid.url";
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  client.Create<HttpConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}

TEST_F(ConnectTest, HttpGet) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  HttpConnectOptions opt = {.method = "GET"};
  opt.body = "";
  opt.url = "http://httpbin.org/get?user=chaohui&id=42";
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  opt.on_message = [&](IConnect* c, HttpMessage msg) {
    LOGI("on_message status=%d body:%.*s", msg.status, msg.body.size(),
         msg.body.data());
  };
  client.Create<HttpConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}

TEST_F(ConnectTest, HttpPost) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  HttpConnectOptions opt = {.method = "POST"};
  opt.body = "{\"user\":\"chaohui\", \"id\":42}";
  opt.url = "https://httpbin.org/post";
  opt.headers = {{"Content-Type", "application/json"},
                 {"Content-Length", std::to_string(opt.body.size())}};
  opt.cert = "/etc/ssl/certs/ca-certificates.crt";
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  opt.on_message = [&](IConnect* c, HttpMessage msg) {
    LOGI("on_message: status=%d body:%.*s", msg.status, msg.body.size(),
         msg.body.data());
  };
  client.Create<HttpConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}

TEST_F(ConnectTest, HttpPostFile) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  HttpConnectOptions opt = {.method = "POST"};
  opt.url = "https://httpbin.org/post";
  opt.cert = "/etc/ssl/certs/ca-certificates.crt";
  opt.file = "./CMakeCache.txt";
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  opt.on_message = [&](IConnect* c, HttpMessage msg) {
    LOGI("on_message: status=%d body:%.*s", msg.status, msg.body.size(),
         msg.body.data());
  };
  client.Create<HttpConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}

TEST_F(ConnectTest, MqttAutoSubscribe) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  MqttConnectOptions opt{};
  opt.url = "mqtt://broker.hivemq.com:1883";
  opt.cert = "/etc/ssl/certs/ca-certificates.crt";
  opt.topics = {"mg/123/rx", "mg/123/tx"};
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  opt.on_message = [&](IConnect* c, MqttMessage msg) {
    LOGI("on_message: topic=%.*s body:%.*s", msg.topic.size(), msg.topic.data(),
         msg.body.size(), msg.body.data());
    MqttMessage tmm;
    tmm.topic = "mg/123/tx";
    tmm.body = "Hello, this is WZHHNET speaking...";
    auto* mc = static_cast<MqttConnect*>(c);
    mc->Publish(std::move(tmm));
  };
  client.Create<MqttConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(5));
}

TEST_F(ConnectTest, MqttManualSubscribe) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  MqttConnectOptions opt{};
  opt.timeout = 3000;
  opt.url = "mqtt://broker.hivemq.com:1883";
  opt.cert = "/etc/ssl/certs/ca-certificates.crt";
  opt.on_close = [&](IConnect* c, std::string_view cause) {
    LOGI("on_close:%s", cause.data());
    cv.notify_all();
  };
  opt.on_mqtt_open = [](IConnect* c) {
    auto* mc = static_cast<MqttConnect*>(c);
    mc->Subscribe("mg/123/rx");
    LOGI("subscribe topic:mg/123/rx");
    mc->Subscribe("mg/123/tx");
    LOGI("subscribe topic:mg/123/rx");
  };
  opt.on_message = [&](IConnect* c, MqttMessage msg) {
    LOGI("on_message: topic=%.*s body:%.*s", msg.topic.size(), msg.topic.data(),
         msg.body.size(), msg.body.data());
    MqttMessage tmm;
    tmm.topic = "mg/123/tx";
    tmm.body = "Hello, this is WZHHNET speaking...";
    auto* mc = static_cast<MqttConnect*>(c);
    mc->Publish(std::move(tmm));
  };
  client.Create<MqttConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(5));
}

TEST_F(ConnectTest, StaticHttpServer) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  HttpSrvOptions opts;
  opts.url = "http://0.0.0.0:8000";
  opts.serve_dir = "./web_root";
  HttpServer server(std::move(opts));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(60));
}

}  // namespace test

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
