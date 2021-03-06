#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"
#include "Eigen/LU"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/riccati_matrix_inverter.hpp"
#include "idocp/ocp/riccati_gain.hpp"


namespace idocp {

class RiccatiRecursionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    std::random_device rnd;
    fixed_base_urdf_ = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf_ = "../urdf/anymal/anymal.urdf";
    fixed_base_robot_ = Robot(fixed_base_urdf_);
    floating_base_robot_ = Robot(floating_base_urdf_);
    dtau_ = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  virtual void TearDown() {
  }

  double dtau_;
  std::string fixed_base_urdf_, floating_base_urdf_;
  Robot fixed_base_robot_, floating_base_robot_;
};


TEST_F(RiccatiRecursionTest, fixed_base_without_contacts) {
  const int dimv = fixed_base_robot_.dimv();
  const int dimaf = fixed_base_robot_.dimv();
  const int dimf = 0;
  const int dimc = 0;
  Eigen::MatrixXd Qqa = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qva = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qaa = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::VectorXd la = Eigen::VectorXd::Random(dimv);
  Eigen::MatrixXd Kaq = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kav = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::VectorXd ka = Eigen::VectorXd::Zero(dimv);
  Qaa.triangularView<Eigen::StrictlyLower>() 
      = Qaa.transpose().triangularView<Eigen::StrictlyLower>();
  // Makes Qaa semi positive define
  Qaa = Qaa * Qaa.transpose();
  // Adds identity matrix to make Qaa sufficiently positive define
  Qaa.noalias() += Eigen::MatrixXd::Identity(dimv, dimv);
  while (Qaa.determinant() == 0) {
    Qaa = Eigen::MatrixXd::Random(dimv, dimv);
    Qaa.triangularView<Eigen::StrictlyLower>() 
        = Qaa.transpose().triangularView<Eigen::StrictlyLower>();
    Qaa = Qaa * Qaa.transpose();
    Qaa.noalias() += Eigen::MatrixXd::Identity(dimv, dimv);
  }
  RiccatiMatrixInverter inverter(fixed_base_robot_);
  Eigen::MatrixXd G = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  G = Qaa;
  Eigen::MatrixXd Caf = Eigen::MatrixXd::Zero(dimc, dimv+dimf);
  Eigen::MatrixXd Ginv = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  inverter.invert(G, Caf, Ginv);
  Eigen::MatrixXd Qafqv = Eigen::MatrixXd::Zero(dimv+dimf, 2*dimv);
  Qafqv.topLeftCorner(dimv, dimv) = Qqa.transpose();
  Qafqv.topRightCorner(dimv, dimv) = Qva.transpose();
  Eigen::MatrixXd Cqv = Eigen::MatrixXd::Zero(dimc, 2*dimv);
  RiccatiGain gain(fixed_base_robot_);
  gain.computeFeedbackGain(Ginv, Qafqv, Cqv);
  Eigen::VectorXd laf = Eigen::VectorXd::Zero(dimv);
  laf.head(dimv) = la;
  Eigen::VectorXd C = Eigen::VectorXd::Zero(dimc);
  gain.computeFeedforward(Ginv, laf, C);
  const Eigen::MatrixXd Qaa_inv = Qaa.inverse();
  const Eigen::MatrixXd Kaq_ref = - Qaa_inv * Qqa.transpose();
  const Eigen::MatrixXd Kav_ref = - Qaa_inv * Qva.transpose();
  const Eigen::VectorXd ka_ref = - Qaa_inv * la;
  EXPECT_TRUE(gain.Kaq().isApprox(Kaq_ref));
  EXPECT_TRUE(gain.Kav().isApprox(Kav_ref));
  EXPECT_TRUE(gain.ka().isApprox(ka_ref));
  std::cout << "Kaq error:" << std::endl;
  std::cout << gain.Kaq() - Kaq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kav error:" << std::endl;
  std::cout << gain.Kav() - Kav_ref << std::endl;
  std::cout << std::endl;
  std::cout << "ka error:" << std::endl;
  std::cout << gain.ka() - ka_ref << std::endl;
  std::cout << std::endl;
}


TEST_F(RiccatiRecursionTest, fixed_base_with_contacts) {
  std::vector<int> contact_frames = {18};
  fixed_base_robot_ = Robot(fixed_base_urdf_, contact_frames);
  ContactStatus contact_status(contact_frames.size());
  contact_status.setContactStatus({true});
  const int dimv = fixed_base_robot_.dimv();
  const int dimaf = fixed_base_robot_.dimv() + contact_status.dimf();
  const int dimf = contact_status.dimf();
  const int dimc = fixed_base_robot_.dim_passive() + contact_status.dimf();
  ASSERT_EQ(dimf, 3);
  ASSERT_EQ(dimc, 3);
  Eigen::MatrixXd gen_mat = Eigen::MatrixXd::Random(dimv+dimf, dimv+dimf);
  gen_mat.triangularView<Eigen::StrictlyLower>() 
      = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
  // Makes pos_mat semi positive define
  Eigen::MatrixXd pos_mat = gen_mat * gen_mat.transpose();
  // Adds identity matrix to make pos_mat sufficiently positive define
  pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv+dimf, dimv+dimf);
  while (pos_mat.determinant() == 0) {
    gen_mat = Eigen::MatrixXd::Random(dimv+dimf, dimv+dimf);
    gen_mat.triangularView<Eigen::StrictlyLower>() 
        = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
    pos_mat = gen_mat * gen_mat.transpose();
    pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv+dimf, dimv+dimf);
  }
  Eigen::MatrixXd Qqa = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qva = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qaa = pos_mat.block(0, 0, dimv, dimv);
  Eigen::MatrixXd Qqf = Eigen::MatrixXd::Random(dimv, dimf);
  Eigen::MatrixXd Qvf = Eigen::MatrixXd::Random(dimv, dimf);
  Eigen::MatrixXd Qaf = pos_mat.block(0, dimv, dimv, dimf);
  Eigen::MatrixXd Qff = pos_mat.block(dimv, dimv, dimf, dimf);
  Eigen::MatrixXd Cq = Eigen::MatrixXd::Random(dimf, dimv);
  Eigen::MatrixXd Cv = Eigen::MatrixXd::Random(dimf, dimv);
  Eigen::MatrixXd Ca = Eigen::MatrixXd::Random(dimf, dimv);
  Eigen::VectorXd la = Eigen::VectorXd::Random(dimv);
  Eigen::VectorXd lf = Eigen::VectorXd::Random(dimf);
  Eigen::VectorXd C_res = Eigen::VectorXd::Random(dimf);
  Eigen::MatrixXd Kaq = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kav = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kfq = Eigen::MatrixXd::Zero(dimf, dimv);
  Eigen::MatrixXd Kfv = Eigen::MatrixXd::Zero(dimf, dimv);
  Eigen::MatrixXd Kmuq = Eigen::MatrixXd::Zero(dimf, dimv);
  Eigen::MatrixXd Kmuv = Eigen::MatrixXd::Zero(dimf, dimv);
  Eigen::VectorXd ka = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd kf = Eigen::VectorXd::Zero(dimf);
  Eigen::VectorXd kmu = Eigen::VectorXd::Zero(dimf);
  Eigen::MatrixXd M = Eigen::MatrixXd::Zero(dimv+2*dimf, dimv+2*dimf);
  M.block(0, 0, dimv+dimf, dimv+dimf) = pos_mat;
  M.block(0, dimv+dimf, dimv, dimf) = Ca.transpose();
  M.block(dimv+dimf, 0, dimf, dimv) = Ca;
  M.triangularView<Eigen::StrictlyLower>() 
      = M.transpose().triangularView<Eigen::StrictlyLower>();
  const Eigen::MatrixXd Minv = M.inverse();
  const Eigen::MatrixXd Kaq_ref =  - Minv.block(        0,         0, dimv, dimv) * Qqa.transpose()
                                   - Minv.block(        0,      dimv, dimv, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kav_ref =  - Minv.block(        0,         0, dimv, dimv) * Qva.transpose()
                                   - Minv.block(        0,      dimv, dimv, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * Cv.topRows(dimc);
  const Eigen::MatrixXd Kfq_ref =  - Minv.block(     dimv,         0, dimf, dimv) * Qqa.transpose()
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kfv_ref =  - Minv.block(     dimv,         0, dimf, dimv) * Qva.transpose()
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * Cv.topRows(dimc);
  const Eigen::MatrixXd Kmuq_ref = - Minv.block(dimv+dimf,         0, dimc, dimv) * Qqa.transpose()
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kmuv_ref = - Minv.block(dimv+dimf,         0, dimc, dimv) * Qva.transpose()
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * Cv.topRows(dimc);
  const Eigen::VectorXd ka_ref =   - Minv.block(        0,         0, dimv, dimv) * la
                                   - Minv.block(        0,      dimv, dimv, dimf) * lf.head(dimf)
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * C_res.head(dimc);
  const Eigen::VectorXd kf_ref =   - Minv.block(     dimv,         0, dimf, dimv) * la
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * lf.head(dimf)
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * C_res.head(dimc);
  const Eigen::VectorXd kmu_ref =  - Minv.block(dimv+dimf,         0, dimc, dimv) * la
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * lf.head(dimf)
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * C_res.head(dimc);
  RiccatiMatrixInverter inverter(fixed_base_robot_);
  inverter.setContactStatus(contact_status);
  Eigen::MatrixXd G = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  G = M.topLeftCorner(dimv+dimf, dimv+dimf);
  Eigen::MatrixXd Caf = Eigen::MatrixXd::Zero(dimc, dimv+dimf);
  Caf.leftCols(dimv) = Ca;
  Eigen::MatrixXd Ginv = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  inverter.invert(G, Caf, Ginv);
  Eigen::MatrixXd Qafqv = Eigen::MatrixXd::Zero(dimv+dimf, 2*dimv);
  Qafqv.topLeftCorner(dimv, dimv) = Qqa.transpose();
  Qafqv.topRightCorner(dimv, dimv) = Qva.transpose();
  Qafqv.bottomLeftCorner(dimf, dimv) = Qqf.transpose();
  Qafqv.bottomRightCorner(dimf, dimv) = Qvf.transpose();
  Eigen::MatrixXd Cqv = Eigen::MatrixXd::Zero(dimc, 2*dimv);
  Cqv.leftCols(dimv) = Cq;
  Cqv.rightCols(dimv) = Cv;
  RiccatiGain gain(fixed_base_robot_);
  gain.setContactStatus(contact_status);
  gain.computeFeedbackGain(Ginv, Qafqv, Cqv);
  Eigen::VectorXd laf = Eigen::VectorXd::Zero(dimv+dimf);
  laf.head(dimv) = la;
  laf.tail(dimf) = lf;
  Eigen::VectorXd C = C_res;
  gain.computeFeedforward(Ginv, laf, C);
  EXPECT_TRUE(gain.Kaq().isApprox(Kaq_ref));
  EXPECT_TRUE(gain.Kav().isApprox(Kav_ref));
  EXPECT_TRUE(gain.Kfq().isApprox(Kfq_ref));
  EXPECT_TRUE(gain.Kfv().isApprox(Kfv_ref));
  EXPECT_TRUE(gain.Kmuq().isApprox(Kmuq_ref));
  EXPECT_TRUE(gain.Kmuv().isApprox(Kmuv_ref));
  EXPECT_TRUE(gain.ka().isApprox(ka_ref));
  EXPECT_TRUE(gain.kf().isApprox(kf_ref));
  EXPECT_TRUE(gain.kmu().isApprox(kmu_ref));
  std::cout << "Kaq error:" << std::endl;
  std::cout << gain.Kaq() - Kaq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kav error:" << std::endl;
  std::cout << gain.Kav() - Kav_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kfq error:" << std::endl;
  std::cout << gain.Kfq() - Kfq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kfv error:" << std::endl;
  std::cout << gain.Kfv() - Kfv_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuq error:" << std::endl;
  std::cout << gain.Kmuq() - Kmuq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuv error:" << std::endl;
  std::cout << gain.Kmuv() - Kmuv_ref << std::endl;
  std::cout << std::endl;
  std::cout << "ka error:" << std::endl;
  std::cout << gain.ka() - ka_ref << std::endl;
  std::cout << std::endl;
  std::cout << "kf error:" << std::endl;
  std::cout << gain.kf() - kf_ref << std::endl;
  std::cout << std::endl;
  std::cout << "kmu error:" << std::endl;
  std::cout << gain.kmu() - kmu_ref << std::endl;
  std::cout << std::endl;
}


TEST_F(RiccatiRecursionTest, floating_base_without_contacts) {
  const int dimv = floating_base_robot_.dimv();
  const int dimaf = floating_base_robot_.dimv();
  const int dimf = 0;
  const int dimc = floating_base_robot_.dim_passive();
  ASSERT_EQ(dimf, 0);
  ASSERT_EQ(dimc, 6);
  Eigen::MatrixXd gen_mat = Eigen::MatrixXd::Random(dimv, dimv);
  gen_mat.triangularView<Eigen::StrictlyLower>() 
      = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
  // Makes pos_mat semi positive define
  Eigen::MatrixXd pos_mat = gen_mat * gen_mat.transpose();
  // Adds identity matrix to make pos_mat sufficiently positive define
  pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv, dimv);
  while (pos_mat.determinant() == 0) {
    gen_mat = Eigen::MatrixXd::Random(dimv, dimv);
    gen_mat.triangularView<Eigen::StrictlyLower>() 
        = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
    pos_mat = gen_mat * gen_mat.transpose();
    pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv, dimv);
  }
  Eigen::MatrixXd Qqa = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qva = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qaa = pos_mat.block(0, 0, dimv, dimv);
  Eigen::MatrixXd Cq = Eigen::MatrixXd::Random(dimc, dimv);
  Eigen::MatrixXd Cv = Eigen::MatrixXd::Random(dimc, dimv);
  Eigen::MatrixXd Ca = Eigen::MatrixXd::Random(dimc, dimv);
  Eigen::VectorXd la = Eigen::VectorXd::Random(dimv);
  Eigen::VectorXd C_res = Eigen::VectorXd::Random(dimc);
  Eigen::MatrixXd Kaq = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kav = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kmuq = Eigen::MatrixXd::Zero(dimc, dimv);
  Eigen::MatrixXd Kmuv = Eigen::MatrixXd::Zero(dimc, dimv);
  Eigen::VectorXd ka = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd kmu = Eigen::VectorXd::Zero(dimc);
  Eigen::MatrixXd M = Eigen::MatrixXd::Zero(dimv+dimc, dimv+dimc);
  M.block(0, 0, dimv, dimv) = pos_mat;
  M.block(0, dimv, dimv, dimc) = Ca.transpose();
  M.block(dimv, 0, dimc, dimv) = Ca;
  M.triangularView<Eigen::StrictlyLower>() 
      = M.transpose().triangularView<Eigen::StrictlyLower>();
  const Eigen::MatrixXd Minv = M.inverse();
  const Eigen::MatrixXd Kaq_ref =  - Minv.block(   0,    0, dimv, dimv) * Qqa.transpose()
                                   - Minv.block(   0, dimv, dimv, dimc) * Cq;
  const Eigen::MatrixXd Kav_ref =  - Minv.block(   0,    0, dimv, dimv) * Qva.transpose()
                                   - Minv.block(   0, dimv, dimv, dimc) * Cv;
  const Eigen::MatrixXd Kmuq_ref = - Minv.block(dimv,    0, dimc, dimv) * Qqa.transpose()
                                   - Minv.block(dimv, dimv, dimc, dimc) * Cq;
  const Eigen::MatrixXd Kmuv_ref = - Minv.block(dimv,    0, dimc, dimv) * Qva.transpose()
                                   - Minv.block(dimv, dimv, dimc, dimc) * Cv;
  const Eigen::VectorXd ka_ref =   - Minv.block(   0,    0, dimv, dimv) * la
                                   - Minv.block(   0, dimv, dimv, dimc) * C_res;
  const Eigen::VectorXd kmu_ref =  - Minv.block(dimv,    0, dimc, dimv) * la
                                   - Minv.block(dimv, dimv, dimc, dimc) * C_res;
  RiccatiMatrixInverter inverter(floating_base_robot_);
  Eigen::MatrixXd G = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  G = M.topLeftCorner(dimv+dimf, dimv+dimf);
  Eigen::MatrixXd Caf = Eigen::MatrixXd::Zero(dimc, dimv+dimf);
  Caf.leftCols(dimv) = Ca;
  Eigen::MatrixXd Ginv = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  inverter.invert(G, Caf, Ginv);
  Eigen::MatrixXd Qafqv = Eigen::MatrixXd::Zero(dimv+dimf, 2*dimv);
  Qafqv.topLeftCorner(dimv, dimv) = Qqa.transpose();
  Qafqv.topRightCorner(dimv, dimv) = Qva.transpose();
  Eigen::MatrixXd Cqv = Eigen::MatrixXd::Zero(dimc, 2*dimv);
  Cqv.leftCols(dimv) = Cq;
  Cqv.rightCols(dimv) = Cv;
  RiccatiGain gain(floating_base_robot_);
  gain.computeFeedbackGain(Ginv, Qafqv, Cqv);
  Eigen::VectorXd laf = Eigen::VectorXd::Zero(dimv+dimf);
  laf.head(dimv) = la;
  Eigen::VectorXd C = C_res;
  gain.computeFeedforward(Ginv, laf, C);
  EXPECT_TRUE(gain.Kaq().isApprox(Kaq_ref));
  EXPECT_TRUE(gain.Kav().isApprox(Kav_ref));
  EXPECT_TRUE(gain.Kmuq().isApprox(Kmuq_ref));
  EXPECT_TRUE(gain.Kmuv().isApprox(Kmuv_ref));
  EXPECT_TRUE(gain.ka().isApprox(ka_ref));
  EXPECT_TRUE(gain.kmu().isApprox(kmu_ref));
  std::cout << "Kaq error:" << std::endl;
  std::cout << gain.Kaq() - Kaq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kav error:" << std::endl;
  std::cout << gain.Kav() - Kav_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuq error:" << std::endl;
  std::cout << gain.Kmuq() - Kmuq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuv error:" << std::endl;
  std::cout << gain.Kmuv() - Kmuv_ref << std::endl;
  std::cout << std::endl;
  std::cout << "ka error:" << std::endl;
  std::cout << gain.ka() - ka_ref << std::endl;
  std::cout << std::endl;
  std::cout << "kmu error:" << std::endl;
  std::cout << gain.kmu() - kmu_ref << std::endl;
  std::cout << std::endl;
}


TEST_F(RiccatiRecursionTest, floating_base_with_contacts) {
  const std::vector<int> contact_frames = {14, 24, 34, 44};
  floating_base_robot_ = Robot(floating_base_urdf_, contact_frames);
  ContactStatus contact_status(contact_frames.size());
  std::vector<bool> active_contacts;
  std::random_device rnd;
  for (int i=0; i<contact_frames.size(); ++i) {
    active_contacts.push_back(rnd()%2==0);
  }
  contact_status.setContactStatus(active_contacts);
  const int dimv = floating_base_robot_.dimv();
  const int dimaf = floating_base_robot_.dimv() + contact_status.dimf();
  const int dimf = contact_status.dimf();
  const int dimc = floating_base_robot_.dim_passive() + contact_status.dimf();
  const int max_dimf = floating_base_robot_.max_dimf();
  const int max_dimc = max_dimf + floating_base_robot_.dim_passive();
  const int dim_passive = floating_base_robot_.dim_passive();
  Eigen::MatrixXd gen_mat = Eigen::MatrixXd::Random(dimv+max_dimf, dimv+max_dimf);
  gen_mat.triangularView<Eigen::StrictlyLower>() 
      = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
  // Makes pos_mat semi positive define
  Eigen::MatrixXd pos_mat = gen_mat * gen_mat.transpose();
  // Adds identity matrix to make pos_mat sufficiently positive define
  pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv+max_dimf, dimv+max_dimf);
  while (pos_mat.determinant() == 0) {
    gen_mat = Eigen::MatrixXd::Random(dimv+max_dimf, dimv+max_dimf);
    gen_mat.triangularView<Eigen::StrictlyLower>() 
        = gen_mat.transpose().triangularView<Eigen::StrictlyLower>();
    pos_mat = gen_mat * gen_mat.transpose();
    pos_mat.noalias() += Eigen::MatrixXd::Identity(dimv+max_dimf, dimv+max_dimf);
  }
  Eigen::MatrixXd Qqa = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qva = Eigen::MatrixXd::Random(dimv, dimv);
  Eigen::MatrixXd Qaa = pos_mat.block(0, 0, dimv, dimv);
  Eigen::MatrixXd Qqf = Eigen::MatrixXd::Random(dimv, max_dimf);
  Eigen::MatrixXd Qvf = Eigen::MatrixXd::Random(dimv, max_dimf);
  Eigen::MatrixXd Qaf = pos_mat.block(0, dimv, dimv, max_dimf);
  Eigen::MatrixXd Qff = pos_mat.block(dimv, dimv, max_dimf, max_dimf);
  Eigen::MatrixXd Cq = Eigen::MatrixXd::Random(max_dimc, dimv);
  Eigen::MatrixXd Cv = Eigen::MatrixXd::Random(max_dimc, dimv);
  Eigen::MatrixXd Ca = Eigen::MatrixXd::Random(max_dimc, dimv);
  Eigen::MatrixXd Cf = Eigen::MatrixXd::Random(dim_passive, max_dimf);
  Eigen::VectorXd la = Eigen::VectorXd::Random(dimv);
  Eigen::VectorXd lf = Eigen::VectorXd::Random(max_dimf);
  Eigen::VectorXd C_res = Eigen::VectorXd::Random(max_dimc);
  Eigen::MatrixXd Kaq = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kav = Eigen::MatrixXd::Zero(dimv, dimv);
  Eigen::MatrixXd Kfq = Eigen::MatrixXd::Zero(max_dimf, dimv);
  Eigen::MatrixXd Kfv = Eigen::MatrixXd::Zero(max_dimf, dimv);
  Eigen::MatrixXd Kmuq = Eigen::MatrixXd::Zero(max_dimc, dimv);
  Eigen::MatrixXd Kmuv = Eigen::MatrixXd::Zero(max_dimc, dimv);
  Eigen::VectorXd ka = Eigen::VectorXd::Zero(dimv);
  Eigen::VectorXd kf = Eigen::VectorXd::Zero(max_dimf);
  Eigen::VectorXd kmu = Eigen::VectorXd::Zero(max_dimc);
  Eigen::MatrixXd M = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  M.block(0, 0, dimv+dimf, dimv+dimf) = pos_mat.topLeftCorner(dimv+dimf, dimv+dimf);
  M.block(0, dimv+dimf, dimv, dimc) = Ca.topRows(dimc).transpose();
  Eigen::MatrixXd Cf_block = Eigen::MatrixXd::Zero(dimc, dimf);
  Cf_block.bottomRows(dim_passive) = Cf.leftCols(dimf);
  M.block(dimv, dimv+dimf, dimf, dimc) = Cf_block.transpose();
  M.triangularView<Eigen::StrictlyLower>() 
      = M.transpose().triangularView<Eigen::StrictlyLower>();
  const Eigen::MatrixXd Minv = M.inverse();
  const Eigen::MatrixXd Kaq_ref =  - Minv.block(        0,         0, dimv, dimv) * Qqa.transpose()
                                   - Minv.block(        0,      dimv, dimv, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kav_ref =  - Minv.block(        0,         0, dimv, dimv) * Qva.transpose()
                                   - Minv.block(        0,      dimv, dimv, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * Cv.topRows(dimc);
  const Eigen::MatrixXd Kfq_ref =  - Minv.block(     dimv,         0, dimf, dimv) * Qqa.transpose()
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kfv_ref =  - Minv.block(     dimv,         0, dimf, dimv) * Qva.transpose()
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * Cv.topRows(dimc);
  const Eigen::MatrixXd Kmuq_ref = - Minv.block(dimv+dimf,         0, dimc, dimv) * Qqa.transpose()
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * Qqf.leftCols(dimf).transpose()
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * Cq.topRows(dimc);
  const Eigen::MatrixXd Kmuv_ref = - Minv.block(dimv+dimf,         0, dimc, dimv) * Qva.transpose()
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * Qvf.leftCols(dimf).transpose()
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * Cv.topRows(dimc);
  const Eigen::VectorXd ka_ref =   - Minv.block(        0,         0, dimv, dimv) * la
                                   - Minv.block(        0,      dimv, dimv, dimf) * lf.head(dimf)
                                   - Minv.block(        0, dimv+dimf, dimv, dimc) * C_res.head(dimc);
  const Eigen::VectorXd kf_ref =   - Minv.block(     dimv,         0, dimf, dimv) * la
                                   - Minv.block(     dimv,      dimv, dimf, dimf) * lf.head(dimf)
                                   - Minv.block(     dimv, dimv+dimf, dimf, dimc) * C_res.head(dimc);
  const Eigen::VectorXd kmu_ref =  - Minv.block(dimv+dimf,         0, dimc, dimv) * la
                                   - Minv.block(dimv+dimf,      dimv, dimc, dimf) * lf.head(dimf)
                                   - Minv.block(dimv+dimf, dimv+dimf, dimc, dimc) * C_res.head(dimc);
  RiccatiMatrixInverter inverter(floating_base_robot_);
  inverter.setContactStatus(contact_status);
  Eigen::MatrixXd G = Eigen::MatrixXd::Zero(dimv+dimf, dimv+dimf);
  G = M.topLeftCorner(dimv+dimf, dimv+dimf);
  Eigen::MatrixXd Caf = Eigen::MatrixXd::Zero(dimc, dimv+dimf);
  Caf.leftCols(dimv) = Ca.topRows(dimc);
  Caf.rightCols(dimf) = Cf_block;
  Eigen::MatrixXd Ginv = Eigen::MatrixXd::Zero(dimv+dimf+dimc, dimv+dimf+dimc);
  inverter.invert(G, Caf, Ginv);
  Eigen::MatrixXd Qafqv = Eigen::MatrixXd::Zero(dimv+dimf, 2*dimv);
  Qafqv.topLeftCorner(dimv, dimv) = Qqa.transpose();
  Qafqv.topRightCorner(dimv, dimv) = Qva.transpose();
  Qafqv.bottomLeftCorner(dimf, dimv) = Qqf.leftCols(dimf).transpose();
  Qafqv.bottomRightCorner(dimf, dimv) = Qvf.leftCols(dimf).transpose();
  Eigen::MatrixXd Cqv = Eigen::MatrixXd::Zero(dimc, 2*dimv);
  Cqv.leftCols(dimv) = Cq.topRows(dimc);
  Cqv.rightCols(dimv) = Cv.topRows(dimc);
  RiccatiGain gain(floating_base_robot_);
  gain.setContactStatus(contact_status);
  gain.computeFeedbackGain(Ginv, Qafqv, Cqv);
  Eigen::VectorXd laf = Eigen::VectorXd::Zero(dimv+dimf);
  laf.head(dimv) = la;
  laf.tail(dimf) = lf.head(dimf);
  Eigen::VectorXd C = C_res.head(dimc);
  gain.computeFeedforward(Ginv, laf, C);
  EXPECT_TRUE(gain.Kaq().isApprox(Kaq_ref));
  EXPECT_TRUE(gain.Kav().isApprox(Kav_ref));
  EXPECT_TRUE(gain.Kfq().isApprox(Kfq_ref));
  EXPECT_TRUE(gain.Kfv().isApprox(Kfv_ref));
  EXPECT_TRUE(gain.Kmuq().isApprox(Kmuq_ref));
  EXPECT_TRUE(gain.Kmuv().isApprox(Kmuv_ref));
  EXPECT_TRUE(gain.ka().isApprox(ka_ref));
  EXPECT_TRUE(gain.kf().isApprox(kf_ref));
  EXPECT_TRUE(gain.kmu().isApprox(kmu_ref));
  std::cout << "Kaq error:" << std::endl;
  std::cout << gain.Kaq() - Kaq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kav error:" << std::endl;
  std::cout << gain.Kav() - Kav_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kfq error:" << std::endl;
  std::cout << gain.Kfq() - Kfq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kfv error:" << std::endl;
  std::cout << gain.Kfv() - Kfv_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuq error:" << std::endl;
  std::cout << gain.Kmuq() - Kmuq_ref << std::endl;
  std::cout << std::endl;
  std::cout << "Kmuv error:" << std::endl;
  std::cout << gain.Kmuv() - Kmuv_ref << std::endl;
  std::cout << std::endl;
  std::cout << "ka error:" << std::endl;
  std::cout << gain.ka() - ka_ref << std::endl;
  std::cout << std::endl;
  std::cout << "kf error:" << std::endl;
  std::cout << gain.kf() - kf_ref << std::endl;
  std::cout << std::endl;
  std::cout << "kmu error:" << std::endl;
  std::cout << gain.kmu() - kmu_ref << std::endl;
  std::cout << std::endl;
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}