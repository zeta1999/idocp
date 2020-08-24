#include <vector>
#include <string>
#include <random>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"
#include "pinocchio/multibody/model.hpp"
#include "pinocchio/multibody/data.hpp"
#include "pinocchio/parsers/urdf.hpp"
#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/crba.hpp"
#include "pinocchio/algorithm/rnea.hpp"
#include "pinocchio/algorithm/rnea-derivatives.hpp"

#include "idocp/robot/point_contact.hpp"
#include "idocp/robot/robot.hpp"


namespace idocp {

class FixedBaseRobotTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    urdf_ = "../../urdf/iiwa14/iiwa14.urdf";
    pinocchio::urdf::buildModel(urdf_, model_);
    data_ = pinocchio::Data(model_);
    dimq_ = model_.nq;
    contact_frame_id_ = 18;
    q_ = pinocchio::randomConfiguration(model_, -Eigen::VectorXd::Ones(dimq_), 
                                        Eigen::VectorXd::Ones(dimq_));
    v_ = Eigen::VectorXd::Random(dimq_);
    a_ = Eigen::VectorXd::Random(dimq_);
    baumgarte_weight_on_velocity_ = std::abs(Eigen::VectorXd::Random(2)[0]);
    baumgarte_weight_on_position_ = std::abs(Eigen::VectorXd::Random(2)[0]);
  }

  virtual void TearDown() {
  }

  std::string urdf_;
  pinocchio::Model model_;
  pinocchio::Data data_;
  int dimq_, contact_frame_id_;
  Eigen::VectorXd q_, v_, a_;
  double baumgarte_weight_on_velocity_, baumgarte_weight_on_position_;
};


TEST_F(FixedBaseRobotTest, constructor) {
  // Default constructor
  Robot robot_empty;
  EXPECT_EQ(robot_empty.dimq(), 0);
  EXPECT_EQ(robot_empty.dimv(), 0);
  EXPECT_EQ(robot_empty.max_dimf(), 0);
  EXPECT_EQ(robot_empty.dimf(), 0);
  EXPECT_EQ(robot_empty.dim_passive(), 0);
  EXPECT_EQ(robot_empty.max_point_contacts(), 0);
  EXPECT_EQ(robot_empty.num_active_point_contacts(), 0);
  EXPECT_FALSE(robot_empty.has_active_contacts());
  EXPECT_FALSE(robot_empty.has_floating_base());
  Robot robot(urdf_);
  EXPECT_EQ(robot.dimq(), dimq_);
  EXPECT_EQ(robot.dimv(), dimq_);
  EXPECT_EQ(robot.dimf(), 0);
  EXPECT_EQ(robot.max_dimf(), 0);
  EXPECT_EQ(robot.dim_passive(), 0);
  EXPECT_EQ(robot.max_point_contacts(), 0);
  EXPECT_EQ(robot.num_active_point_contacts(), 0);
  EXPECT_FALSE(robot.has_active_contacts());
  EXPECT_FALSE(robot.has_floating_base());
  robot.printRobotModel();
  std::vector<int> contacts = {contact_frame_id_};
  Robot robot_contact(urdf_, contacts, baumgarte_weight_on_velocity_, 
                      baumgarte_weight_on_position_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimq_);
  EXPECT_EQ(robot_contact.dimf(), 0);
  EXPECT_EQ(robot_contact.max_dimf(), 3);
  EXPECT_EQ(robot_contact.dim_passive(), 0);
  EXPECT_EQ(robot_contact.max_point_contacts(), 1);
  EXPECT_EQ(robot_contact.num_active_point_contacts(), 0);
  EXPECT_EQ(robot_contact.is_contact_active(0), false);
  EXPECT_FALSE(robot_contact.has_active_contacts());
  EXPECT_FALSE(robot_contact.has_floating_base());
  robot_contact.printRobotModel();
  Eigen::VectorXd effort_limit(dimq_), velocity_limit(dimq_), 
                  lower_position_limit(dimq_), upper_position_limit(dimq_);
  effort_limit << 300, 300, 300, 300, 300, 300, 300;
  velocity_limit << 10, 10, 10, 10, 10, 10, 10;
  lower_position_limit << -2.96705972839, -2.09439510239, -2.96705972839, 
                          -2.09439510239, -2.96705972839, -2.09439510239, 
                          -3.05432619099;
  upper_position_limit << 2.96705972839, 2.09439510239, 2.96705972839, 
                          2.09439510239, 2.96705972839, 2.09439510239, 
                          3.05432619099;
  EXPECT_TRUE(robot.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(robot.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(robot.upperJointPositionLimit().isApprox(upper_position_limit));
  EXPECT_TRUE(robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot_contact.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(upper_position_limit));
  effort_limit.setZero();
  velocity_limit.setZero();
  lower_position_limit.setZero();
  upper_position_limit.setZero();
  robot.setJointEffortLimit(effort_limit);
  robot.setJointVelocityLimit(velocity_limit);
  robot.setLowerJointPositionLimit(lower_position_limit);
  robot.setUpperJointPositionLimit(upper_position_limit);
  EXPECT_TRUE(robot.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(robot.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(robot.upperJointPositionLimit().isApprox(upper_position_limit));
}


TEST_F(FixedBaseRobotTest, moveAssign) {
  // Default constructor
  Robot robot_empty;
  EXPECT_EQ(robot_empty.dimq(), 0);
  EXPECT_EQ(robot_empty.dimv(), 0);
  EXPECT_EQ(robot_empty.dimf(), 0);
  EXPECT_EQ(robot_empty.max_dimf(), 0);
  EXPECT_EQ(robot_empty.dim_passive(), 0);
  EXPECT_EQ(robot_empty.max_point_contacts(), 0);
  EXPECT_FALSE(robot_empty.has_floating_base());
  std::vector<int> contacts = {contact_frame_id_};
  Robot robot_contact(urdf_, contacts, baumgarte_weight_on_velocity_, 
                      baumgarte_weight_on_position_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimq_);
  EXPECT_EQ(robot_contact.dimf(), 0);
  EXPECT_EQ(robot_contact.max_dimf(), 3);
  EXPECT_EQ(robot_contact.dim_passive(), 0);
  EXPECT_EQ(robot_contact.max_point_contacts(), 1);
  EXPECT_EQ(robot_contact.is_contact_active(0), false);
  EXPECT_FALSE(robot_contact.has_floating_base());
  robot_contact.printRobotModel();
  Eigen::VectorXd effort_limit(dimq_), velocity_limit(dimq_), 
                  lower_position_limit(dimq_), upper_position_limit(dimq_);
  effort_limit << 300, 300, 300, 300, 300, 300, 300;
  velocity_limit << 10, 10, 10, 10, 10, 10, 10;
  lower_position_limit << -2.96705972839, -2.09439510239, -2.96705972839, 
                          -2.09439510239, -2.96705972839, -2.09439510239, 
                          -3.05432619099;
  upper_position_limit << 2.96705972839, 2.09439510239, 2.96705972839, 
                          2.09439510239, 2.96705972839, 2.09439510239, 
                          3.05432619099;
  EXPECT_TRUE(robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot_contact.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(upper_position_limit));
  Robot robot_ref = robot_contact; 
  robot_empty = std::move(robot_ref);
  EXPECT_EQ(robot_contact.dimq(), robot_empty.dimq());
  EXPECT_EQ(robot_contact.dimv(), robot_empty.dimv());
  EXPECT_EQ(robot_contact.dimf(), robot_empty.dimf());
  EXPECT_EQ(robot_contact.max_dimf(), robot_empty.max_dimf());
  EXPECT_EQ(robot_contact.dim_passive(), robot_empty.dim_passive());
  EXPECT_EQ(robot_contact.max_point_contacts(), robot_empty.max_point_contacts());
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit().isApprox(robot_empty.jointVelocityLimit()));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(robot_empty.lowerJointPositionLimit()));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(robot_empty.upperJointPositionLimit()));
}


TEST_F(FixedBaseRobotTest, moveConstructor) {
  // Default constructor
  std::vector<int> contacts = {contact_frame_id_};
  Robot robot_contact(urdf_, contacts, baumgarte_weight_on_velocity_, 
                      baumgarte_weight_on_position_);
  EXPECT_EQ(robot_contact.dimq(), dimq_);
  EXPECT_EQ(robot_contact.dimv(), dimq_);
  EXPECT_EQ(robot_contact.dimf(), 0);
  EXPECT_EQ(robot_contact.max_dimf(), 3);
  EXPECT_EQ(robot_contact.dim_passive(), 0);
  EXPECT_EQ(robot_contact.max_point_contacts(), 1);
  EXPECT_EQ(robot_contact.is_contact_active(0), false);
  EXPECT_FALSE(robot_contact.has_floating_base());
  robot_contact.printRobotModel();
  Eigen::VectorXd effort_limit(dimq_), velocity_limit(dimq_), 
                  lower_position_limit(dimq_), upper_position_limit(dimq_);
  effort_limit << 300, 300, 300, 300, 300, 300, 300;
  velocity_limit << 10, 10, 10, 10, 10, 10, 10;
  lower_position_limit << -2.96705972839, -2.09439510239, -2.96705972839, 
                          -2.09439510239, -2.96705972839, -2.09439510239, 
                          -3.05432619099;
  upper_position_limit << 2.96705972839, 2.09439510239, 2.96705972839, 
                          2.09439510239, 2.96705972839, 2.09439510239, 
                          3.05432619099;
  EXPECT_TRUE(robot_contact.jointEffortLimit().isApprox(effort_limit));
  EXPECT_TRUE(robot_contact.jointVelocityLimit().isApprox(velocity_limit));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(lower_position_limit));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(upper_position_limit));
  Robot robot_ref = robot_contact; 
  Robot robot_empty(std::move(robot_ref));
  EXPECT_EQ(robot_contact.dimq(), robot_empty.dimq());
  EXPECT_EQ(robot_contact.dimv(), robot_empty.dimv());
  EXPECT_EQ(robot_contact.dimf(), robot_empty.dimf());
  EXPECT_EQ(robot_contact.max_dimf(), robot_empty.max_dimf());
  EXPECT_EQ(robot_contact.dim_passive(), robot_empty.dim_passive());
  EXPECT_EQ(robot_contact.max_point_contacts(), robot_empty.max_point_contacts());
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointEffortLimit().isApprox(robot_empty.jointEffortLimit()));
  EXPECT_TRUE(
      robot_contact.jointVelocityLimit().isApprox(robot_empty.jointVelocityLimit()));
  EXPECT_TRUE(
      robot_contact.lowerJointPositionLimit().isApprox(robot_empty.lowerJointPositionLimit()));
  EXPECT_TRUE(
      robot_contact.upperJointPositionLimit().isApprox(robot_empty.upperJointPositionLimit()));
}


TEST_F(FixedBaseRobotTest, integrateConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q_ref = q_;
  Eigen::VectorXd q_integrated = q_;
  const double integration_length = Eigen::VectorXd::Random(2)[0];
  robot.integrateConfiguration(q_, v_, integration_length, q_integrated);
  pinocchio::integrate(model_, q_, integration_length*v_, q_ref);
  EXPECT_TRUE(q_integrated.isApprox(q_ref));
  Eigen::VectorXd q = q_;
  robot.integrateConfiguration(v_, integration_length, q);
  EXPECT_TRUE(q.isApprox(q_ref));
}


TEST_F(FixedBaseRobotTest, subtractConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q = q_;
  Eigen::VectorXd v_ref = v_;
  const double integration_length = std::abs(Eigen::VectorXd::Random(2)[0]);
  robot.integrateConfiguration(v_, integration_length, q);
  robot.subtractConfiguration(q, q_, v_ref);
  v_ref = v_ref / integration_length;
  EXPECT_TRUE(v_.isApprox(v_ref));
}


TEST_F(FixedBaseRobotTest, dIntegrateConfiguration) {
  Robot robot(urdf_);
  Eigen::MatrixXd dintegrate_dq = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dintegrate_dv = Eigen::MatrixXd::Zero(dimq_, dimq_);
  const double integration_length = std::abs(Eigen::VectorXd::Random(2)[0]);
  robot.dIntegrateConfiguration(q_, v_, integration_length, dintegrate_dq, 
                                dintegrate_dv);
  EXPECT_TRUE(dintegrate_dq.isApprox(Eigen::MatrixXd::Identity(dimq_, dimq_)));
  EXPECT_TRUE(dintegrate_dv.isApprox(Eigen::MatrixXd::Identity(dimq_, dimq_)));
}


TEST_F(FixedBaseRobotTest, dSubtractdConfigurationPlus) {
  Robot robot(urdf_);
  Eigen::VectorXd q_plus = q_;
  Eigen::VectorXd q_minus = q_;
  const double integration_length = std::abs(Eigen::VectorXd::Random(2)[0]);
  robot.integrateConfiguration(v_, integration_length, q_plus);
  Eigen::MatrixXd dSubtract_dqplus_ref 
    = Eigen::MatrixXd::Zero(robot.dimq(), robot.dimv());
  pinocchio::dDifference(model_, q_minus, q_plus, dSubtract_dqplus_ref, 
                         pinocchio::ARG1);
  Eigen::MatrixXd dSubtract_dqplus
    = Eigen::MatrixXd::Zero(robot.dimq(), robot.dimv());
  robot.dSubtractdConfigurationPlus(q_plus, q_minus, dSubtract_dqplus);
  EXPECT_TRUE(dSubtract_dqplus.isApprox(dSubtract_dqplus_ref));
  Eigen::MatrixXd dSubtract_dqminus_ref 
    = Eigen::MatrixXd::Zero(robot.dimq(), robot.dimv());
  pinocchio::dDifference(model_, q_minus, q_plus, dSubtract_dqminus_ref, 
                         pinocchio::ARG0);
  Eigen::MatrixXd dSubtract_dqminus
    = Eigen::MatrixXd::Zero(robot.dimq(), robot.dimv());
  robot.dSubtractdConfigurationMinus(q_plus, q_minus, dSubtract_dqminus);
  EXPECT_TRUE(dSubtract_dqminus.isApprox(dSubtract_dqminus_ref));
}


TEST_F(FixedBaseRobotTest, baumgarteResidualAndDerivatives) {
  std::vector<int> contact_frames = {contact_frame_id_};
  Robot robot(urdf_, contact_frames, baumgarte_weight_on_velocity_, 
              baumgarte_weight_on_position_);
  std::random_device rnd;
  const int segment_begin = rnd() % 5;
  Eigen::VectorXd residual 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  Eigen::VectorXd residual_ref 
      = Eigen::VectorXd::Zero(segment_begin+robot.max_dimf());
  std::vector<bool> is_each_contacts_active = {true};
  robot.setContactStatus(is_each_contacts_active);
  EXPECT_EQ(robot.dimf(), robot.max_dimf());
  EXPECT_EQ(robot.is_contact_active(0), true);
  robot.updateKinematics(q_, v_, a_);
  robot.setContactPointsByCurrentKinematics();
  robot.computeBaumgarteResidual(residual.segment<3>(segment_begin));
  PointContact contact_ref(model_, contact_frame_id_, 
                           baumgarte_weight_on_velocity_, 
                           baumgarte_weight_on_position_);
  pinocchio::forwardKinematics(model_, data_, q_, v_, a_);
  pinocchio::updateFramePlacements(model_, data_);
  pinocchio::computeForwardKinematicsDerivatives(model_, data_, q_, v_, a_);
  contact_ref.resetContactPointByCurrentKinematics(data_);
  contact_ref.computeBaumgarteResidual(model_, data_, residual_ref.segment<3>(segment_begin));
  EXPECT_TRUE(residual.isApprox(residual_ref));
  const double coeff = Eigen::VectorXd::Random(1)[0];
  robot.computeBaumgarteResidual(coeff, residual.segment<3>(segment_begin));
  EXPECT_TRUE(residual.isApprox(coeff*residual_ref));

  Eigen::MatrixXd baumgarte_partial_q_ref
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimq_);
  Eigen::MatrixXd baumgarte_partial_v_ref 
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimq_);
  Eigen::MatrixXd baumgarte_partial_a_ref 
      = Eigen::MatrixXd::Zero(robot.max_dimf(), dimq_);
  contact_ref.computeBaumgarteDerivatives(model_, data_, 
                                          baumgarte_partial_q_ref, 
                                          baumgarte_partial_v_ref, 
                                          baumgarte_partial_a_ref);
  const int block_rows_begin = rnd() % 5;
  const int block_cols_begin = rnd() % 5;
  Eigen::MatrixXd baumgarte_partial_q 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_v 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_a 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  robot.computeBaumgarteDerivatives(
      baumgarte_partial_q.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_v.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_a.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(
      baumgarte_partial_q.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_q_ref));
  EXPECT_TRUE(
      baumgarte_partial_v.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_v_ref));
  EXPECT_TRUE(
      baumgarte_partial_a.block(block_rows_begin, block_cols_begin, 
                                robot.max_dimf(), robot.dimv())
      .isApprox(baumgarte_partial_a_ref));
  Eigen::MatrixXd baumgarte_partial_q_coeff
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_v_coeff 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);
  Eigen::MatrixXd baumgarte_partial_a_coeff 
      = Eigen::MatrixXd::Zero(2*block_rows_begin+robot.max_dimf(), 2*block_cols_begin+dimq_);

  robot.computeBaumgarteDerivatives(
      coeff, 
      baumgarte_partial_q_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_v_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()), 
      baumgarte_partial_a_coeff.block(block_rows_begin, block_cols_begin, 
                                      robot.max_dimf(), robot.dimv()));
  EXPECT_TRUE(baumgarte_partial_q_coeff.isApprox(coeff*baumgarte_partial_q));
  EXPECT_TRUE(baumgarte_partial_v_coeff.isApprox(coeff*baumgarte_partial_v));
  EXPECT_TRUE(baumgarte_partial_a_coeff.isApprox(coeff*baumgarte_partial_a));
  std::cout << baumgarte_partial_q << std::endl;
  std::cout << baumgarte_partial_q_coeff << std::endl;
  std::cout << baumgarte_partial_v << std::endl;
  std::cout << baumgarte_partial_v_coeff << std::endl;
  std::cout << baumgarte_partial_a << std::endl;
  std::cout << baumgarte_partial_a_coeff << std::endl;
}


TEST_F(FixedBaseRobotTest, RNEA) {
  // without contact
  Robot robot(urdf_);
  Eigen::VectorXd tau = Eigen::VectorXd::Zero(dimq_);
  robot.RNEA(q_, v_, a_, tau);
  Eigen::VectorXd tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_);
  EXPECT_TRUE(tau_ref.isApprox(tau));
  std::vector<int> contact_frames = {contact_frame_id_};
  Robot robot_contact(urdf_, contact_frames, baumgarte_weight_on_velocity_, 
                      baumgarte_weight_on_position_);
  tau = Eigen::VectorXd::Zero(dimq_);
  tau_ref = Eigen::VectorXd::Zero(dimq_);
  Eigen::VectorXd fext = Eigen::VectorXd::Random(robot_contact.max_dimf());
  robot_contact.RNEA(q_, v_, a_, tau);
  tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_);
  EXPECT_TRUE(tau_ref.isApprox(tau));
  // with contact
  std::vector<bool> is_each_contacts_active = {true};
  robot_contact.setContactStatus(is_each_contacts_active);
  robot_contact.setContactForces(fext);
  robot_contact.RNEA(q_, v_, a_, tau);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  PointContact contact_ref(model_, contact_frame_id_, 
                           baumgarte_weight_on_velocity_, 
                           baumgarte_weight_on_position_);
  contact_ref.computeJointForceFromContactForce(fext, fjoint);
  tau_ref = pinocchio::rnea(model_, data_, q_, v_, a_, fjoint);
  EXPECT_TRUE(tau_ref.isApprox(tau));
}


TEST_F(FixedBaseRobotTest, RNEADerivativesWithoutFext) {
  Robot robot(urdf_);
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_da = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_dq_ref = dRNEA_dq;
  Eigen::MatrixXd dRNEA_dv_ref = dRNEA_dv;
  Eigen::MatrixXd dRNEA_da_ref = dRNEA_da;
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_da);
  pinocchio::computeRNEADerivatives(model_, data_, q_, v_, a_, dRNEA_dq_ref, 
                                    dRNEA_dv_ref, dRNEA_da_ref);
  dRNEA_da_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_da_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_dv.isApprox(dRNEA_dv_ref));
  EXPECT_TRUE(dRNEA_da.isApprox(dRNEA_da_ref));
}


TEST_F(FixedBaseRobotTest, RNEADerivativesWithContacts) {
  std::vector<int> contact_frames = {contact_frame_id_};
  Robot robot(urdf_, contact_frames, baumgarte_weight_on_velocity_, 
              baumgarte_weight_on_position_);
  Eigen::VectorXd fext = Eigen::VectorXd::Random(robot.max_dimf());
  Eigen::MatrixXd dRNEA_dq = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_dv = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_da = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_dfext = Eigen::MatrixXd::Zero(dimq_, robot.max_dimf());
  Eigen::MatrixXd dRNEA_dq_ref = dRNEA_dq;
  Eigen::MatrixXd dRNEA_dv_ref = dRNEA_dv;
  Eigen::MatrixXd dRNEA_da_ref = Eigen::MatrixXd::Zero(dimq_, dimq_);
  Eigen::MatrixXd dRNEA_dfext_ref 
      = Eigen::MatrixXd::Zero(dimq_, robot.max_dimf());
  std::vector<bool> is_each_contacts_active = {true};
  robot.setContactStatus(is_each_contacts_active);
  robot.setContactForces(fext);
  robot.RNEADerivatives(q_, v_, a_, dRNEA_dq, dRNEA_dv, dRNEA_da);
  pinocchio::container::aligned_vector<pinocchio::Force> fjoint 
      = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  PointContact contact_ref(model_, contact_frame_id_, 
                           baumgarte_weight_on_velocity_, 
                           baumgarte_weight_on_position_);
  contact_ref.computeJointForceFromContactForce(fext, fjoint);
  pinocchio::computeRNEADerivatives(model_, data_, q_, v_, a_, fjoint, 
                                    dRNEA_dq_ref, dRNEA_dv_ref, dRNEA_da_ref);
  dRNEA_da_ref.triangularView<Eigen::StrictlyLower>() 
      = dRNEA_da_ref.transpose().triangularView<Eigen::StrictlyLower>();
  EXPECT_TRUE(dRNEA_dq.isApprox(dRNEA_dq_ref));
  EXPECT_TRUE(dRNEA_dv.isApprox(dRNEA_dv_ref));
  EXPECT_TRUE(dRNEA_da.isApprox(dRNEA_da_ref));
  const bool transpose_jacobian = true;
  robot.dRNEAPartialdFext(dRNEA_dfext);
  contact_ref.getContactJacobian(model_, data_, -1, dRNEA_dfext_ref,
                                 transpose_jacobian);
  std::cout << dRNEA_dfext_ref << std::endl;
  std::cout << dRNEA_dfext << std::endl;
  EXPECT_TRUE(dRNEA_dfext.isApprox(dRNEA_dfext_ref));
}


TEST_F(FixedBaseRobotTest, generateFeasibleConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q = Eigen::VectorXd::Zero(robot.dimq());
  robot.generateFeasibleConfiguration(q);
  Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  Eigen::VectorXd qmax = robot.upperJointPositionLimit();
  for (int i=0; i<robot.dimq(); ++i) {
    EXPECT_TRUE(q(i) >= qmin(i));
    EXPECT_TRUE(q(i) <= qmax(i));
  }
}


TEST_F(FixedBaseRobotTest, normalizeConfiguration) {
  Robot robot(urdf_);
  Eigen::VectorXd q = Eigen::VectorXd::Random(robot.dimq());
  Eigen::VectorXd q_ref = q;
  robot.normalizeConfiguration(q);
  EXPECT_TRUE(q.isApprox(q_ref));
}

} // namespace idocp 


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}