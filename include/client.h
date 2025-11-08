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

#include <mutex>
#include <queue>
#include <set>
#include "iloop.h"
#include "connect.h"

namespace mg {

class IClient : public ILoop {
 public:
  IClient() = default;
  virtual ~IClient() { Stop(); }

  template <class CONNECT, class... Args>
  Base::Ptr Create(Args&&... args) {
    auto c = std::make_shared<CONNECT>(std::forward<Args>(args)...);
    if (Add(c))
      return c;
    return nullptr;
  }

 private:
  virtual bool EventLoop() override;
  bool Add(Base::Ptr conn);
  Base* Pop();

 private:
  std::mutex mtx_;
  std::set<Base::Ptr> sess_set_;
  std::queue<Base*> sess_queue_;
};

}  // namespace mg