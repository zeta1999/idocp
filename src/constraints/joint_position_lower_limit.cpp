#include "idocp/constraints/joint_position_lower_limit.hpp"

#include <assert.h>


namespace idocp {

JointPositionLowerLimit::JointPositionLowerLimit(
    const Robot& robot, const double barrier, 
    const double fraction_to_boundary_rate)
  : ConstraintComponentBase(barrier, fraction_to_boundary_rate),
    dimc_(robot.lowerJointPositionLimit().size()),
    dim_passive_(robot.dim_passive()),
    qmin_(robot.lowerJointPositionLimit()) {
}


JointPositionLowerLimit::JointPositionLowerLimit()
  : ConstraintComponentBase(),
    dimc_(0),
    dim_passive_(0),
    qmin_() {
}


JointPositionLowerLimit::~JointPositionLowerLimit() {
}


bool JointPositionLowerLimit::useKinematics() const {
  return false;
}


bool JointPositionLowerLimit::isFeasible(Robot& robot, 
                                         ConstraintComponentData& data, 
                                         const SplitSolution& s) const {
  for (int i=0; i<dimc_; ++i) {
    if (s.q.tail(dimc_).coeff(i) < qmin_.coeff(i)) {
      return false;
    }
  }
  return true;
}


void JointPositionLowerLimit::setSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  assert(dtau > 0);
  data.slack = dtau * (s.q.tail(dimc_)-qmin_);
  setSlackAndDualPositive(data);
}


void JointPositionLowerLimit::augmentDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTResidual& kkt_residual) const {
  kkt_residual.lq().tail(dimc_).noalias() -= dtau * data.dual;
}


void JointPositionLowerLimit::condenseSlackAndDual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, KKTMatrix& kkt_matrix, 
    KKTResidual& kkt_residual) const {
  kkt_matrix.Qqq().diagonal().tail(dimc_).array()
      += dtau * dtau * data.dual.array() / data.slack.array();
  computePrimalAndDualResidual(robot, data, dtau, s);
  kkt_residual.lq().tail(dimc_).array() 
      -= dtau * (data.dual.array()*data.residual.array()-data.duality.array()) 
              / data.slack.array();
}


void JointPositionLowerLimit::computeSlackAndDualDirection(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s, const SplitDirection& d) const {
  data.dslack = dtau * d.dq().tail(dimc_) - data.residual;
  computeDualDirection(data);
}


void JointPositionLowerLimit::computePrimalAndDualResidual(
    Robot& robot, ConstraintComponentData& data, const double dtau, 
    const SplitSolution& s) const {
  data.residual = dtau * (qmin_-s.q.tail(dimc_)) + data.slack;
  computeDuality(data);
}


int JointPositionLowerLimit::dimc() const {
  return dimc_;
}

} // namespace idocp