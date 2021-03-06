#ifndef IDOCP_CONTACT_NORMAL_FORCE_HXX_ 
#define IDOCP_CONTACT_NORMAL_FORCE_HXX_

#include "idocp/contact_complementarity/contact_normal_force.hpp"

#include <exception>
#include <iostream>
#include <assert.h>


namespace idocp {

inline ContactNormalForce::ContactNormalForce(
   const Robot& robot, const double barrier, 
   const double fraction_to_boundary_rate)
  : ContactComplementarityComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.max_point_contacts()) {
}


inline ContactNormalForce::ContactNormalForce()
  : ContactComplementarityComponentBase(),
    dimc_(0) {
}


inline ContactNormalForce::~ContactNormalForce() {
}


inline bool ContactNormalForce::isFeasible_impl(Robot& robot, 
                                                ConstraintComponentData& data, 
                                                const SplitSolution& s) const {
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      if (s.f[i].coeff(2) < 0) {
        return false;
      }
    }
  }
  return true;
}


inline void ContactNormalForce::setSlackAndDual_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  assert(dtau > 0);
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    data.slack.coeffRef(i) = dtau * s.f[i].coeff(2);
  }
  setSlackAndDualPositive(data.slack, data.dual);
}


inline void ContactNormalForce::augmentDualResidual_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTResidual& kkt_residual) const {
  assert(dtau > 0);
  int dimf_stack = 0;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      kkt_residual.lf().coeffRef(dimf_stack+2) -= dtau * data.dual.coeff(i);
      dimf_stack += 3;
    }
  }
}


inline void ContactNormalForce::condenseSlackAndDual_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTMatrix& kkt_matrix, 
    KKTResidual& kkt_residual) const {
  assert(dtau > 0);
  int dimf_stack = 0;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      kkt_matrix.Qff().coeffRef(dimf_stack+2, dimf_stack+2)
          += dtau * dtau * data.dual.coeff(i) / data.slack.coeff(i);
      data.residual.coeffRef(i) = - dtau * s.f[i].coeff(2) + data.slack.coeff(i);
      data.duality.coeffRef(i) = computeDuality(data.slack.coeff(i), 
                                                data.dual.coeff(i));
      kkt_residual.lf().coeffRef(dimf_stack+2)
          -= dtau * (data.dual.coeff(i)*data.residual.coeff(i)-data.duality.coeff(i)) 
                  / data.slack.coeff(i);
      dimf_stack += 3;
    }
  }
}


inline void ContactNormalForce::computeSlackAndDualDirection_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, const SplitDirection& d) const {
  int dimf_stack = 0;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      data.dslack.coeffRef(i) 
          = dtau * d.df().coeff(dimf_stack+2) - data.residual.coeff(i);
      data.ddual.coeffRef(i) = computeDualDirection(data.slack.coeff(i), 
                                                    data.dual.coeff(i), 
                                                    data.dslack.coeff(i), 
                                                    data.duality.coeff(i));
      dimf_stack += 3;
    }
  }
}


inline double ContactNormalForce::residualL1Nrom_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  double norm = 0;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      norm += std::abs(data.slack.coeff(i) - dtau * s.f[i].coeff(2));
      norm += std::abs(computeDuality(data.slack.coeff(i), 
                                      data.dual.coeff(i)));
    }
  }
  return norm;
}


inline double ContactNormalForce::squaredKKTErrorNorm_impl(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  double norm = 0;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    if (robot.is_contact_active(i)) {
      const double residual = data.slack.coeff(i) - dtau * s.f[i].coeff(2);
      const double duality = computeDuality(data.slack.coeff(i), 
                                            data.dual.coeff(i));
      norm += residual * residual + duality * duality;
    }
  }
  return norm;
}


inline int ContactNormalForce::dimc_impl() const {
  return dimc_;
}


inline double ContactNormalForce::maxSlackStepSize_impl(
    const ConstraintComponentData& data,
    const std::vector<bool>& is_contact_active) const {
  double min_step_size = 1;
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      const double fraction_to_boundary 
          = fractionToBoundary(data.slack.coeff(i), data.dslack.coeff(i));
      if (fraction_to_boundary > 0 && fraction_to_boundary < 1) {
        if (fraction_to_boundary < min_step_size) {
          min_step_size = fraction_to_boundary;
        }
      }
    }
  }
  assert(min_step_size > 0);
  assert(min_step_size <= 1);
  return min_step_size;
}


inline double ContactNormalForce::maxDualStepSize_impl(
    const ConstraintComponentData& data,
    const std::vector<bool>& is_contact_active) const {
  double min_step_size = 1;
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      const double fraction_to_boundary 
          = fractionToBoundary(data.dual.coeff(i), data.ddual.coeff(i));
      if (fraction_to_boundary > 0 && fraction_to_boundary < 1) {
        if (fraction_to_boundary < min_step_size) {
          min_step_size = fraction_to_boundary;
        }
      }
    }
  }
  assert(min_step_size > 0);
  assert(min_step_size <= 1);
  return min_step_size;
}


inline void ContactNormalForce::updateSlack_impl(
    ConstraintComponentData& data, const std::vector<bool>& is_contact_active,
    const double step_size) const {
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      data.slack.coeffRef(i) += step_size * data.dslack.coeff(i);
    }
  }
}


inline void ContactNormalForce::updateDual_impl(
    ConstraintComponentData& data, const std::vector<bool>& is_contact_active,
    const double step_size) const {
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      data.dual.coeffRef(i) += step_size * data.ddual.coeff(i);
    }
  }
}


inline double ContactNormalForce::costSlackBarrier_impl(
    const ConstraintComponentData& data,
    const std::vector<bool>& is_contact_active) const {
  double cost = 0;
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      cost += costSlackBarrier(data.slack.coeff(i));
    }
  }
  return cost;
}


inline double ContactNormalForce::costSlackBarrier_impl(
    const ConstraintComponentData& data, 
    const std::vector<bool>& is_contact_active, const double step_size) const {
  double cost = 0;
  for (int i=0; i<dimc_; ++i) {
    if (is_contact_active[i]) {
      cost += costSlackBarrier(data.slack.coeff(i), data.dslack.coeff(i), 
                               step_size);
    }
  }
  return cost;
}

} // namespace idocp

#endif // IDOCP_CONTACT_NORMAL_FORCE_HXX_ 