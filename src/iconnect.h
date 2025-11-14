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
#include "mongoose.h"

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

 protected:
  static void Callback(struct mg_connection* c, int ev, void* ev_data);

 protected:
  struct mg_connection* mgc_ = nullptr;
  std::function<void(Ptr)> on_release;
};

template <class OPTIONS>
class TcpConnect : public IConnect {
 public:
  TcpConnect(OPTIONS options) : options_(std::move(options)) {}

 private:
  void Init(struct mg_mgr* mgr) override {
    mgc_ = mg_connect(mgr, options_.url.c_str(), &IConnect::Callback,
                      static_cast<void*>(this));
  }

 protected:
  void Handler(int ev, void* ev_data) override {
    switch (ev) {
      case MG_EV_ERROR:
        if (options_.on_error) {
          options_.on_error(
              this, std::string_view(static_cast<const char*>(ev_data)));
        }
        break;
      case MG_EV_OPEN:
        if (options_.on_open) {
          options_.on_open(this);
        }
        break;
      case MG_EV_CLOSE:
        if (options_.on_close) {
          options_.on_close(this);
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
      case MG_EV_CONNECT:
        if (options_.on_connect) {
          options_.on_connect(this);
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