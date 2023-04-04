/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <boost/iterator/transform_iterator.hpp>
#include <initializer_list>
#include <ostream>

#include <json/json.h>

#include <AbstractDomain.h>
#include <PatriciaTreeMapAbstractPartition.h>

#include <mariana-trench/Access.h>
#include <mariana-trench/Assert.h>
#include <mariana-trench/FlattenIterator.h>
#include <mariana-trench/Frame.h>
#include <mariana-trench/GroupHashedSetAbstractDomain.h>
#include <mariana-trench/IncludeMacros.h>
#include <mariana-trench/TaintConfig.h>

namespace marianatrench {

/**
 * Represents a set of frames with the same call position.
 * Based on its position in `Taint`, it is expected that all frames within
 * this class have the same callee and call position.
 */
class CalleePortFrames final : public sparta::AbstractDomain<CalleePortFrames> {
 private:
  using Frames =
      GroupHashedSetAbstractDomain<Frame, Frame::GroupHash, Frame::GroupEqual>;
  using FramesByKind =
      sparta::PatriciaTreeMapAbstractPartition<const Kind*, Frames>;

 private:
  // Iterator based on `FlattenIterator`.

  struct KindToFramesMapDereference {
    static Frames::iterator begin(const std::pair<const Kind*, Frames>& pair) {
      return pair.second.begin();
    }
    static Frames::iterator end(const std::pair<const Kind*, Frames>& pair) {
      return pair.second.end();
    }
  };

  using ConstIterator = FlattenIterator<
      /* OuterIterator */ FramesByKind::MapType::iterator,
      /* InnerIterator */ Frames::iterator,
      KindToFramesMapDereference>;

 public:
  // C++ container concept member types
  using iterator = ConstIterator;
  using const_iterator = ConstIterator;
  using value_type = Frame;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using const_reference = const Frame&;
  using const_pointer = const Frame*;

 private:
  explicit CalleePortFrames(
      AccessPath callee_port,
      FramesByKind frames,
      LocalPositionSet local_positions)
      : callee_port_(std::move(callee_port)),
        frames_(std::move(frames)),
        local_positions_(std::move(local_positions)) {
    mt_assert(!local_positions_.is_bottom());
  }

 public:
  /**
   * Create the bottom (i.e, empty) frame set. Value of callee_port_ doesn't
   * matter, so we pick some default (Leaf). Also avoid using `bottom()` for
   * local_positions_ because `bottom().add(new_position)` gives `bottom()`
   * which is not the desired behavior for `CalleePortFrames::add`.
   * Consider re-visiting LocalPositionSet.
   */
  CalleePortFrames()
      : callee_port_(Root(Root::Kind::Leaf)),
        frames_(FramesByKind::bottom()),
        local_positions_({}) {}

  explicit CalleePortFrames(std::initializer_list<TaintConfig> configs);

  explicit CalleePortFrames(const Frame& frame);

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(CalleePortFrames)

  // Describe how to join frames together in `CallPositionFrames`.
  struct GroupEqual {
    bool operator()(const CalleePortFrames& left, const CalleePortFrames& right)
        const;
  };

  // Describe how to join frames together in `CallPositionFrames`.
  struct GroupHash {
    std::size_t operator()(const CalleePortFrames& frame) const;
  };

  // Describe how to diff frames together in `CallPositionFrames`.
  struct GroupDifference {
    void operator()(CalleePortFrames& left, const CalleePortFrames& right)
        const;
  };

  static CalleePortFrames bottom() {
    return CalleePortFrames();
  }

  static CalleePortFrames top() {
    return CalleePortFrames(
        /* callee_port */ AccessPath(Root(Root::Kind::Leaf)),
        FramesByKind::top(),
        /* local_positions */ {});
  }

  bool is_bottom() const override {
    return frames_.is_bottom();
  }

  bool is_top() const override {
    return frames_.is_top();
  }

  void set_to_bottom() override {
    callee_port_ = AccessPath(Root(Root::Kind::Leaf));
    frames_.set_to_bottom();
    local_positions_ = {};
  }

  void set_to_top() override {
    callee_port_ = AccessPath(Root(Root::Kind::Leaf));
    frames_.set_to_top();
    local_positions_.set_to_top();
  }

  bool empty() const {
    return frames_.is_bottom();
  }

  const AccessPath& callee_port() const {
    return callee_port_;
  }

  const LocalPositionSet& local_positions() const {
    return local_positions_;
  }

  void add(const TaintConfig& config);

  void add(const Frame& frame);

  bool leq(const CalleePortFrames& other) const override;

  bool equals(const CalleePortFrames& other) const override;

  void join_with(const CalleePortFrames& other) override;

  void widen_with(const CalleePortFrames& other) override;

  void meet_with(const CalleePortFrames& other) override;

  void narrow_with(const CalleePortFrames& other) override;

  void difference_with(const CalleePortFrames& other);

  void map(const std::function<void(Frame&)>& f);

  void filter(const std::function<bool(const Frame&)>& predicate);

  ConstIterator begin() const {
    return ConstIterator(frames_.bindings().begin(), frames_.bindings().end());
  }

  ConstIterator end() const {
    return ConstIterator(frames_.bindings().end(), frames_.bindings().end());
  }

  void set_origins_if_empty(const MethodSet& origins);

  void set_field_origins_if_empty_with_field_callee(const Field* field);

  FeatureMayAlwaysSet inferred_features() const;

  void add_inferred_features(const FeatureMayAlwaysSet& features);

  void add_local_position(const Position* position);

  void set_local_positions(LocalPositionSet positions);

  void add_inferred_features_and_local_position(
      const FeatureMayAlwaysSet& features,
      const Position* MT_NULLABLE position);

  /**
   * Appends `path_element` to the output paths of all propagation frames.
   */
  void append_to_propagation_output_paths(Path::Element path_element);

  /**
   * Propagate the taint from the callee to the caller.
   *
   * Return bottom if the taint should not be propagated.
   */
  CalleePortFrames propagate(
      const Method* callee,
      const AccessPath& callee_port,
      const Position* call_position,
      int maximum_source_sink_distance,
      Context& context,
      const std::vector<const DexType * MT_NULLABLE>& source_register_types,
      const std::vector<std::optional<std::string>>& source_constant_arguments)
      const;

  void transform_kind_with_features(
      const std::function<std::vector<const Kind*>(const Kind*)>&,
      const std::function<FeatureMayAlwaysSet(const Kind*)>&);

  void filter_invalid_frames(
      const std::function<
          bool(const Method* MT_NULLABLE, const AccessPath&, const Kind*)>&
          is_valid);

  bool contains_kind(const Kind*) const;

  template <class T>
  std::unordered_map<T, CalleePortFrames> partition_by_kind(
      const std::function<T(const Kind*)>& map_kind) const {
    std::unordered_map<T, CalleePortFrames> result;

    for (const auto& [kind, kind_frames] : frames_.bindings()) {
      T mapped_value = map_kind(kind);
      result[mapped_value].join_with(CalleePortFrames(
          callee_port_,
          FramesByKind{std::pair(kind, kind_frames)},
          local_positions_));
    }
    return result;
  }

  template <class T>
  std::unordered_map<T, std::vector<std::reference_wrapper<const Frame>>>
  partition_map(const std::function<T(const Frame&)>& map) const {
    std::unordered_map<T, std::vector<std::reference_wrapper<const Frame>>>
        result;
    for (const auto& [_, frames] : frames_.bindings()) {
      for (const auto& frame : frames) {
        auto value = map(frame);
        result[value].push_back(std::cref(frame));
      }
    }

    return result;
  }

  Json::Value to_json(
      const Method* MT_NULLABLE callee,
      const Position* MT_NULLABLE position,
      CallInfo call_info) const;

  friend std::ostream& operator<<(
      std::ostream& out,
      const CalleePortFrames& frames);

 private:
  Frame propagate_frames(
      const Method* callee,
      const AccessPath& callee_port,
      const Position* call_position,
      int maximum_source_sink_distance,
      Context& context,
      const std::vector<const DexType * MT_NULLABLE>& source_register_types,
      const std::vector<std::optional<std::string>>& source_constant_arguments,
      std::vector<std::reference_wrapper<const Frame>> frames,
      std::vector<const Feature*>& via_type_of_features_added) const;

  CalleePortFrames propagate_crtex_leaf_frames(
      const Method* callee,
      const AccessPath& callee_port,
      const Position* call_position,
      int maximum_source_sink_distance,
      Context& context,
      const std::vector<const DexType * MT_NULLABLE>& source_register_types,
      std::vector<std::reference_wrapper<const Frame>> frames) const;

  /**
   * Checks that this object and `other` have the same key. Abstract domain
   * operations here only operate on `CalleePortFrames` that have the same key.
   * The only exception is if one of them `is_bottom()`.
   */
  bool has_same_key(const CalleePortFrames& other) const {
    return callee_port_ == other.callee_port();
  }

 private:
  AccessPath callee_port_;
  FramesByKind frames_;

  // TODO(T91357916): Move local_features here from `Frame`.
  LocalPositionSet local_positions_;
};

} // namespace marianatrench
