/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gmock/gmock.h>

#include <mariana-trench/Fields.h>
#include <mariana-trench/Frame.h>
#include <mariana-trench/Redex.h>
#include <mariana-trench/tests/Test.h>

namespace marianatrench {

class FrameTest : public test::Test {};

TEST_F(FrameTest, FrameConstructor) {
  EXPECT_TRUE(Frame().is_bottom());
  EXPECT_TRUE(Frame::bottom().is_bottom());
}

TEST_F(FrameTest, FrameLeq) {
  Scope scope;
  auto dex_fields = redex::create_fields(
      scope,
      /* class_name */ "LClassThree;",
      /* fields */
      {{"field1", type::java_lang_Boolean()},
       {"field2", type::java_lang_String()}});
  DexStore store("stores");
  store.add_classes(scope);
  auto context = test::make_context(store);

  auto* one = context.methods->create(
      redex::create_void_method(scope, "LClass;", "one"));
  auto* two = context.methods->create(
      redex::create_void_method(scope, "LOther;", "two"));

  auto* leaf =
      context.access_path_factory->get(AccessPath(Root(Root::Kind::Leaf)));
  auto* one_origin = context.origin_factory->method_origin(one, leaf);
  auto* two_origin = context.origin_factory->method_origin(two, leaf);

  auto* field_one = context.fields->get(dex_fields[0]);
  auto* field_two = context.fields->get(dex_fields[1]);

  EXPECT_TRUE(Frame::bottom().leq(Frame::bottom()));
  EXPECT_TRUE(Frame::bottom().leq(test::make_taint_frame(
      /* kind */ context.kind_factory->get("TestSource"),
      test::FrameProperties{})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{})
                   .leq(Frame::bottom()));

  // Compare kind.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{})
                   .leq(test::make_taint_frame(
                       /* kind */ context.kind_factory->get("TestSink"),
                       test::FrameProperties{})));

  // Compare distances.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{.distance = 1})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{.distance = 0})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{.distance = 0})
                   .leq(test::make_taint_frame(
                       /* kind */ context.kind_factory->get("TestSource"),
                       test::FrameProperties{.distance = 1})));

  // Compare origins.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{.origins = OriginSet{one_origin}})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .origins = OriginSet{one_origin, two_origin}})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{.origins = OriginSet{one_origin, two_origin}})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{.origins = OriginSet{one_origin}})));

  // Compare field origins.
  EXPECT_TRUE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .origins =
                  OriginSet{context.origin_factory->field_origin(field_one)}})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .origins = OriginSet{
                      context.origin_factory->field_origin(field_one),
                      context.origin_factory->field_origin(field_two)}})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .origins =
                  OriginSet{
                      context.origin_factory->field_origin(field_one),
                      context.origin_factory->field_origin(field_two)}})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .origins = OriginSet{
                      context.origin_factory->field_origin(field_one)}})));

  // Compare inferred features.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{
                      .inferred_features = FeatureMayAlwaysSet::make_may(
                          {context.feature_factory->get("FeatureOne")})})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .inferred_features = FeatureMayAlwaysSet::make_may(
                              {context.feature_factory->get("FeatureOne"),
                               context.feature_factory->get("FeatureTwo")})})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .inferred_features = FeatureMayAlwaysSet::make_may(
                  {context.feature_factory->get("FeatureOne"),
                   context.feature_factory->get("FeatureTwo")})})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .inferred_features = FeatureMayAlwaysSet::make_may(
                      {context.feature_factory->get("FeatureOne")})})));

  // Compare user features.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{
                      .user_features = FeatureSet{context.feature_factory->get(
                          "FeatureOne")}})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .locally_inferred_features = {},
                          .user_features = FeatureSet{
                              context.feature_factory->get("FeatureOne"),
                              context.feature_factory->get("FeatureTwo")}})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{
                       .user_features =
                           FeatureSet{
                               context.feature_factory->get("FeatureOne"),
                               context.feature_factory->get("FeatureTwo")}})
                   .leq(test::make_taint_frame(
                       /* kind */ context.kind_factory->get("TestSource"),
                       test::FrameProperties{
                           .user_features = FeatureSet{
                               context.feature_factory->get("FeatureOne")}})));

  // Compare via_type_of_ports
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{
                      .via_type_of_ports =
                          RootSetAbstractDomain({Root(Root::Kind::Return)})})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .via_type_of_ports = RootSetAbstractDomain(
                              {Root(Root::Kind::Return),
                               Root(Root::Kind::Argument, 1)})})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{
                       .via_type_of_ports =
                           RootSetAbstractDomain({Root(Root::Kind::Return)})})
                   .leq(test::make_taint_frame(
                       /* kind */ context.kind_factory->get("TestSource"),
                       test::FrameProperties{
                           .via_type_of_ports = RootSetAbstractDomain(
                               {Root(Root::Kind::Argument, 1)})})));

  // callee_port, callee and call_position must be equal.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Return)),
                      .callee = one,
                      .call_position = context.positions->unknown(),
                      .distance = 1,
                      .origins = OriginSet{one_origin},
                      .call_kind = CallKind::callsite()})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .callee_port = AccessPath(Root(Root::Kind::Return)),
                          .callee = one,
                          .call_position = context.positions->unknown(),
                          .distance = 1,
                          .origins = OriginSet{one_origin},
                          .call_kind = CallKind::callsite()})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins = OriginSet{one_origin},
              .call_kind = CallKind::callsite()})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 1,
                  .origins = OriginSet{one_origin},
                  .call_kind = CallKind::callsite()})));
  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{
                       .callee_port = AccessPath(Root(Root::Kind::Return)),
                       .callee = one,
                       .call_position = context.positions->unknown(),
                       .distance = 1,
                       .origins = OriginSet{one_origin},
                       .call_kind = CallKind::callsite()})
                   .leq(test::make_taint_frame(
                       /* kind */ context.kind_factory->get("TestSource"),
                       test::FrameProperties{
                           .callee_port = AccessPath(Root(Root::Kind::Return)),
                           .callee = two,
                           .call_position = context.positions->unknown(),
                           .distance = 1,
                           .origins = OriginSet{one_origin},
                           .call_kind = CallKind::callsite()})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins = OriginSet{one_origin},
              .call_kind = CallKind::callsite()})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->get("Test.java", 1),
                  .distance = 1,
                  .origins = OriginSet{one_origin},
                  .call_kind = CallKind::callsite()})));

  // Compare canonical names.
  EXPECT_TRUE(test::make_taint_frame(
                  /* kind */ context.kind_factory->get("TestSource"),
                  test::FrameProperties{})
                  .leq(test::make_taint_frame(
                      /* kind */ context.kind_factory->get("TestSource"),
                      test::FrameProperties{
                          .canonical_names = CanonicalNameSetAbstractDomain{
                              CanonicalName(CanonicalName::TemplateValue{
                                  "%programmatic_leaf_name%"})}})));
  EXPECT_FALSE(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .canonical_names = CanonicalNameSetAbstractDomain{CanonicalName(
                  CanonicalName::TemplateValue{"%programmatic_leaf_name%"})}})
          .leq(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{})));

  // Compare output paths.
  auto x = PathElement::field("x");
  EXPECT_TRUE(
      test::make_taint_frame(
          context.kind_factory->local_return(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .output_paths = PathTreeDomain{{Path{x}, CollapseDepth::zero()}},
              .call_kind = CallKind::propagation(),
          })
          .leq(test::make_taint_frame(
              context.kind_factory->local_return(),
              test::FrameProperties{test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .output_paths =
                      PathTreeDomain{{Path{}, CollapseDepth::zero()}},
                  .call_kind = CallKind::propagation(),
              }})));
  EXPECT_FALSE(
      test::make_taint_frame(
          context.kind_factory->local_return(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .output_paths = PathTreeDomain{{Path{}, CollapseDepth::zero()}},
              .call_kind = CallKind::propagation(),
          })
          .leq(test::make_taint_frame(
              context.kind_factory->local_return(),
              test::FrameProperties{test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .output_paths =
                      PathTreeDomain{{Path{x}, CollapseDepth::zero()}},
                  .call_kind = CallKind::propagation(),
              }})));
}

TEST_F(FrameTest, FrameEquals) {
  auto context = test::make_empty_context();

  EXPECT_TRUE(Frame::bottom().equals(Frame::bottom()));
  EXPECT_FALSE(Frame::bottom().equals(test::make_taint_frame(
      /* kind */ context.kind_factory->get("TestSource"),
      test::FrameProperties{})));

  EXPECT_FALSE(test::make_taint_frame(
                   /* kind */ context.kind_factory->get("TestSource"),
                   test::FrameProperties{})
                   .equals(Frame::bottom()));
}

TEST_F(FrameTest, FrameJoin) {
  Scope scope;
  auto dex_fields = redex::create_fields(
      scope,
      /* class_name */ "LClassThree;",
      /* fields */
      {{"field1", type::java_lang_Boolean()},
       {"field2", type::java_lang_String()}});
  DexStore store("stores");
  store.add_classes(scope);
  auto context = test::make_context(store);

  auto* one = context.methods->create(
      redex::create_void_method(scope, "LClass;", "one"));
  auto* two = context.methods->create(
      redex::create_void_method(scope, "LOther;", "two"));
  auto* field_one = context.fields->get(dex_fields[0]);
  auto* field_two = context.fields->get(dex_fields[1]);
  auto* leaf =
      context.access_path_factory->get(AccessPath(Root(Root::Kind::Leaf)));
  auto* one_origin = context.origin_factory->method_origin(one, leaf);
  auto* two_origin = context.origin_factory->method_origin(two, leaf);

  EXPECT_EQ(Frame::bottom().join(Frame::bottom()), Frame::bottom());
  EXPECT_EQ(
      Frame::bottom().join(test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{}));
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{})
          .join(Frame::bottom()),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{}));

  // Test incompatible joins.
  EXPECT_THROW(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{})
          .join_with(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSink"),
              test::FrameProperties{})),
      std::exception);
  EXPECT_THROW(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Leaf))})
          .join_with(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Anchor))})),
      std::exception);
  EXPECT_THROW(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .call_kind = CallKind::callsite(),
          })
          .join_with(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = two,
                  .call_position = context.positions->unknown(),
                  .distance = 1,
                  .call_kind = CallKind::callsite(),
              })),
      std::exception);
  EXPECT_THROW(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .call_kind = CallKind::callsite(),
          })
          .join_with(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->get("Test.java", 1),
                  .distance = 1,
                  .call_kind = CallKind::callsite(),
              })),
      std::exception);

  // Minimum distance.
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .call_kind = CallKind::callsite(),
          })
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 1,
                  .call_kind = CallKind::callsite(),
              })),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .call_kind = CallKind::callsite(),
          }));

  // Join origins.
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins = OriginSet{one_origin},
              .call_kind = CallKind::callsite()})
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 1,
                  .origins = OriginSet{two_origin},
                  .call_kind = CallKind::callsite()})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins = OriginSet{one_origin, two_origin},
              .call_kind = CallKind::callsite()}));

  // Join field origins
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins =
                  OriginSet{context.origin_factory->field_origin(field_one)},
              .call_kind = CallKind::callsite()})
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 1,
                  .origins = OriginSet{context.origin_factory->field_origin(
                      field_two)},
                  .call_kind = CallKind::callsite()})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 1,
              .origins =
                  OriginSet{
                      context.origin_factory->field_origin(field_one),
                      context.origin_factory->field_origin(field_two)},
              .call_kind = CallKind::callsite()}));

  // Join inferred features.
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .inferred_features =
                  FeatureMayAlwaysSet{
                      context.feature_factory->get("FeatureOne")},
              .call_kind = CallKind::callsite()})
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 2,
                  .inferred_features =
                      FeatureMayAlwaysSet{
                          context.feature_factory->get("FeatureTwo")},
                  .call_kind = CallKind::callsite()})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .inferred_features = FeatureMayAlwaysSet::make_may(
                  {context.feature_factory->get("FeatureOne"),
                   context.feature_factory->get("FeatureTwo")}),
              .call_kind = CallKind::callsite(),
          }));

  // Join user features.
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .user_features =
                  FeatureSet{context.feature_factory->get("FeatureOne")},
              .call_kind = CallKind::callsite()})
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 2,
                  .user_features =
                      FeatureSet{context.feature_factory->get("FeatureTwo")},
                  .call_kind = CallKind::callsite()})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .user_features =
                  FeatureSet{
                      context.feature_factory->get("FeatureOne"),
                      context.feature_factory->get("FeatureTwo")},
              .call_kind = CallKind::callsite()}));

  // Join via_type_of_ports
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .via_type_of_ports =
                  RootSetAbstractDomain({Root(Root::Kind::Return)}),
              .call_kind = CallKind::callsite(),
          })
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .callee = one,
                  .call_position = context.positions->unknown(),
                  .distance = 2,
                  .via_type_of_ports =
                      RootSetAbstractDomain({Root(Root::Kind::Argument, 1)}),
                  .call_kind = CallKind::callsite(),
              })),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .callee = one,
              .call_position = context.positions->unknown(),
              .distance = 2,
              .via_type_of_ports = RootSetAbstractDomain(
                  {Root(Root::Kind::Return), Root(Root::Kind::Argument, 1)}),
              .call_kind = CallKind::callsite(),
          }));

  // Join canonical names.
  EXPECT_EQ(
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .call_position = context.positions->unknown(),
              .canonical_names = CanonicalNameSetAbstractDomain{CanonicalName(
                  CanonicalName::TemplateValue{"%programmatic_leaf_name%"})}})
          .join(test::make_taint_frame(
              /* kind */ context.kind_factory->get("TestSource"),
              test::FrameProperties{
                  .call_position = context.positions->unknown(),
                  .canonical_names =
                      CanonicalNameSetAbstractDomain{CanonicalName(
                          CanonicalName::TemplateValue{"%via_type_of%"})}})),
      test::make_taint_frame(
          /* kind */ context.kind_factory->get("TestSource"),
          test::FrameProperties{
              .call_position = context.positions->unknown(),
              .canonical_names = CanonicalNameSetAbstractDomain{
                  CanonicalName(
                      CanonicalName::TemplateValue{"%programmatic_leaf_name%"}),
                  CanonicalName(
                      CanonicalName::TemplateValue{"%via_type_of%"})}}));

  // Join output paths.
  auto x = PathElement::field("x");
  auto y = PathElement::field("y");
  EXPECT_EQ(
      test::make_taint_frame(
          context.kind_factory->local_return(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .output_paths = PathTreeDomain{{Path{x}, CollapseDepth::zero()}},
              .call_kind = CallKind::propagation(),
          })
          .join(test::make_taint_frame(
              context.kind_factory->local_return(),
              test::FrameProperties{test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .output_paths =
                      PathTreeDomain{{Path{y}, CollapseDepth::zero()}},
                  .call_kind = CallKind::propagation(),
              }})),
      test::make_taint_frame(
          context.kind_factory->local_return(),
          test::FrameProperties{test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return)),
              .output_paths =
                  PathTreeDomain{
                      {Path{x}, CollapseDepth::zero()},
                      {Path{y}, CollapseDepth::zero()},
                  },
              .call_kind = CallKind::propagation(),
          }})

  );
}

TEST_F(FrameTest, FrameWithKind) {
  Scope scope;
  auto dex_fields = redex::create_field(
      scope,
      /* class_name */ "LClassThree;",
      /* fields */
      {"field1", type::java_lang_Boolean()});
  DexStore store("stores");
  store.add_classes(scope);
  auto context = test::make_context(store);

  auto* one = context.methods->create(
      redex::create_void_method(scope, "LClass;", "one"));
  auto* two = context.methods->create(
      redex::create_void_method(scope, "LOther;", "two"));

  auto* field = context.fields->get(dex_fields);
  auto kind_a = context.kind_factory->get("TestSourceA");
  auto kind_b = context.kind_factory->get("TestSourceB");

  auto* leaf =
      context.access_path_factory->get(AccessPath(Root(Root::Kind::Leaf)));
  auto* two_origin = context.origin_factory->method_origin(two, leaf);
  auto* field_origin = context.origin_factory->field_origin(field);

  auto frame1 = test::make_taint_frame(
      /* kind */ kind_a,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Leaf)),
          .callee = one,
          .call_position = context.positions->unknown(),
          .distance = 5,
          .origins = OriginSet{two_origin, field_origin},
          .inferred_features = FeatureMayAlwaysSet::make_may(
              {context.feature_factory->get("FeatureOne"),
               context.feature_factory->get("FeatureTwo")}),
          .call_kind = CallKind::callsite(),
      });

  auto frame2 = frame1.with_kind(kind_b);
  EXPECT_EQ(frame1.callee(), frame2.callee());
  EXPECT_EQ(frame1.callee_port(), frame2.callee_port());
  EXPECT_EQ(frame1.call_position(), frame2.call_position());
  EXPECT_EQ(frame1.distance(), frame2.distance());
  EXPECT_EQ(frame1.origins(), frame2.origins());
  EXPECT_EQ(frame1.inferred_features(), frame2.inferred_features());

  EXPECT_NE(frame1.kind(), frame2.kind());
  EXPECT_EQ(frame1.kind(), kind_a);
  EXPECT_EQ(frame2.kind(), kind_b);
}

} // namespace marianatrench
