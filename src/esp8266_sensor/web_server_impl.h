// Home Sensor Network
// Copyright (C) 2021 Steven Giacomelli (steve@giacomelli.ca)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// web_server_impl.h abstracts the web server implementation.

#include <Arduino.h>
#include <ESP8266WebServerSecure.h>

#include "web_server_ssl.h"

#include <functional>
#include <memory>
#include <vector>

namespace impl {
struct web_server_t;
using web_server_handler_func_t = std::function<void(web_server_t&)>;

struct web_server_handler_t {
  Uri uri;
  HTTPMethod method;
  web_server_handler_func_t handler;
};

static const char web_server_header_connection[] PROGMEM = "Connection";
static const char web_server_header_connection_close[] PROGMEM = "close";

static const char html_mime_type[] PROGMEM = "text/html";
static const char css_mime_type[] PROGMEM = "text/css";
static const char js_mime_type[] PROGMEM = "application/javascript";
static const char json_mime_type[] PROGMEM = "application/json";

template <typename P>
struct web_server_data_t {
  web_server_data_t() = delete;
  web_server_data_t(PGM_P server_cert, PGM_P server_key, P* parent)
      : _server(443), _server_cache(5), _parent(parent) {
    _server.getServer().setRSACert(new BearSSL::X509List(server_cert),
                                   new BearSSL::PrivateKey(server_key));
    _server.getServer().setCache(&_server_cache);
  }

  void set_handlers(bool close_connection = true) {
    for (const auto& h : _parent->_handlers) {
      _server.on(h.uri, h.method, [&]() {
        if (close_connection) {
          _server.sendHeader(web_server_header_connection,
                             web_server_header_connection_close);
        }
        h.handler(*_parent);
      });
    }

    _server.onNotFound([&]() {
      if (close_connection) {
        _server.sendHeader(web_server_header_connection,
                           web_server_header_connection_close);
      }
      _parent->_not_found_handler(*_parent);
    });
  }

  BearSSL::ESP8266WebServerSecure& server() { return _server; }

  bool has_arg(const String& key) { return _server.hasArg(key); }

  String arg(const String& key) { return _server.arg(key); }

  String request_body() {
    if (_server.hasArg("plain")) {
      return _server.arg("plain");
    }
    return {};
  }

  const String& uri() const { return _server.uri(); }

  void send_P(int code, PGM_P content_type, PGM_P content) {
    _server.send_P(code, content_type, content);
  }

  void send(int code, const String& content_type, const String& content) {
    _server.send(code, content_type, content);
  }

 private:
  BearSSL::ESP8266WebServerSecure _server;
  BearSSL::ServerSessions _server_cache;
  P* _parent;
};

struct web_server_t {
  friend struct web_server_data_t<web_server_t>;
  web_server_t() = delete;
  web_server_t(std::vector<web_server_handler_t> handlers,
               web_server_handler_func_t not_found_handler)
      : _data(new web_server_data_t<web_server_t>(web_server_cert::cert,
                                                  web_server_cert::key,
                                                  this)),
        _handlers{handlers},
        _not_found_handler{not_found_handler} {
    _data->set_handlers();
  }

  web_server_t(const web_server_t& other) = delete;

  bool begin() {
    _data->server().begin();
    return true;
  }
  bool loop() {
    _data->server().handleClient();
    return true;
  }

  String request_body() { return _data->request_body(); }

  const String& uri() const { return _data->uri(); }

  void send_P(int code, PGM_P content_type, PGM_P content) {
    _data->send_P(code, content_type, content);
  }

  void send_html_P(int code, PGM_P content) {
    send_P(code, html_mime_type, content);
  }

  void send_css_P(int code, PGM_P content) {
    send_P(code, css_mime_type, content);
  }

  void send_js_P(int code, PGM_P content) {
    send_P(code, js_mime_type, content);
  }

  void send_json_P(int code, PGM_P content) {
    send_P(code, json_mime_type, content);
  }

  void send(int code, const String& content_type, const String& content) {
    _data->send(code, content_type, content);
  }

  void send_html(int code, const String& content) {
    send(code, html_mime_type, content);
  }

  void send_css(int code, const String& content) {
    send(code, css_mime_type, content);
  }

  void send_js(int code, const String& content) {
    send(code, js_mime_type, content);
  }

  void send_json(int code, const String& content) {
    send(code, json_mime_type, content);
  }

  static void on_not_found(web_server_t& server) {
    String message = "File Not Found\n";
    message += "URI: ";
    message += server.uri();
    message += "\n";
    server.send(404, "text/plain", message);
  }

 private:
  std::unique_ptr<web_server_data_t<web_server_t>> _data;
  std::vector<web_server_handler_t> _handlers;
  web_server_handler_func_t _not_found_handler;
};
}  // namespace impl

using web_server_t = impl::web_server_t;