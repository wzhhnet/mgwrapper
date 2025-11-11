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

#include "connect.h"
#include "mongoose.h"

namespace mg {

static void EventHandler(struct mg_connection* c, int ev, void* ev_data) {
  if (auto* conn = static_cast<IConnect*>(c->fn_data); conn) {
    conn->Handler(ev, ev_data);
  } else {
    c->is_draining = 1;
  }
}

static HttpHeaders ReverseParseHeaders(struct mg_http_message* hm) {
  HttpHeaders headers;
  for (size_t i = 0; i < MG_MAX_HTTP_HEADERS && hm->headers[i].name.len > 0;
       i++) {
    headers.emplace(
        std::string(hm->headers[i].name.buf, hm->headers[i].name.len),
        std::string(hm->headers[i].value.buf, hm->headers[i].value.len));
  }
  return headers;
}

bool IConnect::Send(std::string_view body) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (!c)
    return false;
  return mg_send(c, body.data(), body.size());
}

TcpConnect::TcpConnect(Options options)
    : IConnectImpl<Options>(std::move(options)) {}

void TcpConnect::Init(void* data) {
  struct mg_mgr* mgr = static_cast<mg_mgr*>(data);
  auto* c = mg_connect(mgr, options_.url.c_str(), &EventHandler,
                       static_cast<void*>(this));
  mgc_ = static_cast<void*>(c);
}

void TcpConnect::Handler(int ev, void* ev_data) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (ev == MG_EV_CONNECT && options_.on_connect) {
    options_.on_connect(this);
  } else if (ev == MG_EV_READ && options_.on_message) {
    Message msg = {std::string_view(reinterpret_cast<const char*>(c->recv.buf),
                                    c->recv.len)};
    options_.on_message(msg);
    /// Tell Mongoose we've consumed data
    mg_iobuf_del(&c->recv, 0, c->recv.len);
  } else if (ev == MG_EV_ERROR && options_.on_error) {
    options_.on_error(std::string_view(static_cast<const char*>(ev_data)));
  }
}

HttpConnect::HttpConnect(HttpOptions options)
    : IConnectImpl<HttpOptions>(std::move(options)) {
  if (options_.file) {
    auto* mgfd = mg_fs_open(&mg_fs_posix, options_.file->c_str(), MG_FS_READ);
    if (mgfd) {
      time_t tm;
      size_t fs;
      mgfd->fs->st(options_.file->c_str(), &fs, &tm);
      options_.headers.emplace(
          std::make_pair("Content-Type", "application/octet-stream"));
      options_.headers.emplace(
          std::make_pair("Content-Length", std::to_string(fs)));
      mgfd_ = mgfd;
    }
  }
}

void HttpConnect::Handler(int ev, void* ev_data) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (ev == MG_EV_CONNECT) {
    Request();
  } else if (ev == MG_EV_HTTP_MSG && options_.on_message) {
    auto* hm = static_cast<struct mg_http_message*>(ev_data);
    HttpMessage msg = {.status = mg_http_status(hm),
                       .headers = ReverseParseHeaders(hm)};
    msg.body = std::string_view(hm->body.buf, hm->body.len);
    options_.on_message(msg);
  } else if (ev == MG_EV_WRITE && mgfd_ != nullptr) {
    char buf[MG_IO_SIZE];
    size_t len = MG_IO_SIZE - c->send.len;
    auto* fd = static_cast<struct mg_fd*>(mgfd_);
    size_t rlen = fd->fs->rd(fd->fd, buf, len);
    if (rlen) {
      mg_send(c, buf, rlen);
      LOGI("post size=%u", rlen);
    } else {
      mg_fs_close(fd);
      mgfd_ = nullptr;
    }
  } else if (ev == MG_EV_ERROR && options_.on_error) {
    options_.on_error(std::string_view(static_cast<const char*>(ev_data)));
  } else if (ev == MG_EV_CLOSE && options_.on_closed) {
    if (mgfd_) {
      mg_fs_close(static_cast<struct mg_fd*>(mgfd_));
    }
    options_.on_closed();
  }
}

void HttpConnect::Init(void* data) {
  struct mg_mgr* mgr = static_cast<mg_mgr*>(data);
  auto* c = mg_http_connect(mgr, options_.url.c_str(), &EventHandler,
                            static_cast<void*>(this));
  mgc_ = static_cast<void*>(c);
}

void HttpConnect::Request() {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  struct mg_str host = mg_url_host(options_.url.c_str());
  const char* uri = mg_url_uri(options_.url.c_str());
  auto hstr = ParseHeaders();
  if (options_.cert && mg_url_is_ssl(options_.url.c_str())) {
    struct mg_tls_opts opts = {.ca = mg_unpacked(options_.cert->c_str()),
                               .name = host};
    mg_tls_init(c, &opts);
  }
  mg_printf(c,
            "%s %s HTTP/1.0\r\n"  // method uri
            "Host: %.*s\r\n"      // host
            "%s"                  // other headers
            "\r\n",
            options_.method.c_str(), uri, (int)host.len, host.buf,
            hstr.c_str());
  if (options_.body) {
    IConnect::Send(*options_.body);
  }
}

std::string HttpConnect::ParseHeaders() {
  std::string hstr;
  for (const auto& [key, value] : options_.headers) {
    hstr += key + ": " + value + "\r\n";
  }
  return hstr;
}

}  // namespace mg