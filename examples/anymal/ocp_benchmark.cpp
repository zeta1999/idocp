#include <iostream>
#include <string>
#include <memory>
#include <chrono>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/parnmpc.hpp"
#include "idocp/ocp/ocp.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/joint_space_cost.hpp"
// #include "idocp/cost/contact_cost.hpp"
#include "idocp/constraints/constraints.hpp"

#include "idocp/utils/joint_constraints_factory.hpp"
#include "idocp/utils/ocp_benchmarker.hpp"

namespace ocpbenchmark {
namespace anymal {

void BenchmarkWithContacts() {
  srand((unsigned int) time(0));
  std::vector<int> contact_frames = {14, 24, 34, 44};
  const std::string urdf_file_name = "../anymal/anymal.urdf";
  idocp::Robot robot(urdf_file_name, contact_frames);
  Eigen::VectorXd q_ref(robot.dimq());
  q_ref << 0, 0, 0.48, 0, 0, 0, 1, 
           0.0315, 0.4, -0.8, 
           0.0315, -0.4, 0.8, 
           -0.0315, 0.4, -0.8,
           -0.0315, -0.4, 0.8;
  auto cost = std::make_shared<idocp::CostFunction>();
  auto joint_cost = std::make_shared<idocp::JointSpaceCost>(robot);
  joint_cost->set_q_weight(Eigen::VectorXd::Constant(robot.dimv(), 10));
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_qf_weight(Eigen::VectorXd::Constant(robot.dimv(), 10));
  joint_cost->set_v_weight(Eigen::VectorXd::Constant(robot.dimv(), 1));
  joint_cost->set_vf_weight(Eigen::VectorXd::Constant(robot.dimv(), 1));
  joint_cost->set_a_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.01));
  joint_cost->set_u_weight(Eigen::VectorXd::Constant(robot.dimv(), 0.0));
  // auto contact_cost = std::make_shared<idocp::ContactCost>(robot);
  // std::vector<Eigen::Vector3d> f_weight;
  // for (int i=0; i<contact_frames.size(); ++i) {
  //   f_weight.push_back(Eigen::Vector3d::Constant(0.0));
  // }
  // contact_cost->set_f_weight(f_weight);
  cost->push_back(joint_cost);
  // cost->push_back(contact_cost);
  idocp::JointConstraintsFactory constraints_factory(robot);
  auto constraints = constraints_factory.create();
  const double T = 1;
  const int N = 50;
  const int num_proc = 4;
  const double t = 0;
  Eigen::VectorXd q = Eigen::VectorXd::Zero(robot.dimq());
  q << 0, 0, 0.48, 0, 0, 0, 1, 
       0.0315, 0.4, -0.8, 
       0.0315, -0.4, 0.8, 
       -0.0315, 0.4, -0.8,
       -0.0315, -0.4, 0.8;
  const Eigen::VectorXd v = Eigen::VectorXd::Random(robot.dimv());
  robot.updateKinematics(q, v, Eigen::VectorXd::Zero(robot.dimv()));
  robot.setContactPointsByCurrentKinematics();

  idocp::OCPBenchmarker<idocp::OCP> ocp_benchmarker("OCP for anymal with contacts",
                                                    robot, cost, constraints, T, N, num_proc);
  q << 0, 0, 0.5, 0, 0, 0, 1, 
       0.0315, 0.4, -0.8, 
       0.0315, -0.4, 0.8, 
       -0.0315, 0.4, -0.8,
       -0.0315, -0.4, 0.8;
  ocp_benchmarker.setInitialGuessSolution(t, q, v);
  ocp_benchmarker.getSolverHandle()->activateContacts({0, 1, 2, 3}, 0, N);
  ocp_benchmarker.testConvergence(t, q, v, 20, false);
  ocp_benchmarker.testCPUTime(t, q, v);
  idocp::OCPBenchmarker<idocp::ParNMPC> parnmpc_benchmarker("ParNMPC for anymal with contacts",
                                                            robot, cost, constraints, T, N, num_proc);
  parnmpc_benchmarker.setInitialGuessSolution(t, q, v);
  parnmpc_benchmarker.getSolverHandle()->activateContacts({0, 1, 2, 3}, 0, N);
  parnmpc_benchmarker.testConvergence(t, q, v, 20, false);
  parnmpc_benchmarker.testCPUTime(t, q, v);
}

} // namespace anymal
} // namespace ocpbenchmark


int main() {
  ocpbenchmark::anymal::BenchmarkWithContacts();
  return 0;
}