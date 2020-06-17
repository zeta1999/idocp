#ifndef IDOCP_JOINT_VELOCITY_LOWER_LIMITS_SOFT_HPP_
#define IDOCP_JOINT_VELOCITY_LOWER_LIMITS_SOFT_HPP_

#include "Eigen/Core"

#include "robot/robot.hpp"


namespace idocp {
namespace softconstraints {

class JointVelocityLowerLimits {
public:
  JointVelocityLowerLimits(const Robot& robot, const double barrier);

  // Use default copy constructor.
  JointVelocityLowerLimits(const JointVelocityLowerLimits&) = default;

  // Use default copy operator.
  JointVelocityLowerLimits& operator=(const JointVelocityLowerLimits&) = default;

  void setSlackAndDual(const Robot& robot, const double dtau,
                       const Eigen::VectorXd& v);

  void linearizeConstraint(const Robot& robot, const double dtau, 
                           const Eigen::VectorXd& v);

  void updateSlackAndDual(const Robot& robot, const double dtau, 
                          const Eigen::VectorXd& dv); 

  void condenseSlackAndDual(const Robot& robot, const double dtau, 
                            Eigen::MatrixXd& Cvv, Eigen::VectorXd& Cv) const;

  void augmentDualResidual(const Robot& robot, const double dtau,
                           Eigen::VectorXd& Cv);

  double squaredConstraintsResidualNrom(const Robot& robot, const double dtau,
                                        const Eigen::VectorXd& v);

private:
  unsigned int dimq_, dimv_, dimc_;
  double barrier_;
  Eigen::VectorXd vmin_, slack_, dual_, residual_, FB_residual_, dslack_, 
                  ddual_;
};

} // namespace softconstraints
} // namespace idocp


#endif // IDOCP_JOINT_VELOCITY_LOWER_LIMITS_SOFT_HPP_