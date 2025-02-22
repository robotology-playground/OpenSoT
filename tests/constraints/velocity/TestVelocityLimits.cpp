#include <gtest/gtest.h>
#include <OpenSoT/constraints/velocity/VelocityLimits.h>
#include <cmath>
#define  s 1.0
#define  vel_lim 20.0
#define  dT 0.001*s
#include "../../common.h"


using namespace OpenSoT::constraints::velocity;

namespace {

// The fixture for testing class Foo.
class testVelocityLimits : public TestBase {
 protected:

  // You can remove any or all of the following functions if its body
  // is empty.

  testVelocityLimits() : TestBase("coman_floating_base") {
    // You can do set-up work for each test here.
      zeros.resize(_model_ptr->getNv());
      zeros.setZero(_model_ptr->getNv());

      velocityLimits = new VelocityLimits( *_model_ptr, vel_lim,
                                            dT);

  }

  virtual ~testVelocityLimits() {
    // You can do clean-up work that doesn't throw exceptions here.
      if(velocityLimits != NULL) {
        delete velocityLimits;
        velocityLimits = NULL;
      }
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:


  // Objects declared here can be used by all tests in the test case for Foo.

  VelocityLimits* velocityLimits;

  Eigen::VectorXd zeros;
};

TEST_F(testVelocityLimits, sizesAreCorrect) {
    Eigen::VectorXd lowerBound = velocityLimits->getLowerBound();
    Eigen::VectorXd upperBound = velocityLimits->getUpperBound();

    EXPECT_EQ(_model_ptr->getNv(), lowerBound.size()) << "lowerBound should have size"
                                         << _model_ptr->getNv();
    EXPECT_EQ(_model_ptr->getNv(), upperBound.size()) << "upperBound should have size"
                                         << _model_ptr->getNv();

    EXPECT_EQ(0, velocityLimits->getAeq().rows()) << "Aeq should have size 0"
                                                  << "but has size"
                                                  << velocityLimits->getAeq().rows();

    EXPECT_EQ(0, velocityLimits->getbeq().size()) << "beq should have size 0"
                                                  << "but has size"
                                                  << velocityLimits->getbeq().size();

    EXPECT_EQ(0,velocityLimits->getAineq().rows()) << "Aineq should have size 0"
                                                   << "but has size"
                                                   << velocityLimits->getAeq().rows();

    EXPECT_EQ(0,velocityLimits->getbLowerBound().size()) << "beq should have size 0"
                                                         << "but has size"
                                                         << velocityLimits->getbLowerBound().size();

    EXPECT_EQ(0,velocityLimits->getbUpperBound().size()) << "beq should have size 0"
                                                         << "but has size"
                                                         << velocityLimits->getbUpperBound().size();
}


// Tests that the Foo::getLowerBounds() are zero at the bounds
TEST_F(testVelocityLimits, BoundsAreCorrect) {

    Eigen::VectorXd lowerBound = velocityLimits->getLowerBound();
    Eigen::VectorXd upperBound = velocityLimits->getUpperBound();
    /* checking a joint at upper bound */
    EXPECT_DOUBLE_EQ(-dT*vel_lim, lowerBound[0]) << "Lower Velocity Limits should be "
                                                  << -dT*vel_lim << ", "
                                                  <<  lowerBound[0] << " instead";
    EXPECT_DOUBLE_EQ(+dT*vel_lim, upperBound[0]) << "Upper Velocity Limits should be "
                                                  << +dT*vel_lim << ", "
                                                  << upperBound[0] << " instead";
}


}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
