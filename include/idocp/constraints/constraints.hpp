#ifndef IDOCP_CONSTRAINTS_HPP_
#define IDOCP_CONSTRAINTS_HPP_

#include <vector>
#include <memory>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/constraints/constraint_component_base.hpp"
#include "idocp/constraints/constraint_component_data.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"


namespace idocp {

///
/// @typedef ConstraintsData
/// @brief Data for constraints. Composed of ConstraintComponentData 
/// corrensponding to the components of Constraints.
///
typedef std::vector<ConstraintComponentData> ConstraintsData;

///
/// @class Constraints 
/// @brief Stack of the inequality constraints. Composed by constraint 
/// components that inherits ConstraintComponentBase.
///
class Constraints {
public:
  ///
  /// @brief Default constructor. 
  ///
  Constraints();

  ///
  /// @brief Destructor. 
  ///
  ~Constraints();

  ///
  /// @brief Default copy constructor. 
  ///
  Constraints(const Constraints&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  Constraints& operator=(const Constraints&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  Constraints(Constraints&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  Constraints& operator=(Constraints&&) noexcept = default;

  ///
  /// @brief Append a constraint component to the cost function.
  /// @param[in] constraint shared pointer to the constraint component appended 
  /// to the constraints.
  ///
  void push_back(const std::shared_ptr<ConstraintComponentBase>& constraint);

  ///
  /// @brief Clear constraints by removing all components.
  ///
  void clear();

  ///
  /// @brief Check whether the constraints is empty or not.
  /// @return true if the constraints is empty. false if not.
  ///
  bool isEmpty() const;

  ///
  /// @brief Check if the constraints component requres kinematics of robot 
  /// model.
  /// @return true if the constraints component requres kinematics of 
  /// Robot model. false if not.
  ///
  bool useKinematics() const;

  ///
  /// @brief Creates ConstraintsData according to robot model and constraint 
  /// components. 
  /// @param[in] robot Robot model.
  /// @return Constraints data.
  ///
  ConstraintsData createConstraintsData(const Robot& robot) const;

  ///
  /// @brief Check whether the current solution s is feasible or not. 
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] s Split solution.
  /// @return true if s is feasible. false if not.
  ///
  bool isFeasible(Robot& robot, ConstraintsData& data,
                  const SplitSolution& s) const;

  ///
  /// @brief Set the slack and dual variables of each constraint components. 
  /// @param[in] robot Robot model.
  /// @param[out] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[in] s Split solution.
  ///
  void setSlackAndDual(Robot& robot, ConstraintsData& data, 
                       const double dtau, const SplitSolution& s) const;

  ///
  /// @brief Augment the dual residual of the constraints to the KKT residual 
  /// with respect to the configuration, velocity, acceleration, and contact 
  /// forces.
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[out] kkt_residual KKT residual.
  ///
  void augmentDualResidual(Robot& robot, ConstraintsData& data,
                           const double dtau, KKTResidual& kkt_residual) const;

  ///
  /// @brief Augment the dual residual of the constraints to the KKT residual
  /// with respect to the control input torques.
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[out] lu KKT residual with respect to the control input torques. 
  /// Size must be Robot::dimv().
  ///
  void augmentDualResidual(const Robot& robot, ConstraintsData& data,
                           const double dtau, Eigen::VectorXd& lu) const;

  ///
  /// @brief Consense slack and dual of the constraints and factorize condensed
  /// KKT Hessian and residual with respect to the configuration, velocity, 
  /// acceleration, and contact forces. 
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData(). residual and duality are also 
  /// computed.
  /// @param[in] dtau Time step.
  /// @param[in] s Split solution.
  /// @param[out] kkT_matrix The KKT matrix. The condensed Hessians are added  
  /// to this data.
  /// @param[out] kkt_residual KKT residual. The condensed residual are added 
  /// to this data.
  ///
  void condenseSlackAndDual(Robot& robot, ConstraintsData& data,
                            const double dtau, const SplitSolution& s,
                            KKTMatrix& kkt_matrix, 
                            KKTResidual& kkt_residual) const;

  ///
  /// @brief Consense slack and dual of the constraints and factorize condensed
  /// KKT Hessian and residual with respect to the configuration, velocity, 
  /// acceleration, and contact forces. 
  /// @param[in] robot Robot model.
  /// @param[in, out] data Constraints data generated by 
  /// Constraints::createConstraintsData(). residual and duality are also 
  /// computed.
  /// @param[in] dtau Time step.
  /// @param[in] u Control input torques.
  /// @param[out] Quu The KKT matrix with respect to the control input torques. 
  /// The condensed Hessians are added to this data. Size must be 
  /// Robot::dimv() x Robot::dimv().
  /// @param[out] lu KKT residual with respect to the control input torques. 
  /// The condensed residual are added to this data. Size must be Robot::dimv().
  ///
  void condenseSlackAndDual(const Robot& robot, ConstraintsData& data,
                            const double dtau, const Eigen::VectorXd& u,
                            Eigen::MatrixXd& Quu, Eigen::VectorXd& lu) const;

  ///
  /// @brief Compute directions of slack and dual.
  /// @param[in] robot Robot model.
  /// @param[in, out] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[in] d Split direction.
  ///
  void computeSlackAndDualDirection(Robot& robot, ConstraintsData& data, 
                                    const double dtau, 
                                    const SplitDirection& d) const;

  ///
  /// @brief Compute and returns the maximum step size by applying 
  /// fraction-to-boundary-rule to the direction of slack.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @return Maximum step size of the slack.
  ///
  double maxSlackStepSize(const ConstraintsData& data) const;

  ///
  /// @brief Compute and returns the maximum step size by applying 
  /// fraction-to-boundary-rule to the direction of dual.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @return Maximum step size of the dual.
  ///
  double maxDualStepSize(const ConstraintsData& data) const;

  ///
  /// @brief Updates the slack with step_size.
  /// @param[in, out] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] step_size Step size. 
  ///
  void updateSlack(ConstraintsData& data, const double step_size) const;

  ///
  /// @brief Updates the dual with step_size.
  /// @param[in, out] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] step_size Step size. 
  ///
  void updateDual(ConstraintsData& data, const double step_size) const;

  ///
  /// @brief Computes and returns the value of the barrier function for slack 
  /// variables.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @return Value of the barrier function. 
  ///
  double costSlackBarrier(const ConstraintsData& data) const;

  ///
  /// @brief Computes and returns the value of the barrier function for slack 
  /// variables with the step_size.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] step_size Step size. 
  /// @return Value of the barrier function. 
  ///
  double costSlackBarrier(const ConstraintsData& data, 
                          const double step_size) const;

  ///
  /// @brief Computes and return the L1-norm of the primal residual and duality
  /// of the constraints. 
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[in] s Split solution.
  /// @return L1 norm of the primal residual and duality of the constraints. 
  ///
  double residualL1Nrom(Robot& robot, ConstraintsData& data, 
                        const double dtau, const SplitSolution& s) const;

  ///
  /// @brief Computes and return the squared norm of the primal residual and 
  /// duality of the constraints. 
  /// @param[in] robot Robot model.
  /// @param[in] data Constraints data generated by 
  /// Constraints::createConstraintsData().
  /// @param[in] dtau Time step.
  /// @param[in] s Split solution.
  /// @return Squared norm of the primal residual and duality of the constraints. 
  ///
  double squaredKKTErrorNorm(Robot& robot, ConstraintsData& data, 
                             const double dtau, const SplitSolution& s) const;

private:
  std::vector<std::shared_ptr<ConstraintComponentBase>> constraints_;

};

} // namespace idocp

#include "idocp/constraints/constraints.hxx"

#endif // IDOCP_CONSTRAINTS_HPP_