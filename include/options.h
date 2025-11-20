/*
 * Mongoose Wrapper
 * Implemented by C++
 *
 * Author wanch
 * Date 2025/11/18
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

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <functional>

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

template <class T>
using OnClose = std::function<void(T*, std::string_view)>;

template <class T>
using OnRead = std::function<void(T*, std::string_view)>;

template <class T>
using OnReady = std::function<void(T*)>;

template <class T>
using OnMqttOpen = std::function<void(T*)>;

template <class T>
using OnHttpMessage = std::function<void(T*, HttpMessage)>;

template <class T>
using OnMqttMessage = std::function<void(T*, MqttMessage)>;


template <class T>
struct Options {
  using Ptr = std::shared_ptr<Options<T>>;
  std::string url;
  std::string ca;
  std::string cert;
  std::string key;
  uint32_t timeout; // timeout of connection, milliseconds
  OnRead<T> on_read; // data received
  OnClose<T> on_close; // connection closed
  OnReady<T> on_ready; // connection established
};

}

