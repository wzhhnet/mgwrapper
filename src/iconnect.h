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

#include <memory>
#include <string>
#include <string_view>
#include "common.h"

namespace mg {

class IConnect : virtual public std::enable_shared_from_this<IConnect> {
  friend class IClient;

 public:
  using Ptr = std::shared_ptr<IConnect>;
  virtual bool Send(std::string_view body);
  bool kill();

 private:
  virtual void Init(struct mg_mgr* mgr) = 0;
  virtual void Handler(int ev, void* ev_data) = 0;
  virtual void OnTimeout() = 0;

 protected:
  static void Timeout(void* fn_data);
  static void Callback(struct mg_connection* c, int ev, void* ev_data);
  void StartTimer(uint64_t period_ms, unsigned flags);

 protected:
  struct mg_mgr* mgr_ = nullptr;
  struct mg_connection* mgc_ = nullptr;
  std::string cause_ = "normal";
  std::function<void(Ptr)> on_release;
};

template <class OPTIONS>
class TcpConnect : public IConnect {
 public:
  TcpConnect(OPTIONS options) : options_(std::move(options)) {}

 private:
  virtual void OnTimeout() {
    cause_ = "connection timeout";
    kill();
  }

  void Init(struct mg_mgr* mgr) override {
    mgr_ = mgr;
    mgc_ = mg_connect(mgr, options_.url.c_str(), &IConnect::Callback,
                      static_cast<void*>(this));
  }

 protected:
  void InitTls() {
    struct mg_tls_opts opts = {.ca = mg_unpacked(options_.ca.c_str()),
                               .cert = mg_unpacked(options_.cert.c_str()),
                               .key = mg_unpacked(options_.key.c_str()),
                               .name = mg_url_host(options_.url.c_str())};
    mg_tls_init(mgc_, &opts);
  }

  void Handler(int ev, void* ev_data) override {
    switch (ev) {
      case MG_EV_ERROR:
        cause_ = static_cast<const char*>(ev_data);
        break;
      case MG_EV_OPEN:
        if (options_.timeout) {
          StartTimer(options_.timeout, MG_TIMER_RUN_NOW);
        }
        break;
      case MG_EV_CLOSE:
        if (options_.on_close) {
          options_.on_close(this, cause_);
        }
        if (this->on_release) {
          this->on_release(this->shared_from_this());
        }
        break;
      case MG_EV_READ:
        if (options_.on_read) {
          options_.on_read(
              this,
              std::string_view(reinterpret_cast<const char*>(mgc_->recv.buf),
                               mgc_->recv.len));
          /// Tell Mongoose we've consumed data
          mg_iobuf_del(&mgc_->recv, 0, mgc_->recv.len);
        }
        break;
      case MG_EV_CONNECT: {
        if (mg_url_is_ssl(options_.url.c_str())) {
          InitTls();
        } else {
          mg_call(mgc_, MG_EV_USER_READY, this);
        }
        break;
      }
      case MG_EV_TLS_HS:
        mg_call(mgc_, MG_EV_USER_READY, this);
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

 protected:
  OPTIONS options_;
};

}  // namespace mg