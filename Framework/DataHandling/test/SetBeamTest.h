#ifndef MANTID_DATAHANDLING_SETBEAMTEST_H_
#define MANTID_DATAHANDLING_SETBEAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SetBeam.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataHandling::SetBeam;

class SetBeamTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetBeamTest *createSuite() { return new SetBeamTest(); }
  static void destroySuite(SetBeamTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Init() {
    SetBeam alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Beam_Size_Parameters_Stored_On_Instrument_Source() {
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    inputWS->setInstrument(testInst);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWS));
    alg->setProperty("Geometry", createRectangularBeamProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto source = testInst->getSource();
    const auto &instParams = inputWS->constInstrumentParameters();
    auto beamWidth = instParams.get(source->getComponentID(), "beam-width");
    TS_ASSERT(beamWidth);
    const double widthValue = beamWidth->value<double>();
    TS_ASSERT_DELTA(0.01, widthValue, 1e-10);
    auto beamHeight = instParams.get(source->getComponentID(), "beam-height");
    TS_ASSERT(beamHeight);
    const double heightValue = beamHeight->value<double>();
    TS_ASSERT_DELTA(0.0075, heightValue, 1e-10);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Workspace_Without_Instrument_Not_Accepted() {
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", inputWS),
                     std::invalid_argument);
  }

  void test_No_Geometry_Inputs_Not_Accepted() {
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    inputWS->setInstrument(testInst);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_Missing_Geometry_Inputs_Not_Accepted() {
    using Mantid::Kernel::PropertyManager;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    inputWS->setInstrument(testInst);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    auto props = boost::make_shared<PropertyManager>();
    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    props = createRectangularBeamProps();
    props->removeProperty("Width");
    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    props->removeProperty("Height");
    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::API::IAlgorithm_uptr createAlgorithm() {
    auto alg = Mantid::Kernel::make_unique<SetBeam>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    return std::move(alg);
  }

  Mantid::Kernel::PropertyManager_sptr createRectangularBeamProps() {
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Slit"), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Width", 1.0), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Height", 0.75), "");
    return props;
  }
};

#endif /* MANTID_DATAHANDLING_SETBEAMTEST_H_ */
