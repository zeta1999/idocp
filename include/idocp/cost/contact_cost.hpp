#ifndef IDOCP_CONTACT_COST_HPP_
#define IDOCP_CONTACT_COST_HPP_

#include "Eigen/Core"

#include "robot/robot.hpp"


namespace idocp {

class ContactCost {
public:
  ContactCost(const Robot& robot, const Eigen::VectorXd& f_weight);

  ContactCost(const Robot& robot, const Eigen::VectorXd& f_ref, 
              const Eigen::VectorXd& f_weight);

  ContactCost();

  // Use defalut copy constructor.
  ContactCost(const ContactCost&) = default;

  // Use defalut copy operator.
  ContactCost& operator=(const ContactCost&) = default;

  void set_f_ref(const Eigen::VectorXd& f_ref);

  void set_f_weight(const Eigen::VectorXd& f_weight);

  void setContactStatus(const Robot& robot);

  double l(const double dtau, const Eigen::VectorXd& f) const;

  void lf(const double dtau, const Eigen::VectorXd& f, 
          Eigen::VectorXd& lf) const;

  void lff(const double dtau, Eigen::MatrixXd& lff) const;

  void augment_lff(const double dtau, Eigen::MatrixXd& lff) const;

private:
  std::vector<bool> contact_status_;
  int max_point_contacts_, max_dimf_;
  Eigen::VectorXd f_ref_, f_weight_;

};

} // namespace idocp


#endif // IDOCP_CONTACT_COST_HPP_