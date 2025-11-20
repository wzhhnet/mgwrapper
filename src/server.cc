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

#include "server.h"

namespace mg {

HttpServer::HttpServer(HttpSrvOptions options)
    : HttpSrvBase(std::move(options)) {}
HttpServer::~HttpServer() {
  Stop();
}
void HttpServer::Handler(struct mg_connection* c, int ev, void* ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    auto* hm = static_cast<struct mg_http_message*>(ev_data);
    if (!options_.serve_dir.empty()) {
      struct mg_http_serve_opts opts = {.root_dir = options_.serve_dir.c_str()};
      mg_http_serve_dir(c, hm, &opts);
      LOGI("serve dir:%s, uri:%.*s", options_.serve_dir.c_str(),
           (int)hm->uri.len, hm->uri.buf);
    }
  }
}

void HttpServer::InitLoop() {
  ILoop::InitLoop();
  mg_http_listen(&mgr_, options_.url.data(), &IServer::Callback, this);
}

}  // namespace mg