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
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#ifndef LOGE
#define LOGE(...) MG_ERROR((__VA_ARGS__))
#endif
#ifndef LOGW
#define LOGW(...) MG_WARNING((__VA_ARGS__))
#endif
#ifndef LOGI
#define LOGI(...) MG_INFO((__VA_ARGS__))
#endif
#ifndef LOGD
#define LOGD(...) MG_DEBUG((__VA_ARGS__))
#endif

namespace mg {

using HttpHeaders = std::map<std::string, std::string>;

struct HttpMessage {
  int status;
  HttpHeaders headers;
  std::string_view body;
};

struct MqttMessage {
  std::string_view topic;
  std::string_view body;
};

class IConnect;
using FnOnClose = std::function<void()>;
using FnOnError = std::function<void(IConnect*, std::string_view)>;
using FnOnRead = std::function<void(IConnect*, std::string_view)>;
using FnOnConnect = std::function<void(IConnect*)>;
using FnOnMqttOpen = std::function<void(IConnect*)>;
using FnOnHttpMessage = std::function<void(IConnect*, HttpMessage)>;
using FnOnMqttMessage = std::function<void(IConnect*, MqttMessage)>;

struct Options {
  std::string url;
  FnOnRead on_read;
  FnOnClose on_close;
  FnOnError on_error;
  FnOnConnect on_connect;
};

struct HttpOptions : public Options {
  std::string method;
  HttpHeaders headers;
  std::string body;
  std::string file;
  std::string cert;
  FnOnHttpMessage on_message;
};

struct MqttOptions : public Options {
  uint8_t qos;
  std::string user;
  std::string pass;
  std::string cert;
  std::vector<std::string> topics;
  FnOnMqttOpen on_mqtt_open;
  FnOnMqttMessage on_message;
};

class IConnect {
  friend class IClient;

 public:
  using Ptr = std::shared_ptr<IConnect>;
  virtual void Init(void* data) = 0;
  virtual void Handler(int ev, void* ev_data) = 0;
  virtual bool Send(std::string_view body);

 protected:
  void* mgc_ = nullptr;
};

template <class OPTIONS>
class IConnectImpl : public IConnect {
 public:
  IConnectImpl(OPTIONS options) : options_(std::move(options)) {}

 private:
  virtual void Init(void* data) = 0;
  virtual void Handler(int ev, void* ev_data) = 0;

 protected:
  OPTIONS options_;
};

class TcpConnect : public IConnectImpl<Options> {
 public:
  TcpConnect(Options options);

 private:
  virtual void Init(void* data) override;
  virtual void Handler(int ev, void* ev_data) override;
};

class HttpConnect : public IConnectImpl<HttpOptions> {
 public:
  HttpConnect(HttpOptions options);

 private:
  virtual void Init(void* data) override;
  virtual void Handler(int ev, void* ev_data) override;
  void Request();
  std::string ParseHeaders();

 private:
  void* mgfd_ = nullptr;
};

class MqttConnect : public IConnectImpl<MqttOptions> {
 public:
  MqttConnect(MqttOptions options);
  bool Publish(MqttMessage msg);
  bool Subscribe(std::string_view topic);

 private:
  virtual void Init(void* data) override;
  virtual void Handler(int ev, void* ev_data) override;
};

}  // namespace mg
