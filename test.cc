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

using namespace mg;

namespace test {

class HttpTest : public ::testing::Test {

 protected:
  std::condition_variable cv_;
  std::mutex cv_mtx_;
  void SetUp() override {
  }
  void TearDown() override {}

  void GetSwitchCallback() {}
};

TEST_F(HttpTest, GET) {
  std::condition_variable cv;
  std::mutex cv_mtx;
  IClient client;
  HttpOptions opt = {.method = "GET"};
  opt.body = "";
  opt.url = "http://httpbin.org/get?user=chaohui&id=42";
  opt.on_closed = [&]() {
    LOGI("on_closed");
    cv.notify_all();
  };
  opt.on_message = [&](const Message& msg) {
    LOGI("on_message: %.*s", msg.body.size(), msg.body.data());
  };
  opt.on_error = [](std::string_view msg) {
    LOGI("on_error: %.*s", msg.size(), msg.data());
  };
  client.Create<HttpConnect>(std::move(opt));
  std::unique_lock<std::mutex> lk(cv_mtx);
  cv.wait_for(lk, std::chrono::seconds(10));
}

}  // namespace test

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
