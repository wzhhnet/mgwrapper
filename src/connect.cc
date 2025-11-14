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

void IConnect::Timeout(void* fn_data)
{
   auto* conn = static_cast<IConnect*>(fn_data);
   conn->cause_ = "connection timeout";
   conn->kill();
}

void IConnect::Callback(struct mg_connection* c, int ev, void* ev_data) {
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
  return mg_send(mgc_, body.data(), body.size());
}

bool IConnect::kill() {
  if (!mgc_) return false;
  mgc_->is_draining = 1;
  return true;
}

HttpConnect::HttpConnect(HttpOptions options)
    : TcpConnect<HttpOptions>(std::move(options)) {
  if (!options_.file.empty()) {
    mgfd_ = mg_fs_open(&mg_fs_posix, options_.file.c_str(), MG_FS_READ);
    if (mgfd_) {
      time_t tm;
      size_t fs;
      mgfd_->fs->st(options_.file.c_str(), &fs, &tm);
      options_.headers.emplace(
          std::make_pair("Content-Type", "application/octet-stream"));
      options_.headers.emplace(
          std::make_pair("Content-Length", std::to_string(fs)));
    }
  }
}

void HttpConnect::Handler(int ev, void* ev_data) {
  if (ev == MG_EV_CONNECT) {
    Request();
  } else if (ev == MG_EV_HTTP_MSG && options_.on_message) {
    auto* hm = static_cast<struct mg_http_message*>(ev_data);
    HttpMessage msg = {.status = mg_http_status(hm),
                       .headers = ReverseParseHeaders(hm)};
    msg.body = std::string_view(hm->body.buf, hm->body.len);
    options_.on_message(this, std::move(msg));
  } else if (ev == MG_EV_WRITE && mgfd_ != nullptr) {
    char buf[MG_IO_SIZE];
    size_t len = MG_IO_SIZE - mgc_->send.len;
    size_t rlen = mgfd_->fs->rd(mgfd_->fd, buf, len);
    if (rlen) {
      mg_send(mgc_, buf, rlen);
      LOGI("post size=%u", rlen);
    } else {
      mg_fs_close(mgfd_);
      mgfd_ = nullptr;
    }
  } else if (ev == MG_EV_CLOSE) {
    if (mgfd_) {
      mg_fs_close(mgfd_);
    }
  }
  TcpConnect<HttpOptions>::Handler(ev, ev_data);
}

void HttpConnect::Init(struct mg_mgr* mgr) {
  mgr_ = mgr;
  mgc_ = mg_http_connect(mgr, options_.url.c_str(), &IConnect::Callback,
                         static_cast<void*>(this));
}

void HttpConnect::Request() {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  struct mg_str host = mg_url_host(options_.url.c_str());
  const char* uri = mg_url_uri(options_.url.c_str());
  auto hstr = ParseHeaders();
  if (mg_url_is_ssl(options_.url.c_str())) {
    struct mg_tls_opts opts = {.ca = mg_unpacked(options_.cert.c_str()),
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
  if (!options_.body.empty()) {
    IConnect::Send(options_.body);
  }
}

std::string HttpConnect::ParseHeaders() {
  std::string hstr;
  for (const auto& [key, value] : options_.headers) {
    hstr += key + ": " + value + "\r\n";
  }
  return hstr;
}

MqttConnect::MqttConnect(MqttOptions options)
    : TcpConnect<MqttOptions>(std::move(options)) {}

void MqttConnect::Init(struct mg_mgr* mgr) {
  struct mg_mqtt_opts opts = {
      .user = mg_str(options_.user.c_str()),
      .pass = mg_str(options_.pass.c_str()),
      .qos = options_.qos,
  };
  mgr_ = mgr;
  mgc_ = mg_mqtt_connect(mgr, options_.url.c_str(), &opts, &IConnect::Callback,
                         static_cast<void*>(this));
}

void MqttConnect::Handler(int ev, void* ev_data) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (ev == MG_EV_MQTT_OPEN) {
    struct mg_mqtt_opts opt = {
        .user = mg_str(options_.user.c_str()),
        .pass = mg_str(options_.pass.c_str()),
        .qos = options_.qos,
    };
    for (const auto& topic : options_.topics) {
      opt.topic = mg_str(topic.c_str());
      mg_mqtt_sub(c, &opt);
    }
    if (options_.on_mqtt_open) {
      options_.on_mqtt_open(this);
    }
  } else if (ev == MG_EV_MQTT_MSG && options_.on_message) {
    struct mg_mqtt_message* mm = (struct mg_mqtt_message*)ev_data;
    MqttMessage msg = {.topic = std::string_view(mm->topic.buf, mm->topic.len)};
    msg.body = std::string_view(mm->data.buf, mm->data.len);
    options_.on_message(this, std::move(msg));
  }
  TcpConnect<MqttOptions>::Handler(ev, ev_data);
}

bool MqttConnect::Publish(MqttMessage msg) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (!c)
    return false;
  struct mg_mqtt_opts pub_opts = {
      .topic = mg_str(msg.topic.data()),
      .message = mg_str(msg.body.data()),
      .qos = options_.qos,
  };
  mg_mqtt_pub(c, &pub_opts);
  return true;
}

bool MqttConnect::Subscribe(std::string_view topic) {
  auto* c = static_cast<struct mg_connection*>(mgc_);
  if (!c)
    return false;
  struct mg_mqtt_opts opt = {
      .user = mg_str(options_.user.c_str()),
      .pass = mg_str(options_.pass.c_str()),
      .qos = options_.qos,
  };
  opt.topic = mg_str(topic.data());
  mg_mqtt_sub(c, &opt);
  return true;
}

}  // namespace mg