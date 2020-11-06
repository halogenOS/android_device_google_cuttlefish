/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "host/commands/assemble_cvd/alloc.h"

#include <iomanip>
#include <sstream>

#include "common/libs/fs/shared_fd.h"
#include "host/libs/allocd/request.h"
#include "host/libs/allocd/utils.h"

static std::string StrForInstance(const std::string& prefix, int num) {
  std::ostringstream stream;
  stream << prefix << std::setfill('0') << std::setw(2) << num;
  return stream.str();
}

IfaceConfig DefaultNetworkInterfaces(int num) {
  IfaceConfig config{};
  config.mobile_tap.name = StrForInstance("cvd-mtap-", num);
  config.mobile_tap.resource_id = 0;
  config.mobile_tap.session_id = 0;

  config.wireless_tap.name = StrForInstance("cvd-wtap-", num);
  config.wireless_tap.resource_id = 0;
  config.wireless_tap.session_id = 0;
  return config;
}

std::optional<IfaceConfig> AllocateNetworkInterfaces() {
  IfaceConfig config{};

  cuttlefish::SharedFD allocd_sock = cuttlefish::SharedFD::SocketLocalClient(
      cuttlefish::kDefaultLocation, false, SOCK_STREAM);
  CHECK(allocd_sock->IsOpen())
      << "Unable to connect to allocd on " << cuttlefish::kDefaultLocation
      << ": " << allocd_sock->StrError();

  Json::Value resource_config;
  Json::Value request_list;
  Json::Value req;
  req["request_type"] = "create_interface";
  req["uid"] = geteuid();
  req["iface_type"] = "mtap";
  request_list.append(req);
  req["iface_type"] = "wtap";
  request_list.append(req);

  resource_config["config_request"]["request_list"] = request_list;

  CHECK(cuttlefish::SendJsonMsg(allocd_sock, resource_config))
      << "Failed to send JSON to allocd";

  auto resp_opt = cuttlefish::RecvJsonMsg(allocd_sock);
  CHECK(resp_opt.has_value()) << "Bad response from allocd";
  auto resp = resp_opt.value();

  CHECK(resp.isMember("config_status") && !resp["config_status"].isString())
      << "Bad response from allocd: " << resp;

  CHECK_EQ(
      resp["config_status"].asString(),
      cuttlefish::StatusToStr(cuttlefish::RequestStatus::Success))
          <<"Failed to allocate interfaces " << resp;

  CHECK(resp.isMember("session_id") && resp["session_id"].isUInt())
      << "Bad response from allocd: " << resp;
  auto session_id = resp["session_id"].asUInt();

  CHECK(resp.isMember("response_list") && resp["response_list"].isArray())
      << "Bad response from allocd: " << resp;

  Json::Value resp_list = resp["response_list"];
  Json::Value mtap_resp;
  Json::Value wifi_resp;
  for (Json::Value::ArrayIndex i = 0; i != resp_list.size(); ++i) {
    auto ty = cuttlefish::StrToIfaceTy(resp_list[i]["iface_type"].asString());

    switch (ty) {
      case cuttlefish::IfaceType::mtap: {
        mtap_resp = resp_list[i];
        break;
      }
      case cuttlefish::IfaceType::wtap: {
        wifi_resp = resp_list[i];
        break;
      }
      default: {
        break;
      }
    }
  }

  if (!mtap_resp.isMember("iface_type")) {
    LOG(ERROR) << "Missing mtap response from allocd";
    return std::nullopt;
  }
  if (!wifi_resp.isMember("iface_type")) {
    LOG(ERROR) << "Missing wtap response from allocd";
    return std::nullopt;
  }

  config.mobile_tap.name = mtap_resp["iface_name"].asString();
  config.mobile_tap.resource_id = mtap_resp["resource_id"].asUInt();
  config.mobile_tap.session_id = session_id;

  config.wireless_tap.name = wifi_resp["iface_name"].asString();
  config.wireless_tap.resource_id = wifi_resp["resource_id"].asUInt();
  config.wireless_tap.session_id = session_id;

  return config;
}

