#include <string>
#include <random>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/complementarity/baumgarte_inequality.hpp"
#include "idocp/constraints/constraint_component_data.hpp"

namespace idocp {

class FloatingBaseBaumgarteInequalityTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    barrier_ = 1.0e-04;
    dtau_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    fraction_to_boundary_rate_ = std::abs(Eigen::VectorXd::Random(1)[0]);
    const std::vector<int> contact_frames = {14, 24, 34, 44};
    const std::string urdf = "../urdf/anymal/anymal.urdf";
    robot_ = Robot(urdf, contact_frames, 0, 0);
    baumgarte_inequality_ = BaumgarteInequality(robot_, barrier_, fraction_to_boundary_rate_);
  }

  virtual void TearDown() {
  }

  double barrier_, dtau_, fraction_to_boundary_rate_;
  Eigen::VectorXd slack_, dual_, dslack_, ddual_;
  Robot robot_;
  BaumgarteInequality baumgarte_inequality_;
};


TEST_F(FloatingBaseBaumgarteInequalityTest, isFeasible) {
  SplitSolution s(robot_);
  assert(s.f.size() == 3*4);
  assert(s.f_verbose.size() == 7*4);
  assert(robot_.num_point_contacts() == 4);
  s.q = Eigen::VectorXd::Random(robot_.dimq());
  robot_.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot_.dimv());
  s.a = Eigen::VectorXd::Random(robot_.dimv());
  s.f_verbose = Eigen::VectorXd::Random(7*robot_.num_point_contacts()).array().abs();
  s.f_verbose(5) = -1;
  s.f_verbose(6) = -1;
  s.set_f();
  robot_.updateKinematics(s.q, s.v, s.a);
  EXPECT_FALSE(baumgarte_inequality_.isFeasible(robot_, s));
}


TEST_F(FloatingBaseBaumgarteInequalityTest, computePrimalResidual) {
  SplitSolution s(robot_);
  s.q = Eigen::VectorXd::Random(robot_.dimq());
  robot_.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot_.dimv());
  s.a = Eigen::VectorXd::Random(robot_.dimv());
  s.f_verbose = Eigen::VectorXd::Random(7*robot_.num_point_contacts()).array().abs();
  s.set_f();
  const int dimc = 6*robot_.num_point_contacts();
  ConstraintComponentData data(dimc);
  data.slack = Eigen::VectorXd::Random(dimc).array().abs();
  robot_.updateKinematics(s.q, s.v, s.a);
  baumgarte_inequality_.computePrimalResidual(robot_, dtau_, s, data);
  Eigen::VectorXd baum_residual(Eigen::VectorXd::Zero(3*4));
  robot_.computeBaumgarteResidual(baum_residual); 
  Eigen::VectorXd residual_ref(Eigen::VectorXd::Zero(dimc));
  for (int i=0; i<robot_.num_point_contacts(); ++i) {
    residual_ref(6*i+0) = - dtau_ * (s.f_verbose(7*i+5) + baum_residual(3*i+0));
    residual_ref(6*i+1) = - dtau_ * (s.f_verbose(7*i+5) - baum_residual(3*i+0));
    residual_ref(6*i+2) = - dtau_ * (s.f_verbose(7*i+6) + baum_residual(3*i+1));
    residual_ref(6*i+3) = - dtau_ * (s.f_verbose(7*i+6) - baum_residual(3*i+1));
    residual_ref(6*i+4) = - dtau_ * baum_residual(3*i+2);
    residual_ref(6*i+5) = - dtau_ * (s.f_verbose(7*i+5)*s.f_verbose(7*i+5)+s.f_verbose(7*i+6)*s.f_verbose(7*i+6));
  }
  residual_ref += data.slack;
  EXPECT_TRUE(data.residual.isApprox(residual_ref));
}


TEST_F(FloatingBaseBaumgarteInequalityTest, augmentDualResidual) {
  SplitSolution s(robot_);
  s.q = Eigen::VectorXd::Random(robot_.dimq());
  robot_.generateFeasibleConfiguration(s.q);
  s.v = Eigen::VectorXd::Random(robot_.dimv());
  s.a = Eigen::VectorXd::Random(robot_.dimv());
  s.f_verbose = Eigen::VectorXd::Random(7*robot_.num_point_contacts()).array().abs();
  s.set_f();
  const int dimc = 6*robot_.num_point_contacts();
  ConstraintComponentData data(dimc);
  data.dual = Eigen::VectorXd::Random(dimc).array().abs();
  KKTResidual kkt_residual(robot_);
  robot_.updateKinematics(s.q, s.v, s.a);
  baumgarte_inequality_.augmentDualResidual(robot_, dtau_, s, data, kkt_residual);
  Eigen::MatrixXd dbaum_dq(Eigen::MatrixXd::Zero(3*robot_.num_point_contacts(), robot_.dimv()));
  Eigen::MatrixXd dbaum_dv(Eigen::MatrixXd::Zero(3*robot_.num_point_contacts(), robot_.dimv()));
  Eigen::MatrixXd dbaum_da(Eigen::MatrixXd::Zero(3*robot_.num_point_contacts(), robot_.dimv()));
  robot_.computeBaumgarteDerivatives(dbaum_dq, dbaum_dv, dbaum_da); 
  Eigen::MatrixXd g_a(Eigen::MatrixXd::Zero(6*robot_.num_point_contacts(), robot_.dimv()));
  Eigen::MatrixXd g_f(Eigen::MatrixXd::Zero(6*robot_.num_point_contacts(), 7*robot_.num_point_contacts()));
  Eigen::MatrixXd g_q(Eigen::MatrixXd::Zero(6*robot_.num_point_contacts(), robot_.dimv()));
  Eigen::MatrixXd g_v(Eigen::MatrixXd::Zero(6*robot_.num_point_contacts(), robot_.dimv()));
  for (int i=0; i<robot_.num_point_contacts(); ++i) {
    g_a.row(6*i+0) = dtau_ * dbaum_da.row(3*i+0);
    g_a.row(6*i+1) = - dtau_ * dbaum_da.row(3*i+0);
    g_a.row(6*i+2) = dtau_ * dbaum_da.row(3*i+1);
    g_a.row(6*i+3) = - dtau_ * dbaum_da.row(3*i+1);
    g_a.row(6*i+4) = dtau_ * dbaum_da.row(3*i+2);
    g_a.row(6*i+5).setZero();
  }
  for (int i=0; i<robot_.num_point_contacts(); ++i) {
    g_f(6*i+0, 7*i+5) = dtau_;
    g_f(6*i+1, 7*i+5) = dtau_;
    g_f(6*i+2, 7*i+6) = dtau_;
    g_f(6*i+3, 7*i+6) = dtau_;
    g_f(6*i+5, 7*i+5) = 2 * dtau_ * s.f_verbose(7*i+5);
    g_f(6*i+5, 7*i+6) = 2 * dtau_ * s.f_verbose(7*i+6);
  }
  for (int i=0; i<robot_.num_point_contacts(); ++i) {
    g_q.row(6*i+0) = dtau_ * dbaum_dq.row(3*i+0);
    g_q.row(6*i+1) = - dtau_ * dbaum_dq.row(3*i+0);
    g_q.row(6*i+2) = dtau_ * dbaum_dq.row(3*i+1);
    g_q.row(6*i+3) = - dtau_ * dbaum_dq.row(3*i+1);
    g_q.row(6*i+4) = dtau_ * dbaum_dq.row(3*i+2);
    g_q.row(6*i+5).setZero();
  }
  for (int i=0; i<robot_.num_point_contacts(); ++i) {
    g_v.row(6*i+0) = dtau_ * dbaum_dv.row(3*i+0);
    g_v.row(6*i+1) = - dtau_ * dbaum_dv.row(3*i+0);
    g_v.row(6*i+2) = dtau_ * dbaum_dv.row(3*i+1);
    g_v.row(6*i+3) = - dtau_ * dbaum_dv.row(3*i+1);
    g_v.row(6*i+4) = dtau_ * dbaum_dv.row(3*i+2);
    g_v.row(6*i+5).setZero();
  }

  std::cout << g_f << std::endl;
  Eigen::VectorXd la_ref(Eigen::VectorXd::Zero(robot_.dimv()));
  Eigen::VectorXd lf_ref(Eigen::VectorXd::Zero(7*robot_.num_point_contacts()));
  Eigen::VectorXd lq_ref(Eigen::VectorXd::Zero(robot_.dimv()));
  Eigen::VectorXd lv_ref(Eigen::VectorXd::Zero(robot_.dimv()));
  la_ref = - g_a.transpose() * data.dual;
  lf_ref = - g_f.transpose() * data.dual;
  lq_ref = - g_q.transpose() * data.dual;
  lv_ref = - g_v.transpose() * data.dual;
  EXPECT_TRUE(kkt_residual.la().isApprox(la_ref));
  EXPECT_TRUE(kkt_residual.lf().isApprox(lf_ref));
  EXPECT_TRUE(kkt_residual.lq().isApprox(lq_ref));
  EXPECT_TRUE(kkt_residual.lv().isApprox(lv_ref));
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}