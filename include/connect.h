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
#pragma once

#include <functional>
#include <map>
#include "iconnect.h"
#include "options.h"

namespace mg {

using ConnectOptions = Options<IConnect>;

struct HttpConnectOptions : ConnectOptions {
  using Ptr = std::shared_ptr<HttpConnectOptions>;
  std::string method;
  HttpHeaders headers;
  std::string body;
  std::string file;

  OnHttpMessage<IConnect> on_message;
};

struct MqttConnectOptions : ConnectOptions {
  using Ptr = std::shared_ptr<MqttConnectOptions>;
  uint8_t qos;
  std::string user;
  std::string pass;
  std::vector<std::string> topics;
  OnMqttOpen<IConnect> on_mqtt_open;
  OnMqttMessage<IConnect> on_message;
};

using Socket = TcpConnect<ConnectOptions>;

class HttpConnect : public TcpConnect<HttpConnectOptions> {
 public:
  HttpConnect(HttpConnectOptions options);

 private:
  virtual void Init(struct mg_mgr* mgr) override;
  virtual void Handler(int ev, void* ev_data) override;
  void Request();
  std::string ParseHeaders();

 private:
  struct mg_fd* mgfd_ = nullptr;
};

class MqttConnect : public TcpConnect<MqttConnectOptions> {
 public:
  MqttConnect(MqttConnectOptions options);
  bool Publish(MqttMessage msg);
  bool Subscribe(std::string_view topic);

 private:
  virtual void Init(struct mg_mgr* mgr) override;
  virtual void Handler(int ev, void* ev_data) override;
  virtual void OnTimeout() override;
};

}  // namespace mg
