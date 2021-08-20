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
// json.h abstracts the json implementation used

#pragma once

#include <Arduino.h>
#include <FirebaseJson.h>

#include <functional>
#include <type_traits>
#include <vector>

namespace impl {
template <typename T>
struct json_t {
  json_t() {}

  bool set(const String& key, const String& value) {
    return static_cast<T*>(this)->set_impl(key, value);
  }

  bool set(const String& key, const int value) {
    return static_cast<T*>(this)->set_impl(key, value);
  }

  template <typename V>
  bool set(const String& key, const std::vector<V>& value) {
    return static_cast<T*>(this)->template add_impl<V>(key, value);
  }

  template <typename V>
  static String as_json_array(const std::vector<V>& value) {
    return T::template as_json_array_impl<V>(value);
  }

  template <typename V>
  V get(const String& key, bool& success) {
    return static_cast<T*>(this)->template get_impl<V>(key, success);
  }

  template <typename V>
  V get(const String& key) {
    bool dummy;
    return get<V>(key, dummy);
  }

  template <typename V, typename F>
  bool get_if_exists(const String& key, F setValue) {
    bool exists;
    auto tmp_value = get<V>(key, exists);
    if (exists) {
      setValue(tmp_value);
    }
    return exists;
  }

  bool loads(const String& data) {
    return static_cast<T*>(this)->loads_impl(data);
  }
  String dumps() { return static_cast<T*>(this)->dumps_impl(); }
};

struct firebase_json_t : json_t<firebase_json_t> {
  friend struct json_t<firebase_json_t>;

 private:
  bool set_impl(const String& key, const String& value) {
    _doc.set(key, value);
    return true;
  }
  bool set_impl(const String& key, const int value) {
    _doc.set(key, value);
    return true;
  }

  template <typename V, typename = std::enable_if_t<std::is_integral_v<V>>>
  int get_impl(const String& key, bool& success) {
    auto data = get_data(key);
    success = data.success;

    if (data.type == "string") {
      return data.stringValue.toInt();
    } else if (data.type == "int") {
      return data.intValue;
    }

    return {};
  }

  template <typename V, typename = std::enable_if_t<std::is_same_v<V, String>>>
  String get_impl(const String& key, bool& success) {
    auto data = get_data(key);
    success = data.success;

    if (data.type == "string") {
      return data.stringValue;
    }
    return {};
  }

  FirebaseJsonData get_data(const String& key) {
    FirebaseJsonData data;
    _doc.get(data, key);
    return data;
  }

  bool loads_impl(const String& data) {
    _doc.setJsonData(data);
    return true;
  }
  String dumps_impl() {
    String output;
    _doc.toString(output);
    return output;
  }

  template <typename V>
  static String as_json_array_impl(const std::vector<V>& value) {
    FirebaseJsonArray arr;
    String output;

    for (auto& v : value) {
      arr.add(v);
    }

    arr.toString(output);
    return output;
  }

  FirebaseJson _doc;
};
}  // namespace impl

using json_t = impl::firebase_json_t;