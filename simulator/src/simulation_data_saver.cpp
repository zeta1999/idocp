#include "simulation_data_saver.hpp"

#include <assert.h>


namespace idocp {
namespace simulator {

SimulationDataSaver::SimulationDataSaver(const std::string& save_dir_path, 
                                         const std::string& save_file_name) 
  : q_file_(save_dir_path+"/"+save_file_name+"_q.dat"),
    v_file_(save_dir_path+"/"+save_file_name+"_v.dat"),
    tau_file_(save_dir_path+"/"+save_file_name+"_tau.dat"),
    KKT_error_file_(save_dir_path+"/"+save_file_name+"_KKT_error.dat") {
}


SimulationDataSaver::~SimulationDataSaver() {
  q_file_.close();
  v_file_.close();
  tau_file_.close();
  KKT_error_file_.close();
}

void SimulationDataSaver::save(const Eigen::VectorXd& q, 
                               const Eigen::VectorXd& v, 
                               const Eigen::VectorXd& tau, 
                               const double KKT_error) {
  assert(KKT_error >= 0);
  for (int i=0; i<q.size(); ++i) {
    q_file_ << q[i] << " ";
  }
  q_file_ << "\n";
  for (int i=0; i<v.size(); ++i) {
    v_file_ << v[i] << " ";
  }
  v_file_ << "\n";
  for (int i=0; i<tau.size(); ++i) {
    tau_file_ << tau[i] << " ";
  }
  tau_file_ << "\n";
  KKT_error_file_ << KKT_error << "\n";
}

} // namespace simulator
} // namespace idocp