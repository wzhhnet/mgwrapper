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

#include <atomic>
#include <thread>
#include "mongoose.h"

namespace mg {

class ILoop {
 public:
  ILoop()
      : exit_(false),
        thread_(std::make_unique<std::thread>(&ILoop::StartRoutine, this)) {}

  virtual ~ILoop() = default;

 protected:
  void Stop() {
    exit_ = true;
    if (thread_ && thread_->joinable()) {
      thread_->join();
      thread_ = nullptr;
    }
  }

  bool Stopped() {
    return exit_;
  }

  void Flush() {
    for (auto* c = mgr_.conns; c != NULL; c = c->next) {
      c->is_draining = 1;
    }
  }

 private:
  virtual bool EventLoop() = 0;

  virtual void InitLoop() {
    mg_log_set(MG_LL_INFO);
    mg_mgr_init(&mgr_);
  }

  virtual void UninitLoop() { mg_mgr_free(&mgr_); }

  void StartRoutine() {
    InitLoop();
    while (EventLoop()) {};
    UninitLoop();
  }

 protected:
  struct mg_mgr mgr_;

 private:
  std::atomic<bool> exit_;
  std::unique_ptr<std::thread> thread_;
};

}  // namespace mg