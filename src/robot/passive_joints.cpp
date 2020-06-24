#include "robot/passive_joints.hpp"

#include <assert.h>


namespace idocp {

PassiveJoints::PassiveJoints(const pinocchio::Model& model) 
  : passive_torque_indices_(),
    dimv_(model.nv) {
  int total_dim_torque = -1; // Note that joint 0 is always universe.
  for (const auto& joint : model.joints) {
    if (joint.shortname() == "JointModelFreeFlyer") {
      passive_torque_indices_.push_back(total_dim_torque);
      passive_torque_indices_.push_back(total_dim_torque+1);
      passive_torque_indices_.push_back(total_dim_torque+2);
      passive_torque_indices_.push_back(total_dim_torque+3);
      passive_torque_indices_.push_back(total_dim_torque+4);
      passive_torque_indices_.push_back(total_dim_torque+5);
      total_dim_torque += 6;
    }
    else {
      total_dim_torque += 1;
    }
  }
}


PassiveJoints::PassiveJoints() 
  : passive_torque_indices_() {
}


PassiveJoints::~PassiveJoints() {
}


void PassiveJoints::setPassiveTorques(Eigen::VectorXd& torques) const {
  assert(torques.size() == dimv_);
  for (int i=0; i<passive_torque_indices_.size(); ++i) {
    torques.coeffRef(i) = 0.0;
  }
}


void PassiveJoints::computePassiveConstraintViolation(
    const Eigen::VectorXd& torques, Eigen::VectorXd& violation) const {
  assert(torques.size() == dimv_);
  assert(violation.size() == passive_torque_indices_.size());
  for (int i=0; i<passive_torque_indices_.size(); ++i) {
    violation.coeffRef(i) = torques.coeff(passive_torque_indices_[i]);
  }
}


unsigned int PassiveJoints::dim_passive() const {
  return passive_torque_indices_.size();
}


std::vector<unsigned int> PassiveJoints::passive_torque_indices() const {
  return passive_torque_indices_;
}

} // namespace idocp