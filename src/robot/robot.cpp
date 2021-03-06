#include "idocp/robot/robot.hpp"

#include <limits>
#include <stdexcept>
#include <assert.h>


namespace idocp {

Robot::Robot(const std::string& path_to_urdf)
  : model_(),
    data_(model_),
    point_contacts_(),
    floating_base_(),
    fjoint_(),
    dimq_(0),
    dimv_(0),
    max_dimf_(0),
    dimpulse_dv_(),
    joint_effort_limit_(),
    joint_velocity_limit_(),
    lower_joint_position_limit_(),
    upper_joint_position_limit_() {
  pinocchio::urdf::buildModel(path_to_urdf, model_);
  data_ = pinocchio::Data(model_);
  fjoint_ = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  floating_base_ = FloatingBase(model_);
  dimq_ = model_.nq;
  dimv_ = model_.nv;
  dimpulse_dv_.resize(dimv_, dimv_);
  dimpulse_dv_.setZero();
  initializeJointLimits();
}


Robot::Robot(const std::string& path_to_urdf, 
             const std::vector<int>& contact_frames)
  : model_(),
    data_(model_),
    point_contacts_(),
    floating_base_(),
    fjoint_(),
    dimq_(0),
    dimv_(0),
    max_dimf_(0),
    dimpulse_dv_(),
    joint_effort_limit_(),
    joint_velocity_limit_(),
    lower_joint_position_limit_(),
    upper_joint_position_limit_() {
  pinocchio::urdf::buildModel(path_to_urdf, model_);
  data_ = pinocchio::Data(model_);
  for (int i=0; i<contact_frames.size(); ++i) {
    point_contacts_.push_back(PointContact(model_, contact_frames[i]));
    is_each_contact_active_.push_back(false);
  }
  max_dimf_ = 3 * point_contacts_.size();
  fjoint_ = pinocchio::container::aligned_vector<pinocchio::Force>(
                 model_.joints.size(), pinocchio::Force::Zero());
  floating_base_ = FloatingBase(model_);
  dimq_ = model_.nq;
  dimv_ = model_.nv;
  dimpulse_dv_.resize(dimv_, dimv_);
  dimpulse_dv_.setZero();
  initializeJointLimits();
}


Robot::Robot()
  : model_(),
    data_(model_),
    point_contacts_(),
    floating_base_(),
    fjoint_(),
    dimq_(0),
    dimv_(0),
    max_dimf_(0),
    dimpulse_dv_(),
    joint_effort_limit_(),
    joint_velocity_limit_(),
    lower_joint_position_limit_(),
    upper_joint_position_limit_() {
}


Robot::~Robot() {
}


void Robot::setJointEffortLimit(const Eigen::VectorXd& joint_effort_limit) {
  try {
    if (joint_effort_limit_.size() != joint_effort_limit.size()) {
      throw std::out_of_range("invalid size of joint_effort_limit");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  joint_effort_limit_ = joint_effort_limit;
}


void Robot::setJointVelocityLimit(const Eigen::VectorXd& joint_velocity_limit) {
  try {
    if (joint_velocity_limit_.size() != joint_velocity_limit.size()) {
      throw std::out_of_range("invalid size of joint_velocity_limit");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  joint_velocity_limit_ = joint_velocity_limit;
}


void Robot::setLowerJointPositionLimit(
    const Eigen::VectorXd& lower_joint_position_limit) {
  try {
    if (lower_joint_position_limit_.size() != lower_joint_position_limit.size()) {
      throw std::out_of_range("invalid size of lower_joint_position_limit");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  lower_joint_position_limit_ = lower_joint_position_limit;
}


void Robot::setUpperJointPositionLimit(
    const Eigen::VectorXd& upper_joint_position_limit) {
  try {
    if (upper_joint_position_limit_.size() != upper_joint_position_limit.size()) {
      throw std::out_of_range("invalid size of upper_joint_position_limit");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
  upper_joint_position_limit_ = upper_joint_position_limit;
}


void Robot::printRobotModel() const {
  std::cout << "---------- Print robot model ---------- " << std::endl;
  for (int i=0; i<model_.nframes; ++i) {
    std::cout << "Info of frame " << i << std::endl;
    std::cout << "name: " << model_.frames[i].name << std::endl;
    std::cout << "parent joint id: " << model_.frames[i].parent << "\n" 
              << std::endl;
  }
  std::cout << std::endl;
  for (int i=0; i<model_.njoints; ++i) {
    std::cout << "Info of joint " << i << std::endl;
    std::cout << "name: " << model_.names[i] << std::endl;
    std::cout << model_.joints[i] << std::endl;
  }
  std::cout << "effortLimit = [" << model_.effortLimit.transpose() << "]" 
            << std::endl;
  std::cout << "velocityLimit = [" << model_.velocityLimit.transpose() << "]"
            << std::endl;
  std::cout << "lowerPositionLimit = [" << model_.lowerPositionLimit.transpose() 
            << "]" << std::endl;
  std::cout << "upperPositionLimit = [" << model_.upperPositionLimit.transpose() 
            << "]" << std::endl;
  std::cout << "--------------------------------------- " << std::endl;
}

} // namespace idocp 