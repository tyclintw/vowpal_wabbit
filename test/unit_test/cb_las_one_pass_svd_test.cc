// Copyright (c) by respective owners including Yahoo!, Microsoft, and
// individual contributors. All rights reserved. Released under a BSD (revised)
// license as described in the file LICENSE.

#include "reductions/cb/details/large_action_space.h"
#include "test_common.h"
#include "vw/core/qr_decomposition.h"
#include "vw/core/rand48.h"
#include "vw/core/rand_state.h"
#include "vw/core/reductions/cb/cb_explore_adf_common.h"
#include "vw/core/reductions/cb/cb_explore_adf_large_action_space.h"
#include "vw/core/vw.h"

#include <boost/test/unit_test.hpp>

using internal_action_space_op =
    VW::cb_explore_adf::cb_explore_adf_base<VW::cb_explore_adf::cb_explore_adf_large_action_space<
        VW::cb_explore_adf::one_pass_svd_impl, VW::cb_explore_adf::one_rank_spanner_state>>;

BOOST_AUTO_TEST_SUITE(test_suite_las_one_pass_svd)

BOOST_AUTO_TEST_CASE(check_AO_same_actions_same_representation)
{
  auto d = 3;
  std::vector<VW::workspace*> vws;

  auto* vw_zero_threads = VW::initialize("--cb_explore_adf --large_action_space --full_predictions --max_actions " +
          std::to_string(d) + " --quiet --random_seed 5",
      nullptr, false, nullptr, nullptr);

  vws.push_back(vw_zero_threads);

  auto* vw_2_threads = VW::initialize("--cb_explore_adf --large_action_space --full_predictions --max_actions " +
          std::to_string(d) + " --quiet --random_seed 5 --thread_pool_size 2",
      nullptr, false, nullptr, nullptr);

  vws.push_back(vw_2_threads);

  for (auto* vw_ptr : vws)
  {
    auto& vw = *vw_ptr;

    std::vector<std::string> e_r;
    vw.l->get_enabled_reductions(e_r);
    if (std::find(e_r.begin(), e_r.end(), "cb_explore_adf_large_action_space") == e_r.end())
    { BOOST_FAIL("cb_explore_adf_large_action_space not found in enabled reductions"); }

    VW::LEARNER::multi_learner* learner =
        as_multiline(vw.l->get_learner_by_name_prefix("cb_explore_adf_large_action_space"));

    auto action_space = (internal_action_space_op*)learner->get_internal_type_erased_data_pointer_test_use_only();

    BOOST_CHECK_EQUAL(action_space != nullptr, true);

    action_space->explore._populate_all_testing_components();

    {
      VW::multi_ex examples;

      examples.push_back(VW::read_example(vw, "| 1:0.1 2:0.12 3:0.13 b200:2 c500:9"));
      // duplicates start
      examples.push_back(VW::read_example(vw, "| a_1:0.5 a_2:0.65 a_3:0.12 a100:4 a200:33"));
      examples.push_back(VW::read_example(vw, "| a_1:0.5 a_2:0.65 a_3:0.12 a100:4 a200:33"));
      // duplicates end
      examples.push_back(VW::read_example(vw, "| a_4:0.8 a_5:0.32 a_6:0.15 d1:0.2 d10:0.2"));
      examples.push_back(VW::read_example(vw, "| a_7 a_8 a_9 v1:0.99"));
      examples.push_back(VW::read_example(vw, "| a_10 a_11 a_12"));
      examples.push_back(VW::read_example(vw, "| a_13 a_14 a_15"));
      examples.push_back(VW::read_example(vw, "| a_16 a_17 a_18:0.2"));

      vw.predict(examples);

      // representation of actions 2 and 3 (duplicates) should be the same in U
      BOOST_CHECK_EQUAL(action_space->explore.U.row(1).isApprox(action_space->explore.U.row(2)), true);

      vw.finish_example(examples);
    }
    VW::finish(vw);
  }
}

BOOST_AUTO_TEST_CASE(check_AO_linear_combination_of_actions)
{
  auto d = 3;
  std::vector<VW::workspace*> vws;

  auto* vw_zero_threads = VW::initialize("--cb_explore_adf --large_action_space --full_predictions --max_actions " +
          std::to_string(d) + " --quiet --random_seed 5 --noconstant",
      nullptr, false, nullptr, nullptr);

  vws.push_back(vw_zero_threads);

  auto* vw_2_threads = VW::initialize("--cb_explore_adf --large_action_space --full_predictions --max_actions " +
          std::to_string(d) + " --quiet --random_seed 5 --noconstant --thread_pool_size 2",
      nullptr, false, nullptr, nullptr);

  vws.push_back(vw_2_threads);

  for (auto* vw_ptr : vws)
  {
    auto& vw = *vw_ptr;

    std::vector<std::string> e_r;
    vw.l->get_enabled_reductions(e_r);
    if (std::find(e_r.begin(), e_r.end(), "cb_explore_adf_large_action_space") == e_r.end())
    { BOOST_FAIL("cb_explore_adf_large_action_space not found in enabled reductions"); }

    VW::LEARNER::multi_learner* learner =
        as_multiline(vw.l->get_learner_by_name_prefix("cb_explore_adf_large_action_space"));

    auto action_space = (internal_action_space_op*)learner->get_internal_type_erased_data_pointer_test_use_only();

    BOOST_CHECK_EQUAL(action_space != nullptr, true);

    action_space->explore._populate_all_testing_components();

    {
      VW::multi_ex examples;

      examples.push_back(VW::read_example(vw, "| 1:0.1 2:0.12 3:0.13 b200:2 c500:9"));

      examples.push_back(VW::read_example(vw, "| a_1:0.5 a_2:0.65 a_3:0.12 a100:4 a200:33"));
      examples.push_back(VW::read_example(vw, "| a_1:0.8 a_2:0.32 a_3:0.15 a100:0.2 a200:0.2"));
      // linear combination of the above two actions
      // action_4 = action_2 + 2 * action_3
      examples.push_back(VW::read_example(vw, "| a_1:2.1 a_2:1.29 a_3:0.42 a100:4.4 a200:33.4"));

      examples.push_back(VW::read_example(vw, "| a_4:0.8 a_5:0.32 a_6:0.15 d1:0.2 d10: 0.2"));
      examples.push_back(VW::read_example(vw, "| a_7 a_8 a_9 v1:0.99"));
      examples.push_back(VW::read_example(vw, "| a_10 a_11 a_12"));
      examples.push_back(VW::read_example(vw, "| a_13 a_14 a_15"));
      examples.push_back(VW::read_example(vw, "| a_16 a_17 a_18:0.2"));

      vw.predict(examples);

      // check that the representation of the fourth action is the same linear combination of the representation of the
      // 2nd and 3rd actions
      Eigen::VectorXf action_2 = action_space->explore.U.row(1);
      Eigen::VectorXf action_3 = action_space->explore.U.row(2);
      Eigen::VectorXf action_4 = action_space->explore.U.row(3);

      Eigen::VectorXf action_lin_rep = action_2 + 2.f * action_3;

      BOOST_CHECK_EQUAL(action_lin_rep.isApprox(action_4), true);

      vw.finish_example(examples);
    }
    VW::finish(vw);
  }
}

BOOST_AUTO_TEST_SUITE_END()