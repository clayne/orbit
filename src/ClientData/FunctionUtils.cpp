// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ClientData/FunctionUtils.h"

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>

#include <filesystem>
#include <utility>

#include "OrbitBase/Logging.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "xxhash.h"

namespace orbit_client_data {

namespace {
uint64_t StringHash(const std::string& string) {
  return XXH64(string.data(), string.size(), 0xBADDCAFEDEAD10CC);
}
}  // namespace

namespace function_utils {

using orbit_client_protos::FunctionInfo;
using orbit_grpc_protos::SymbolInfo;

std::string GetLoadedModuleName(const FunctionInfo& func) {
  return GetLoadedModuleNameByPath(func.module_path());
}

std::string GetLoadedModuleNameByPath(std::string_view module_path) {
  return std::filesystem::path(module_path).filename().string();
}

uint64_t GetHash(const FunctionInfo& func) { return StringHash(func.pretty_name()); }
uint64_t GetHash(std::string_view function_name) { return StringHash(std::string(function_name)); }

uint64_t Offset(const FunctionInfo& func, const ModuleData& module) {
  return func.address() - module.load_bias();
}

std::optional<uint64_t> GetAbsoluteAddress(const orbit_client_protos::FunctionInfo& func,
                                           const ProcessData& process, const ModuleData& module) {
  std::vector<uint64_t> base_addresses =
      process.GetModuleBaseAddresses(module.file_path(), module.build_id());
  if (base_addresses.empty()) {
    return std::nullopt;
  }

  if (base_addresses.size() > 1) {
    ERROR(
        "Found multiple mappings for \"%s\" with build_id=%s [%s]: "
        "will use the first one as a base address",
        module.file_path(), module.build_id(),
        absl::StrJoin(base_addresses, ",", [](std::string* out, uint64_t address) {
          return out->append(absl::StrFormat("%#x", address));
        }));
  }

  CHECK(!base_addresses.empty());

  return func.address() + base_addresses.at(0) - module.load_bias() -
         module.executable_segment_offset();
}

bool IsOrbitFunctionFromType(const FunctionInfo::OrbitType& type) {
  return type != FunctionInfo::kNone;
}

bool IsOrbitFunctionFromName(const std::string& function_name) {
  return GetOrbitTypeByName(function_name) != FunctionInfo::kNone;
}

std::unique_ptr<FunctionInfo> CreateFunctionInfo(const SymbolInfo& symbol_info,
                                                 const std::string& module_path,
                                                 const std::string& module_build_id) {
  auto function_info = std::make_unique<FunctionInfo>();

  function_info->set_name(symbol_info.name());
  function_info->set_pretty_name(symbol_info.demangled_name());
  function_info->set_address(symbol_info.address());
  function_info->set_size(symbol_info.size());
  function_info->set_module_path(module_path);
  function_info->set_module_build_id(module_build_id);

  SetOrbitTypeFromName(function_info.get());
  return function_info;
}

const absl::flat_hash_map<std::string, FunctionInfo::OrbitType>& GetFunctionNameToOrbitTypeMap() {
  const char* kStubParams =
      "(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)";
  static absl::flat_hash_map<std::string, FunctionInfo::OrbitType> function_name_to_type_map{
      {absl::StrCat("orbit_api::Start", kStubParams), FunctionInfo::kOrbitTimerStart},
      {absl::StrCat("orbit_api::Stop", kStubParams), FunctionInfo::kOrbitTimerStop},
      {absl::StrCat("orbit_api::StartAsync", kStubParams), FunctionInfo::kOrbitTimerStartAsync},
      {absl::StrCat("orbit_api::StopAsync", kStubParams), FunctionInfo::kOrbitTimerStopAsync},
      {absl::StrCat("orbit_api::TrackValue", kStubParams), FunctionInfo::kOrbitTrackValue}};
  return function_name_to_type_map;
}

FunctionInfo::OrbitType GetOrbitTypeByName(const std::string& function_name) {
  if (absl::StartsWith(function_name, "orbit_api::")) {
    for (const auto& pair : GetFunctionNameToOrbitTypeMap()) {
      if (absl::StrContains(function_name, pair.first)) {
        return pair.second;
      }
    }
  }

  return FunctionInfo::kNone;
}

// Detect Orbit API functions by looking for special function names part of the
// orbit_api namespace. On a match, set the corresponding function type.
void SetOrbitTypeFromName(FunctionInfo* func) {
  func->set_orbit_type(GetOrbitTypeByName(GetDisplayName(*func)));
}

}  // namespace function_utils

}  // namespace orbit_client_data
