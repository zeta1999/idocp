#ifndef IDOCP_UTILS_RUNGE_KUTTA_HPP_
#define IDOCP_UTILS_RUNGE_KUTTA_HPP_

#include "Eigen/Core"
#include "idocp/robot/robot.hpp"

namespace idocp {

class RungeKutta {
public:
  RungeKutta(const Robot& robot);

  void integrate(const double integration_length, const Eigen::VectorXd& q, 
                 const Eigen::VectorXd& v, const Eigen::VectorXd& tau, 
                 Eigen::VectorXd& q_next, Eigen::VectorXd& v_next);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  Robot robot_;
  Eigen::VectorXd dq_, dv_, kq1_, kq2_, kq3_, kq4_, kv1_, kv2_, kv3_, kv4_;

};

} // namespace idocp

#endif // IDOCP_UTILS_RUNGE_KUTTA_HPP_