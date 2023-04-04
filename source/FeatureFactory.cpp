/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <mariana-trench/FeatureFactory.h>

namespace marianatrench {

const Feature* FeatureFactory::get(const std::string& data) const {
  return factory_.create(data);
}

const Feature* FeatureFactory::get_via_type_of_feature(
    const DexType* MT_NULLABLE type) const {
  const auto& type_string = type ? type->str() : "unknown";
  return factory_.create("via-type:" + type_string);
}

const Feature* FeatureFactory::get_via_cast_feature(
    const DexType* MT_NULLABLE type) const {
  const auto& type_string = type ? type->str() : "unknown";
  return factory_.create("via-cast:" + type_string);
}

const Feature* FeatureFactory::get_via_value_of_feature(
    const std::optional<std::string_view>& value) const {
  const auto& via_value = value.value_or("unknown");
  return factory_.create("via-value:" + via_value);
}

const Feature* FeatureFactory::get_via_shim_feature(
    const Method* MT_NULLABLE method) const {
  const auto& method_string = method ? method->signature() : "unknown";
  return factory_.create("via-shim:" + method_string);
}

const Feature* FeatureFactory::get_issue_broadening_feature() const {
  return factory_.create("via-issue-broadening");
}

const Feature* FeatureFactory::get_propagation_broadening_feature() const {
  return factory_.create("via-propagation-broadening");
}

const Feature* FeatureFactory::get_widen_broadening_feature() const {
  return factory_.create("via-widen-broadening");
}

const FeatureFactory& FeatureFactory::singleton() {
  // Thread-safe global variable, initialized on first call.
  static FeatureFactory instance;
  return instance;
}

} // namespace marianatrench
