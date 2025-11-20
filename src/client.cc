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

#include "client.h"
#include "common.h"

namespace mg {

bool IClient::Add(IConnect::Ptr conn) {
  std::lock_guard<std::mutex> guard(mtx_);
  auto ret = sess_set_.emplace(conn);
  if (ret.second) {
    sess_queue_.push(ret.first->get());
    return true;
  }
  return false;
}

bool IClient::Remove(IConnect::Ptr conn)
{
  std::lock_guard<std::mutex> guard(mtx_);
  auto r = sess_set_.erase(conn);
  return r > 0 ? true : false;
}

IConnect* IClient::Pop() {
  std::lock_guard<std::mutex> guard(mtx_);
  IConnect* p = nullptr;
  if (!sess_queue_.empty()) {
    p = sess_queue_.front();
    sess_queue_.pop();
  }
  return p;
}

bool IClient::EventLoop() {
  if (IConnect* p = Pop(); p)
    p->Init(&mgr_);

  mg_mgr_poll(&mgr_, 50);
  if (Stopped()) {
    if (mgr_.conns == NULL)
      return false;  // exit loop
    Flush();
  }
  return true;
}

}  // namespace mg