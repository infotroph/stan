#include <string>
#include <boost/random/additive_combine.hpp>
#include <stan/io/dump.hpp>
#include <test/unit/mcmc/hmc/mock_hmc.hpp>
#include <stan/mcmc/hmc/hamiltonians/unit_e_metric.hpp>
#include <stan/interface_callbacks/writer/stream_writer.hpp>
#include <stan/interface_callbacks/writer/noop_writer.hpp>
#include <test/test-models/good/mcmc/hmc/hamiltonians/funnel.hpp>
#include <test/unit/util.hpp>
#include <gtest/gtest.h>

typedef boost::ecuyer1988 rng_t;

TEST(McmcUnitEMetric, sample_p) {
  rng_t base_rng(0);
  
  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());
  stan::mcmc::unit_e_metric<stan::mcmc::mock_model, rng_t> metric(model);
  stan::mcmc::unit_e_point z(q.size());
  
  int n_samples = 1000;
  double m = 0;
  double m2 = 0;
  
  for (int i = 0; i < n_samples; ++i) {
    metric.sample_p(z, base_rng);
    double T = metric.T(z);
    
    double delta = T - m;
    m += delta / static_cast<double>(i + 1);
    m2 += delta * (T - m);
  }
  
  double var = m2 / (n_samples + 1.0);
  
  // Mean within 5sigma of expected value (d / 2)
  EXPECT_EQ(true, fabs(m   - 0.5 * q.size()) < 5.0 * sqrt(var));
  
  // Variance within 10% of expected value (d / 2)
  EXPECT_EQ(true, fabs(var - 0.5 * q.size()) < 0.1 * q.size());
}

TEST(McmcUnitEMetric, gradients) {
  
  rng_t base_rng(0);
  
  Eigen::VectorXd q = Eigen::VectorXd::Ones(11);
  
  stan::mcmc::unit_e_point z(q.size());
  z.q = q;
  z.p.setOnes();
  
  std::fstream data_stream(std::string("").c_str(), std::fstream::in);
  stan::io::dump data_var_context(data_stream);
  data_stream.close();
  
  std::stringstream model_output, metric_output;
  stan::interface_callbacks::writer::stream_writer writer(metric_output);
  
  funnel_model_namespace::funnel_model model(data_var_context, &model_output);
  
  stan::mcmc::unit_e_metric<funnel_model_namespace::funnel_model, rng_t> metric(model);
  
  double epsilon = 1e-6;

  metric.update(z, writer);
  
  Eigen::VectorXd g1 = metric.dtau_dq(z);
  
  for (int i = 0; i < z.q.size(); ++i) {
    
    double delta = 0;
    
    z.q(i) += epsilon;
    metric.update(z, writer);
    delta += metric.tau(z);
    
    z.q(i) -= 2 * epsilon;
    metric.update(z, writer);
    delta -= metric.tau(z);
    
    z.q(i) += epsilon;
    metric.update(z, writer);
    
    delta /= 2 * epsilon;
    
    EXPECT_NEAR(delta, g1(i), epsilon);
    
  }
  
  Eigen::VectorXd g2 = metric.dtau_dp(z);
  
  for (int i = 0; i < z.q.size(); ++i) {
    
    double delta = 0;
    
    z.p(i) += epsilon;
    delta += metric.tau(z);
    
    z.p(i) -= 2 * epsilon;
    delta -= metric.tau(z);
    
    z.p(i) += epsilon;
    
    delta /= 2 * epsilon;
    
    EXPECT_NEAR(delta, g2(i), epsilon);
    
  }
  
  Eigen::VectorXd g3 = metric.dphi_dq(z);
  
  for (int i = 0; i < z.q.size(); ++i) {
    
    double delta = 0;
    
    z.q(i) += epsilon;
    metric.update(z, writer);
    delta += metric.phi(z);
    
    z.q(i) -= 2 * epsilon;
    metric.update(z, writer);
    delta -= metric.phi(z);
    
    z.q(i) += epsilon;
    metric.update(z, writer);
    
    delta /= 2 * epsilon;
    
    EXPECT_NEAR(delta, g3(i), epsilon);
    
  }


  EXPECT_EQ("", model_output.str());
  EXPECT_EQ("", metric_output.str());
}

TEST(McmcUnitEMetric, streams) {
  stan::test::capture_std_streams();
  rng_t base_rng(0);
  
  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;
  stan::mcmc::mock_model model(q.size());
  
  // for use in Google Test macros below
  typedef stan::mcmc::unit_e_metric<stan::mcmc::mock_model, rng_t> unit_e;

  EXPECT_NO_THROW(unit_e metric(model));
  
  stan::test::reset_std_streams();
  EXPECT_EQ("", stan::test::cout_ss.str());
  EXPECT_EQ("", stan::test::cerr_ss.str());
}
