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
 *a
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "iserver.h"
#include "options.h"

namespace mg {

struct HttpSrvOptions;
struct MqttSrvOptions;
using HttpSrvBase = IServer<HttpSrvOptions>;
using MqttSrvBase = IServer<MqttSrvOptions>;

struct HttpSrvOptions : Options<HttpSrvBase> {
  using Ptr = std::shared_ptr<HttpSrvOptions>;
  //TODO
  OnHttpMessage<HttpSrvBase> on_message;
};

struct MqttSrvOptions : Options<MqttSrvBase> {
  using Ptr = std::shared_ptr<HttpSrvOptions>;
  //TODO
  OnHttpMessage<MqttSrvBase> on_message;
};

class HttpServer : public HttpSrvBase {
 public:
  HttpServer(HttpSrvOptions options);
  virtual ~HttpServer();

 private:
  virtual void Handler(struct mg_connection* c, int ev, void* ev_data) override;

 private:
  virtual void InitLoop() override;

};

}  // namespace mg
