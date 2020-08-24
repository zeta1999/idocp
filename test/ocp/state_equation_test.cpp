#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/state_equation.hpp"


namespace idocp {

class StateEquationTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf_ = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf_ = "../urdf/anymal/anymal.urdf";
    dtau_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    t_ = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  virtual void TearDown() {
  }

  double dtau_, t_;
  std::string fixed_base_urdf_, floating_base_urdf_;
};


TEST_F(StateEquationTest, forwardEuler_fixed_base) {
  std::vector<int> contact_frames = {18};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(fixed_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  SplitSolution s_next(robot);
  s_next.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s_next.q);
  s_next.v = Eigen::VectorXd::Random(robot.dimv());
  s_next.a = Eigen::VectorXd::Random(robot.dimv());
  s_next.f = Eigen::VectorXd::Random(robot.max_dimf());
  s_next.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s_next.lmd = Eigen::VectorXd::Random(robot.dimv());
  s_next.gmm = Eigen::VectorXd::Random(robot.dimv());
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(robot);
  StateEquation state_equation(robot);
  state_equation.linearizeForwardEuler(robot, dtau_, s, s_next, kkt_matrix, 
                                       kkt_residual);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((s.q+dtau_*s.v-s_next.q)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((s.v+dtau_*s.a-s_next.v)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((s_next.lmd-s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s_next.lmd+s_next.gmm-s.gmm)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s_next.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isApprox(Eigen::MatrixXd::Identity(robot.dimv(), robot.dimv())));
  EXPECT_TRUE(kkt_matrix.Fqv.isApprox(Eigen::MatrixXd::Identity(robot.dimv(), robot.dimv())));
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), 
                   state_equation.violationL1Norm(kkt_residual));
}


TEST_F(StateEquationTest, forwardEuler_floating_base) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(floating_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0, rnd()%2==0, rnd()%2==0, rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  SplitSolution s_next(robot);
  s_next.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s_next.q);
  s_next.v = Eigen::VectorXd::Random(robot.dimv());
  s_next.a = Eigen::VectorXd::Random(robot.dimv());
  s_next.f = Eigen::VectorXd::Random(robot.max_dimf());
  s_next.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s_next.lmd = Eigen::VectorXd::Random(robot.dimv());
  s_next.gmm = Eigen::VectorXd::Random(robot.dimv());
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(robot);
  StateEquation state_equation(robot);
  state_equation.linearizeForwardEuler(robot, dtau_, s, s_next, kkt_matrix, 
                                       kkt_residual);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(s.q, s_next.q, qdiff);
  Eigen::MatrixXd dintegrate_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd dintegrate_dv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.dIntegrateConfiguration(s.q, s.v, dtau_, dintegrate_dq, dintegrate_dv);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((qdiff+dtau_*s.v)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((s.v+dtau_*s.a-s_next.v)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((s_next.lmd-s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s_next.lmd+s_next.gmm-s.gmm)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s_next.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isApprox(dintegrate_dq));
  EXPECT_TRUE(kkt_matrix.Fqv.isApprox(dintegrate_dv));
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), 
                   state_equation.violationL1Norm(kkt_residual));
}


TEST_F(StateEquationTest, backwardEuler_fixed_base) {
  std::vector<int> contact_frames = {18};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(fixed_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  Eigen::VectorXd q_prev = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_prev);
  const Eigen::VectorXd v_prev = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd lmd_next = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd gmm_next = Eigen::VectorXd::Random(robot.dimv());
  Eigen::VectorXd q_next = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_next);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(robot);
  StateEquation state_equation(robot);
  state_equation.linearizeBackwardEuler(robot, dtau_, q_prev, v_prev, s, 
                                        lmd_next, gmm_next, q_next, 
                                        kkt_matrix,  kkt_residual);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((q_prev-s.q+dtau_*s.v)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((v_prev-s.v+dtau_*s.a)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((lmd_next-s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s.lmd-s.gmm+gmm_next)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isZero());
  kkt_residual.setZero();
  kkt_matrix.setZero();
  state_equation.linearizeBackwardEulerTerminal(robot, dtau_, q_prev, v_prev, 
                                                s, kkt_matrix, kkt_residual);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((q_prev-s.q+dtau_*s.v)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((v_prev-s.v+dtau_*s.a)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((-1*s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s.lmd-s.gmm)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isZero());
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), 
                   state_equation.violationL1Norm(kkt_residual));
}


TEST_F(StateEquationTest, backwardEuler_floating_base) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(floating_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0, rnd()%2==0, rnd()%2==0, rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  Eigen::VectorXd q_prev = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_prev);
  const Eigen::VectorXd v_prev = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd lmd_next = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd gmm_next = Eigen::VectorXd::Random(robot.dimv());
  Eigen::VectorXd q_next = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_next);
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  KKTMatrix kkt_matrix(robot);
  kkt_matrix.setContactStatus(robot);
  StateEquation state_equation(robot);
  state_equation.linearizeBackwardEuler(robot, dtau_, q_prev, v_prev, s, 
                                        lmd_next, gmm_next, q_next, 
                                        kkt_matrix,  kkt_residual);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  Eigen::MatrixXd dsubtract_dqminus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd dsubtract_dqplus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.dSubtractdConfigurationMinus(q_prev, s.q, dsubtract_dqminus);
  robot.dSubtractdConfigurationPlus(s.q, q_next, dsubtract_dqplus);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((qdiff+dtau_*s.v)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((v_prev-s.v+dtau_*s.a)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((dsubtract_dqplus.transpose()*lmd_next+dsubtract_dqminus.transpose()*s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s.lmd-s.gmm+gmm_next)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isApprox(dsubtract_dqminus));
  kkt_residual.setZero();
  kkt_matrix.setZero();
  state_equation.linearizeBackwardEulerTerminal(robot, dtau_, q_prev, v_prev, 
                                                s, kkt_matrix, kkt_residual);
  EXPECT_TRUE(kkt_residual.Fq().isApprox((qdiff+dtau_*s.v)));
  EXPECT_TRUE(kkt_residual.Fv().isApprox((v_prev-s.v+dtau_*s.a)));
  EXPECT_TRUE(kkt_residual.lq().isApprox((dsubtract_dqminus.transpose()*s.lmd)));
  EXPECT_TRUE(kkt_residual.lv().isApprox((dtau_*s.lmd-s.gmm)));
  EXPECT_TRUE(kkt_residual.la().isApprox((dtau_*s.gmm)));
  EXPECT_TRUE(kkt_matrix.Fqq.isApprox(dsubtract_dqminus));
  std::cout << "kkt_matrix.Fqq" << std::endl;
  std::cout << kkt_matrix.Fqq << std::endl;
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), 
                   state_equation.violationL1Norm(kkt_residual));
}


TEST_F(StateEquationTest, violationFixedBase) {
  std::vector<int> contact_frames = {18};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(fixed_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  SplitSolution s_next(robot);
  s_next.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s_next.q);
  s_next.v = Eigen::VectorXd::Random(robot.dimv());
  s_next.a = Eigen::VectorXd::Random(robot.dimv());
  s_next.f = Eigen::VectorXd::Random(robot.max_dimf());
  s_next.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s_next.lmd = Eigen::VectorXd::Random(robot.dimv());
  s_next.gmm = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dq_next = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dv_next = Eigen::VectorXd::Random(robot.dimv());
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  StateEquation state_equation(robot);
  const double step_size = 0.3;
  const double forward_l1 
      = state_equation.computeForwardEulerViolationL1Norm(robot, step_size, dtau_, 
                                                          s, s_next.q, s_next.v, 
                                                          dq_next, dv_next, 
                                                          kkt_residual);
  kkt_residual.Fq() = s.q + dtau_ * s.v - s_next.q - step_size * dq_next;
  kkt_residual.Fv() = s.v + dtau_ * s.a - s_next.v - step_size * dv_next;
  Eigen::VectorXd q_prev = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_prev);
  const Eigen::VectorXd v_prev = Eigen::VectorXd::Random(robot.dimv());
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), forward_l1);
  const double backrward_l1_initial
      = state_equation.computeBackwardEulerViolationL1Norm(robot, dtau_, 
                                                           q_prev, v_prev, s,
                                                           kkt_residual);
  kkt_residual.Fq() = q_prev - s.q + dtau_ * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau_ * s.a;
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), backrward_l1_initial);
  const Eigen::VectorXd dq_prev = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dv_prev = Eigen::VectorXd::Random(robot.dimv());
  const double backrward_l1
      = state_equation.computeBackwardEulerViolationL1Norm(robot, step_size, dtau_, 
                                                           q_prev, v_prev,
                                                           dq_prev, dv_prev, s,
                                                           kkt_residual);
  kkt_residual.Fq() = q_prev + step_size * dq_prev - s.q + dtau_ * s.v;
  kkt_residual.Fv() = v_prev + step_size * dv_prev - s.v + dtau_ * s.a;
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), backrward_l1);
}


TEST_F(StateEquationTest, violationFloatingBase) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
  Robot robot(floating_base_urdf_, contact_frames, baum_a, baum_b);
  std::random_device rnd;
  std::vector<bool> contact_status = {rnd()%2==0, rnd()%2==0, rnd()%2==0, rnd()%2==0};
  robot.setContactStatus(contact_status);
  SplitSolution s(robot);
  s.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot.dimv());
  s.a = Eigen::VectorXd::Random(robot.dimv());
  s.f = Eigen::VectorXd::Random(robot.max_dimf());
  s.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s.lmd = Eigen::VectorXd::Random(robot.dimv());
  s.gmm = Eigen::VectorXd::Random(robot.dimv());
  SplitSolution s_next(robot);
  s_next.setContactStatus(robot);
  robot.generateFeasibleConfiguration(s_next.q);
  s_next.v = Eigen::VectorXd::Random(robot.dimv());
  s_next.a = Eigen::VectorXd::Random(robot.dimv());
  s_next.f = Eigen::VectorXd::Random(robot.max_dimf());
  s_next.mu = Eigen::VectorXd::Random(robot.dim_passive()+robot.max_dimf());
  s_next.lmd = Eigen::VectorXd::Random(robot.dimv());
  s_next.gmm = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dq_next = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dv_next = Eigen::VectorXd::Random(robot.dimv());
  KKTResidual kkt_residual(robot);
  kkt_residual.setContactStatus(robot);
  StateEquation state_equation(robot);
  const double step_size = 0.3;
  const double forward_l1 
      = state_equation.computeForwardEulerViolationL1Norm(robot, step_size, dtau_, 
                                                          s, s_next.q, s_next.v, 
                                                          dq_next, dv_next, 
                                                          kkt_residual);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(s.q, s_next.q, qdiff);
  kkt_residual.Fq() = qdiff + dtau_ * s.v - step_size * dq_next;
  kkt_residual.Fv() = s.v + dtau_ * s.a - s_next.v - step_size * dv_next;
  Eigen::VectorXd q_prev = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_prev);
  const Eigen::VectorXd v_prev = Eigen::VectorXd::Random(robot.dimv());
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), forward_l1);
  const double backrward_l1_initial
      = state_equation.computeBackwardEulerViolationL1Norm(robot, dtau_, 
                                                           q_prev, v_prev, s,
                                                           kkt_residual);
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  kkt_residual.Fq() = qdiff + dtau_ * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau_ * s.a;
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), backrward_l1_initial);
  const Eigen::VectorXd dq_prev = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dv_prev = Eigen::VectorXd::Random(robot.dimv());
  const double backrward_l1
      = state_equation.computeBackwardEulerViolationL1Norm(robot, step_size, dtau_, 
                                                           q_prev, v_prev,
                                                           dq_prev, dv_prev, s,
                                                           kkt_residual);
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  kkt_residual.Fq() = qdiff + step_size * dq_prev + dtau_ * s.v;
  kkt_residual.Fv() = v_prev + step_size * dv_prev - s.v + dtau_ * s.a;
  EXPECT_DOUBLE_EQ(kkt_residual.Fx().lpNorm<1>(), backrward_l1);
}


} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}