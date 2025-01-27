/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gmock/gmock.h>

#include <mariana-trench/CallPositionFrames.h>
#include <mariana-trench/Redex.h>
#include <mariana-trench/tests/Test.h>

namespace marianatrench {

class CallPositionFramesTest : public test::Test {};

TEST_F(CallPositionFramesTest, Add) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one = context.methods->create(
      redex::create_void_method(scope, "LClass;", "one"));
  auto* two = context.methods->create(
      redex::create_void_method(scope, "LOther;", "two"));

  auto* source_kind_one = context.kinds->get("TestSourceOne");
  auto* source_kind_two = context.kinds->get("TestSourceTwo");
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");
  auto* user_feature_one = context.features->get("UserFeatureOne");

  CallPositionFrames frames;
  EXPECT_TRUE(frames.is_bottom());
  EXPECT_TRUE(frames.empty());
  EXPECT_EQ(frames.position(), nullptr);

  frames.add(test::make_frame(
      source_kind_one,
      test::FrameProperties{
          .origins = MethodSet{one},
          .inferred_features = FeatureMayAlwaysSet{feature_one}}));
  EXPECT_FALSE(frames.is_bottom());
  EXPECT_EQ(frames.position(), nullptr);
  EXPECT_EQ(
      frames,
      CallPositionFrames{test::make_frame(
          source_kind_one,
          test::FrameProperties{
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one}})});

  // Add frame with the same kind
  frames.add(test::make_frame(
      source_kind_one,
      test::FrameProperties{
          .origins = MethodSet{two},
          .inferred_features = FeatureMayAlwaysSet{feature_two},
          .user_features = FeatureSet{user_feature_one}}));
  EXPECT_EQ(
      frames,
      CallPositionFrames{test::make_frame(
          source_kind_one,
          test::FrameProperties{
              .origins = MethodSet{one, two},
              .inferred_features =
                  FeatureMayAlwaysSet::make_may({feature_one, feature_two}),
              .user_features = FeatureSet{user_feature_one}})});

  // Add frame with a different kind
  frames.add(test::make_frame(
      source_kind_two,
      test::FrameProperties{
          .origins = MethodSet{two},
          .inferred_features = FeatureMayAlwaysSet{feature_two}}));
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              source_kind_one,
              test::FrameProperties{
                  .origins = MethodSet{one, two},
                  .inferred_features =
                      FeatureMayAlwaysSet::make_may({feature_one, feature_two}),
                  .user_features = FeatureSet{user_feature_one}}),
          test::make_frame(
              source_kind_two,
              test::FrameProperties{
                  .origins = MethodSet{two},
                  .inferred_features = FeatureMayAlwaysSet{feature_two}})}));

  CallPositionFrames frames_with_position;
  frames_with_position.add(test::make_frame(
      source_kind_one,
      test::FrameProperties{.call_position = context.positions->unknown()}));
  EXPECT_EQ(frames_with_position.position(), context.positions->unknown());
}

TEST_F(CallPositionFramesTest, Leq) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position = context.positions->get(std::nullopt, 1);

  // Comparison to bottom
  EXPECT_TRUE(CallPositionFrames::bottom().leq(CallPositionFrames::bottom()));
  EXPECT_TRUE(CallPositionFrames::bottom().leq(CallPositionFrames{
      test::make_frame(test_kind_one, test::FrameProperties{})}));
  EXPECT_FALSE((CallPositionFrames{test::make_frame(
                    test_kind_one,
                    test::FrameProperties{.call_position = test_position})})
                   .leq(CallPositionFrames::bottom()));

  // Comparison to self
  EXPECT_TRUE(
      (CallPositionFrames{test::make_frame(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = one,
               .call_position = test_position,
               .distance = 1,
               .origins = MethodSet{one}})})
          .leq(CallPositionFrames{test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one}})}));

  // Same kind, different port
  EXPECT_TRUE(
      (CallPositionFrames{test::make_frame(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = one,
               .call_position = test_position,
               .distance = 1,
               .origins = MethodSet{one}})})
          .leq(CallPositionFrames{
              test::make_frame(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .call_position = test_position,
                      .distance = 1,
                      .origins = MethodSet{one}}),
              test::make_frame(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
                      .callee = one,
                      .call_position = test_position,
                      .distance = 1,
                      .origins = MethodSet{one}}),
          }));
  EXPECT_FALSE(
      (CallPositionFrames{
           test::make_frame(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                   .callee = one,
                   .call_position = test_position,
                   .distance = 1,
                   .origins = MethodSet{one}}),
           test::make_frame(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
                   .callee = one,
                   .call_position = test_position,
                   .distance = 1,
                   .origins = MethodSet{one}}),
       })
          .leq(CallPositionFrames{test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one}})}));

  // Different kinds
  EXPECT_TRUE(
      (CallPositionFrames{test::make_frame(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = one,
               .call_position = test_position,
               .distance = 1,
               .origins = MethodSet{one}})})
          .leq(CallPositionFrames{
              test::make_frame(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .call_position = test_position,
                      .distance = 1,
                      .origins = MethodSet{one}}),
              test::make_frame(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .call_position = test_position,
                      .distance = 1,
                      .origins = MethodSet{one}}),
          }));
  EXPECT_FALSE(
      (CallPositionFrames{
           test::make_frame(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                   .callee = one,
                   .call_position = test_position,
                   .distance = 1,
                   .origins = MethodSet{one}}),
           test::make_frame(
               test_kind_two,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                   .callee = one,
                   .call_position = test_position,
                   .distance = 1,
                   .origins = MethodSet{one}})})
          .leq(CallPositionFrames{test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one}})}));
}

TEST_F(CallPositionFramesTest, Equals) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position = context.positions->get(std::nullopt, 1);

  // Comparison to bottom
  EXPECT_TRUE(
      CallPositionFrames::bottom().equals(CallPositionFrames::bottom()));
  EXPECT_FALSE(
      CallPositionFrames::bottom().equals(CallPositionFrames{test::make_frame(
          test_kind_one,
          test::FrameProperties{.call_position = test_position})}));
  EXPECT_FALSE((CallPositionFrames{test::make_frame(
                    test_kind_one,
                    test::FrameProperties{.call_position = test_position})})
                   .equals(CallPositionFrames::bottom()));

  // Comparison to self
  EXPECT_TRUE((CallPositionFrames{
                   test::make_frame(test_kind_one, test::FrameProperties{})})
                  .equals(CallPositionFrames{test::make_frame(
                      test_kind_one, test::FrameProperties{})}));

  // Different ports
  EXPECT_FALSE(
      (CallPositionFrames{test::make_frame(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = one,
               .call_position = test_position,
               .distance = 1,
               .origins = MethodSet{one}})})
          .equals(CallPositionFrames{test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one}})}));

  // Different kinds
  EXPECT_FALSE((CallPositionFrames{
                    test::make_frame(test_kind_one, test::FrameProperties{})})
                   .equals(CallPositionFrames{test::make_frame(
                       test_kind_two, test::FrameProperties{})}));
}

TEST_F(CallPositionFramesTest, JoinWith) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position = context.positions->get(std::nullopt, 1);

  // Join with bottom
  EXPECT_EQ(
      CallPositionFrames::bottom().join(CallPositionFrames{
          test::make_frame(test_kind_one, test::FrameProperties{})}),
      CallPositionFrames{
          test::make_frame(test_kind_one, test::FrameProperties{})});
  EXPECT_EQ(
      (CallPositionFrames{test::make_frame(
           test_kind_one,
           test::FrameProperties{.call_position = test_position})})
          .join(CallPositionFrames::bottom()),
      CallPositionFrames{test::make_frame(
          test_kind_one,
          test::FrameProperties{.call_position = test_position})});

  // Join different kinds
  auto frames = CallPositionFrames{
      test::make_frame(test_kind_one, test::FrameProperties{})};
  frames.join_with(CallPositionFrames{
      test::make_frame(test_kind_two, test::FrameProperties{})});
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(test_kind_one, test::FrameProperties{}),
          test::make_frame(test_kind_two, test::FrameProperties{})}));

  // Join same kind
  auto frame_one = test::make_frame(
      test_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
          .callee = one,
          .call_position = test_position,
          .distance = 1,
          .origins = MethodSet{one}});
  auto frame_two = test::make_frame(
      test_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
          .callee = one,
          .call_position = test_position,
          .distance = 2,
          .origins = MethodSet{one}});
  frames = CallPositionFrames{frame_one};
  frames.join_with(CallPositionFrames{frame_two});
  EXPECT_EQ(frames, (CallPositionFrames{frame_one}));
}

TEST_F(CallPositionFramesTest, Difference) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* two =
      context.methods->create(redex::create_void_method(scope, "LTwo;", "two"));
  auto* three = context.methods->create(
      redex::create_void_method(scope, "LThree;", "three"));

  const auto* x = DexString::make_string("x");
  const auto* y = DexString::make_string("y");

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position = context.positions->get(std::nullopt, 1);
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");
  auto* user_feature_one = context.features->get("UserFeatureOne");
  auto* user_feature_two = context.features->get("UserFeatureTwo");

  CallPositionFrames frames, initial_frames;

  // Tests with empty left hand side.
  frames.difference_with(CallPositionFrames{});
  EXPECT_TRUE(frames.is_bottom());

  frames.difference_with(CallPositionFrames{
      test::make_frame(test_kind_one, test::FrameProperties{}),
  });
  EXPECT_TRUE(frames.is_bottom());

  initial_frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_one}}),
  };

  frames = initial_frames;
  frames.difference_with(CallPositionFrames{});
  EXPECT_EQ(frames, initial_frames);

  frames = initial_frames;
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_one}}),
  });
  EXPECT_TRUE(frames.is_bottom());

  // Left hand side is bigger than right hand side.
  frames = initial_frames;
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
  });
  EXPECT_EQ(frames, initial_frames);

  // Left hand side and right hand side have different inferred features.
  frames = initial_frames;
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_two},
              .user_features = FeatureSet{user_feature_one}}),
  });
  EXPECT_EQ(frames, initial_frames);

  // Left hand side and right hand side have different user features.
  frames = initial_frames;
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_two}}),
  });
  EXPECT_EQ(frames, initial_frames);

  // Left hand side and right hand side have different callee_ports.
  frames = initial_frames;
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_one}}),
  });
  EXPECT_EQ(frames, initial_frames);

  // Left hand side is smaller than right hand side (with one kind).
  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_one}}),
  };
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one},
              .inferred_features = FeatureMayAlwaysSet{feature_one},
              .user_features = FeatureSet{user_feature_one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two},
              .inferred_features = FeatureMayAlwaysSet{feature_two},
              .user_features = FeatureSet{user_feature_two}}),
  });
  EXPECT_TRUE(frames.is_bottom());

  // Left hand side has more kinds than right hand side.
  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
  };
  frames.difference_with(CallPositionFrames{test::make_frame(
      test_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
          .callee = one,
          .call_position = test_position,
          .distance = 1,
          .origins = MethodSet{one}})});
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one}}),
      }));

  // Left hand side is smaller for one kind, and larger for another.
  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = three,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{three}}),
  };
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}})});
  EXPECT_EQ(
      frames,
      (CallPositionFrames{test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = three,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{three}})}));

  // Both sides contain access paths
  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0), Path{x}),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0), Path{y}),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}}),
  };
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0), Path{x}),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0), Path{y}),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = three,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{three}}),
  });
  EXPECT_TRUE(frames.is_bottom());

  // Left hand side larger than right hand side for specific frames.
  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one, two}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = two,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{two}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = three,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one, three}}),
  };
  frames.difference_with(CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = three,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one, two, three}}),
  });
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one, two}}),
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{two}}),
      }));
}

TEST_F(CallPositionFramesTest, Iterator) {
  auto context = test::make_empty_context();

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");

  auto call_position_frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0))}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 1))}),
      test::make_frame(test_kind_two, test::FrameProperties{})};

  std::vector<Frame> frames;
  for (const auto& frame : call_position_frames) {
    frames.push_back(frame);
  }

  EXPECT_EQ(frames.size(), 3);
  EXPECT_NE(
      std::find(
          frames.begin(),
          frames.end(),
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0))})),
      frames.end());
  EXPECT_NE(
      std::find(
          frames.begin(),
          frames.end(),
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 1))})),
      frames.end());
  EXPECT_NE(
      std::find(
          frames.begin(),
          frames.end(),
          test::make_frame(test_kind_two, test::FrameProperties{})),
      frames.end());
}

TEST_F(CallPositionFramesTest, Map) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* test_kind = context.kinds->get("TestSink");
  auto* test_position = context.positions->get(std::nullopt, 1);
  auto* feature_one = context.features->get("FeatureOne");

  auto frames = CallPositionFrames{
      test::make_frame(
          test_kind,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .call_position = test_position,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
              .callee = one,
              .call_position = test_position,
              .distance = 2,
              .origins = MethodSet{one}}),
  };
  frames.map([feature_one](Frame& frame) {
    frame.add_inferred_features(FeatureMayAlwaysSet{feature_one});
  });
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .locally_inferred_features =
                      FeatureMayAlwaysSet{feature_one}}),
          test::make_frame(
              test_kind,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 1)),
                  .callee = one,
                  .call_position = test_position,
                  .distance = 2,
                  .origins = MethodSet{one},
                  .locally_inferred_features =
                      FeatureMayAlwaysSet{feature_one}}),
      }));
}

TEST_F(CallPositionFramesTest, FeaturesAndPositions) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position_one = context.positions->get(std::nullopt, 1);
  auto* test_position_two = context.positions->get(std::nullopt, 2);
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");

  // add_inferred_features should be an *add* operation on the features,
  // not a join.
  auto frames = CallPositionFrames{test::make_frame(
      test_kind_one,
      test::FrameProperties{
          .locally_inferred_features = FeatureMayAlwaysSet(
              /* may */ FeatureSet{feature_one},
              /* always */ FeatureSet{})})};
  frames.add_inferred_features(FeatureMayAlwaysSet{feature_two});
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .locally_inferred_features = FeatureMayAlwaysSet(
                      /* may */ FeatureSet{feature_one},
                      /* always */ FeatureSet{feature_two})}),
      }));

  frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .local_positions = LocalPositionSet{test_position_one}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .local_positions = LocalPositionSet{test_position_two}}),
  };
  EXPECT_EQ(
      frames.local_positions(),
      (LocalPositionSet{test_position_one, test_position_two}));

  frames.add_local_position(test_position_one);
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .local_positions = LocalPositionSet{test_position_one}}),
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .local_positions =
                      LocalPositionSet{test_position_one, test_position_two}}),
      }));

  frames.set_local_positions(LocalPositionSet{test_position_two});
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .local_positions = LocalPositionSet{test_position_two}}),
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .local_positions = LocalPositionSet{test_position_two}}),
      }));

  frames.add_inferred_features_and_local_position(
      /* features */ FeatureMayAlwaysSet{feature_one},
      /* position */ test_position_one);
  EXPECT_EQ(
      frames,
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .locally_inferred_features = FeatureMayAlwaysSet{feature_one},
                  .local_positions =
                      LocalPositionSet{test_position_one, test_position_two}}),
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .locally_inferred_features = FeatureMayAlwaysSet{feature_one},
                  .local_positions =
                      LocalPositionSet{test_position_one, test_position_two}}),
      }));
}

TEST_F(CallPositionFramesTest, Propagate) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* two =
      context.methods->create(redex::create_void_method(scope, "LTwo;", "two"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* call_position = context.positions->get("Test.java", 1);

  // It is generally expected (though not enforced) that frames within
  // `CallPositionFrames` have the same callee because of the `Taint`
  // structure. They typically also share the same "callee_port" because they
  // share the same `Position`. However, for testing purposes, we use
  // different callees and callee ports.
  auto frames = CallPositionFrames{
      test::make_frame(
          test_kind_one,
          test::FrameProperties{.callee = two, .origins = MethodSet{two}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .distance = 1,
              .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Anchor)),
              .origins = MethodSet{one},
              .canonical_names = CanonicalNameSetAbstractDomain{CanonicalName(
                  CanonicalName::TemplateValue{"%programmatic_leaf_name%"})}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{.callee = one, .origins = MethodSet{one}}),
      test::make_frame(
          test_kind_two,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Anchor)),
              .origins = MethodSet{one},
              .canonical_names = CanonicalNameSetAbstractDomain{CanonicalName(
                  CanonicalName::TemplateValue{"%programmatic_leaf_name%"})}}),
  };

  auto expected_instantiated_name =
      CanonicalName(CanonicalName::InstantiatedValue{two->signature()});
  EXPECT_EQ(
      frames.propagate(
          /* callee */ two,
          /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
          call_position,
          /* maximum_source_sink_distance */ 100,
          context,
          /* source_register_types */ {},
          /* source_constant_arguments */ {}),
      (CallPositionFrames{
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .call_position = call_position,
                  .distance = 1,
                  .origins = MethodSet{one, two},
                  .locally_inferred_features = FeatureMayAlwaysSet::bottom()}),
          test::make_frame(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(
                      Root(Root::Kind::Anchor),
                      Path{DexString::make_string("Argument(-1)")}),
                  .callee = two,
                  .call_position = call_position,
                  .origins = MethodSet{one},
                  .locally_inferred_features = FeatureMayAlwaysSet::bottom(),
                  .canonical_names =
                      CanonicalNameSetAbstractDomain{
                          expected_instantiated_name}}),
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .call_position = call_position,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .locally_inferred_features = FeatureMayAlwaysSet::bottom()}),
          test::make_frame(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(
                      Root(Root::Kind::Anchor),
                      Path{DexString::make_string("Argument(-1)")}),
                  .callee = two,
                  .call_position = call_position,
                  .origins = MethodSet{one},
                  .locally_inferred_features = FeatureMayAlwaysSet::bottom(),
                  .canonical_names =
                      CanonicalNameSetAbstractDomain{
                          expected_instantiated_name}}),
      }));
}

} // namespace marianatrench
