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

#include <string_view>
#include "iloop.h"

namespace mg {

template <class OPTIONS>
class IServer : public ILoop {
 public:
  IServer(OPTIONS options) : options_(std::move(options)) {};
  virtual ~IServer() { Stop(); }

 protected:
  static void Callback(struct mg_connection* c, int ev, void* ev_data) {
    auto* srv = static_cast<IServer<OPTIONS>*>(c->fn_data);
    srv->Handler(c, ev, ev_data);
  }

  void InitTls(struct mg_connection* c) {
    struct mg_tls_opts opts = {.ca = mg_unpacked(options_.ca.c_str()),
                               .cert = mg_unpacked(options_.cert.c_str()),
                               .key = mg_unpacked(options_.key.c_str()),
                               .name = mg_url_host(options_.url.c_str())};
    mg_tls_init(c, &opts);
  }

  virtual void Handler(struct mg_connection* c, int ev, void* ev_data) {
    switch (ev) {
      case MG_EV_CLOSE:
        if (options_.on_close) {
          options_.on_close(this, "");
        }
        break;
      case MG_EV_READ:
        if (options_.on_read) {
          options_.on_read(
              this, std::string_view(reinterpret_cast<const char*>(c->recv.buf),
                                     c->recv.len));
          /// Tell Mongoose we've consumed data
          mg_iobuf_del(&c->recv, 0, c->recv.len);
        }
        break;
      case MG_EV_ACCEPT:
        if (mg_url_is_ssl(options_.url.c_str())) {
          InitTls(c);
        } else {
          mg_call(c, MG_EV_USER_READY, this);
        }
        break;
      case MG_EV_TLS_HS:
        mg_call(c, MG_EV_USER_READY, this);
        break;
      case MG_EV_USER_READY:
        if (options_.on_ready) {
          options_.on_ready(this);
        }
        break;
      default:
        break;
    }
  }

  virtual void InitLoop() override {
    ILoop::InitLoop();
    mg_listen(&mgr_, options_.url.data(), &IServer::Callback, this);
  }

  virtual bool EventLoop() override {
    mg_mgr_poll(&mgr_, 50);
    if (Stopped()) {
      if (mgr_.conns == NULL)
        return false;  // exit loop
      Flush();
    }
    return true;
  }

 protected:
  OPTIONS options_;
};

}  // namespace mg