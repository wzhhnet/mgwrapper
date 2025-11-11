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
class IConnect;

struct Message {
  std::string_view body;
};

struct HttpMessage : public Message {
  int status;
  HttpHeaders headers;
};

struct MqttMessage : public Message {
  std::string_view topic;
};

struct Options {
  std::string url;
  std::function<void()> on_closed;
  std::function<void(IConnect*)> on_connect;
  std::function<void(IConnect*, const Message&)> on_message;
  std::function<void(IConnect*, std::string_view)> on_error;
};

struct HttpOptions : public Options {
  std::string method;
  HttpHeaders headers;
  std::string body;
  std::string file;
  std::string cert;
};

struct MqttOptions : public Options {
  uint8_t qos;
  std::string user;
  std::string pass;
  std::string cert;
  std::vector<std::string> topics;
  std::function<void(IConnect*)> on_mqtt_open;
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
  void Publish(MqttMessage msg);

 private:
  virtual void Init(void* data) override;
  virtual void Handler(int ev, void* ev_data) override;
};

}  // namespace mg
