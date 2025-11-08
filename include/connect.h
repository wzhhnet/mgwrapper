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
  std::function<void(const Message&)> on_message;
  std::function<void(std::string_view)> on_error;
};

struct HttpOptions : public Options {
  std::string method;
  HttpHeaders headers;
  std::optional<std::string> body;
  std::optional<std::string> file;
  std::optional<std::string> cert;
};

struct MqttOption : public Options {
  //TODO
};

class Base {
 public:
  using Ptr = std::shared_ptr<Base>;

  virtual void Init(void* data) = 0;
  virtual void Handler(int ev, void* ev_data) = 0;
};

template <class OPTIONS>
class IConnect : public Base {
 public:
  IConnect(OPTIONS options) : options_(std::move(options)) {}

 private:
  virtual void Init(void* data) = 0;
  virtual void Handler(int ev, void* ev_data) = 0;

 protected:
  void* mgc_ = nullptr;
  OPTIONS options_;
};

class TcpConnect : public IConnect<Options> {
 public:
  TcpConnect(Options options);
  bool Send(std::string_view body);

 private:
  virtual void Init(void* data) override;
  virtual void Handler(int ev, void* ev_data) override;
};

class HttpConnect : public IConnect<HttpOptions> {
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

}  // namespace mg
