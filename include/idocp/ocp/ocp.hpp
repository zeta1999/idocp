#ifndef IDOCP_OCP_HPP_
#define IDOCP_OCP_HPP_ 

#include <vector>
#include <utility>

#include "Eigen/Core"

#include "robot/robot.hpp"
#include "ocp/split_ocp.hpp"
#include "ocp/split_terminal_ocp.hpp"
#include "ocp/line_search_filter.hpp"
#include "cost/cost_function_interface.hpp"
#include "constraints/constraints_interface.hpp"


namespace idocp {

class OCP {
public:
  // Constructor. 
  OCP(const Robot& robot, const CostFunctionInterface& cost,
      const ConstraintsInterface& constraints, const double T, const int N, 
      const int num_proc=1);

  ~OCP();

  // Use default copy constructor.
  OCP(const OCP&) = default;

  // Use default copy operator.
  OCP& operator=(const OCP&) = default;

  void solveSQP(const double t, const Eigen::VectorXd& q, 
                const Eigen::VectorXd& v, const bool use_line_search=true);

  void getInitialControlInput(Eigen::VectorXd& u);

  void getStateFeedbackGain(Eigen::MatrixXd& Kq, Eigen::MatrixXd& Kv);

  void setStateTrajectory(const Eigen::VectorXd& q, const Eigen::VectorXd& v);

  void setStateTrajectory(const Eigen::VectorXd& q0, const Eigen::VectorXd& v0,
                          const Eigen::VectorXd& qN, const Eigen::VectorXd& vN);

  double KKTError(const double t, const Eigen::VectorXd& q, 
                  const Eigen::VectorXd& v);

  void printSolution() const;

private:

  bool isCurrentSolutionFeasible();

  void initConstraints();

  void activateAllContacts();

  std::vector<SplitOCP> split_OCPs_;
  SplitTerminalOCP split_terminal_OCP_;
  std::vector<Robot> robots_;
  LineSearchFilter filter_;
  CostFunctionInterface* cost_;
  ConstraintsInterface* constraints_;
  double T_, dtau_, step_size_reduction_rate_, min_step_size_;
  int N_, num_proc_;
  std::vector<Eigen::VectorXd> q_, v_, a_, u_, beta_, lmd_, gmm_, 
                               dq_, dv_, sq_, sv_;
  std::vector<Eigen::MatrixXd> Pqq_, Pqv_, Pvq_, Pvv_;
  Eigen::VectorXd primal_step_sizes_, dual_step_sizes_, costs_, 
                  constraints_violations_, cost_derivative_dot_direction_;
  std::vector<std::vector<bool>> contact_sequence_;
};

} // namespace idocp 


#endif // IDOCP_OCP_HPP_