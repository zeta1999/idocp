#ifndef IDOCP_DISTANCE_TO_CONTACT_SURFACE_HPP_
#define IDOCP_DISTANCE_TO_CONTACT_SURFACE_HPP_

#include <vector>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/contact_complementarity/contact_complementarity_component_base.hpp"
#include "idocp/constraints/constraint_component_data.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"


namespace idocp {

class DistanceToContactSurface final : public ContactComplementarityComponentBase<DistanceToContactSurface> {
public:
  DistanceToContactSurface(const Robot& robot, const double barrier=1.0e-04, 
                           const double fraction_to_boundary_rate=0.995);

  DistanceToContactSurface();

  ~DistanceToContactSurface();

  // Use default copy constructor.
  DistanceToContactSurface(const DistanceToContactSurface&) = default;

  // Use default copy coperator.
  DistanceToContactSurface& operator=(const DistanceToContactSurface&) = default;

  // Use default move constructor.
  DistanceToContactSurface(DistanceToContactSurface&&) noexcept = default;

  // Use default move assign coperator.
  DistanceToContactSurface& operator=(DistanceToContactSurface&&) noexcept = default;

  bool isFeasible_impl(Robot& robot, ConstraintComponentData& data, 
                       const SplitSolution& s) const;

  void setSlackAndDual_impl(Robot& robot, ConstraintComponentData& data, 
                            const double dtau, const SplitSolution& s) const;

  void augmentDualResidual_impl(Robot& robot, ConstraintComponentData& data, 
                                const double dtau, const SplitSolution& s,
                                KKTResidual& kkt_residual) const;

  void condenseSlackAndDual_impl(Robot& robot, ConstraintComponentData& data, 
                                 const double dtau, const SplitSolution& s,
                                 KKTMatrix& kkt_matrix,
                                 KKTResidual& kkt_residual) const;

  void computeSlackAndDualDirection_impl(Robot& robot, 
                                         ConstraintComponentData& data, 
                                         const double dtau, 
                                         const SplitSolution& s,
                                         const SplitDirection& d) const; 

  double residualL1Nrom_impl(Robot& robot, ConstraintComponentData& data, 
                             const double dtau, const SplitSolution& s) const;

  double squaredKKTErrorNorm_impl(Robot& robot, ConstraintComponentData& data, 
                                  const double dtau, 
                                  const SplitSolution& s) const;

  int dimc_impl() const;

  double maxSlackStepSize_impl(
      const ConstraintComponentData& data, 
      const std::vector<bool>& is_contact_active) const;

  double maxDualStepSize_impl(const ConstraintComponentData& data, 
                              const std::vector<bool>& is_contact_active) const;

  void updateSlack_impl(ConstraintComponentData& data, 
                        const std::vector<bool>& is_contact_active,
                        const double step_size) const;

  void updateDual_impl(ConstraintComponentData& data,
                       const std::vector<bool>& is_contact_active,
                       const double step_size) const;

  double costSlackBarrier_impl(
      const ConstraintComponentData& data, 
      const std::vector<bool>& is_contact_active) const;

  double costSlackBarrier_impl(const ConstraintComponentData& data, 
                               const std::vector<bool>& is_contact_active,
                               const double step_size) const;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  int dimc_;
  std::vector<double> distance_;
  std::vector<Eigen::Vector3d> frame_position_;
  std::vector<Eigen::MatrixXd> frame_derivative_;
  Eigen::MatrixXd distance_derivative_;
  Eigen::VectorXd dual_active_;

};

} // namespace idocp

#include "idocp/contact_complementarity/distance_to_contact_surface.hxx"

#endif // IDOCP_DISTANCE_TO_CONTACT_SURFACE_HPP_ 