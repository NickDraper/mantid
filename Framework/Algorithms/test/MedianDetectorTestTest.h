#ifndef WBVMEDIANTESTTEST_H_
#define WBVMEDIANTESTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"
//#include "MantidDataHandling/LoadEmptyInstrument.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include <ios>
#include <string>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

const int THEMASKED(40);
const int SAVEDBYERRORBAR(143);
const int Nhist(144);

// these values must match the values in MedianDetectorTest.h
const double BAD_VAL(1.);
const double GOOD_VAL(0.);

class MedianDetectorTestTest : public CxxTest::TestSuite {
public:
  static MedianDetectorTestTest *createSuite() {
    return new MedianDetectorTestTest();
  }
  static void destroySuite(MedianDetectorTestTest *suite) { delete suite; }

  void testWorkspaceAndArray() {
    MedianDetectorTest alg;
    TS_ASSERT_EQUALS(alg.name(), "MedianDetectorTest");
    TS_ASSERT_EQUALS(alg.version(), 1);
    // The spectra were setup in the constructor and passed to our algorithm
    // through this function
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(runInit(alg)));

    alg.setProperty("SignificanceTest", 1.0);
    // These are realistic values that I just made up
    alg.setProperty("LowThreshold", 0.5);
    alg.setProperty("HighThreshold", 1.3333);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(
                                 "MedianDetectorTestOutput"));

    MatrixWorkspace_const_sptr input;
    input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_IWSName);
    TS_ASSERT(input);
    // TS_ASSERT(input->getInstrument()->isDetectorMasked(input->getSpectrum(THEMASKED)->getDetectorIDs()));

    MatrixWorkspace_sptr outputMat =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    ;
    TS_ASSERT(outputMat);
    TS_ASSERT_EQUALS(outputMat->YUnit(), "");

    // There are three outputs, a workspace (tested in the next code block), an
    // array (later, this test) and a file (next test)
    // were all the spectra output?
    const size_t numberOfSpectra = outputMat->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
    const int numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 82);
    // the numbers below are threshold values that were found by trial and error
    // running these tests
    const int firstGoodSpec = 36;
    const int lastGoodSpec = 95;
    for (int lHist = 0; lHist < Nhist; lHist++) {
      //      std::cout << "    " << lHist << " " <<
      //      outputMat->readY(lHist).front() << std::endl;
      double expected = BAD_VAL;
      if (lHist >= firstGoodSpec && lHist <= lastGoodSpec)
        expected = GOOD_VAL;
      if (lHist == THEMASKED)
        expected = BAD_VAL;
      else if (lHist == SAVEDBYERRORBAR)
        expected = GOOD_VAL;
      TS_ASSERT_EQUALS(outputMat->readY(lHist).front(), expected);
    }
  }

  void testSolidAngle() {
    Mantid::DataObjects::Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            5, 10, 1);

    for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
      ws->dataY(i)[0] =
          std::floor(1e9 * ws->getDetector(i)->solidAngle(V3D(0, 0, 0)));
    }
    AnalysisDataService::Instance().addOrReplace("MDTSolidAngle", ws);

    MedianDetectorTest alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    // Set the properties
    alg.setPropertyValue("InputWorkspace", "MDTSolidAngle");
    alg.setPropertyValue("OutputWorkspace", "MedianDetectorTestOutput");
    // set significance test to small
    alg.setProperty("SignificanceTest", 0.00001);
    alg.setProperty("CorrectForSolidAngle", false);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    int numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 200);

    alg.setProperty("CorrectForSolidAngle", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 0);
    AnalysisDataService::Instance().remove("MDTSolidAngle");
  }

  void testLevelsUp() {
    Mantid::DataObjects::Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            5, 10, 1);

    for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
      ws->dataY(i)[0] =
          std::floor(1e9 * ws->getDetector(i)->solidAngle(V3D(0, 0, 0)));
    }
    AnalysisDataService::Instance().addOrReplace("MDTLevelsUp", ws);

    MedianDetectorTest alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    // Set the properties
    alg.setPropertyValue("InputWorkspace", "MDTLevelsUp");
    alg.setPropertyValue("OutputWorkspace", "MedianDetectorTestOutput");
    // set significance test to small
    alg.setProperty("SignificanceTest", 0.00001);
    alg.setProperty("CorrectForSolidAngle", false);
    alg.setProperty("LevelsUp", "0");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    int numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 200);

    alg.setProperty("LevelsUp", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 0);
    AnalysisDataService::Instance().remove("MDTLevelsUp");
  }

  MedianDetectorTestTest() : m_IWSName("MedianDetectorTestInput") {
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", Nhist, 11, 10);
    m_2DWS = boost::dynamic_pointer_cast<Workspace2D>(space);
    const short specLength = 22;
    boost::shared_ptr<MantidVec> x(new MantidVec(specLength));
    for (int i = 0; i < specLength; ++i) {
      (*x)[i] = i * 1000;
    }
    // the data will be 21 random numbers
    double yArray[specLength - 1] = {0.2, 4, 50, 0.001, 0, 0,     0,
                                     1,   0, 15, 4,     0, 0.001, 2e-10,
                                     0,   8, 0,  1e-4,  1, 7,     11};
    m_YSum = 0;
    for (int i = 0; i < specLength - 1; i++) {
      m_YSum += yArray[i];
    }

    // most error values will be small so that they wont affect the tests
    boost::shared_ptr<MantidVec> smallErrors(
        new MantidVec(specLength - 1, 0.01 * m_YSum / specLength));
    // if the SignificanceTest property is set to one, knowing what happens in
    // the loop below, these errors will just make or break the tests
    boost::shared_ptr<MantidVec> almostBigEnough(
        new MantidVec(specLength - 1, 0));
    (*almostBigEnough)[0] = 0.9 * m_YSum * (0.5 * Nhist - 1);
    boost::shared_ptr<MantidVec> bigEnough(new MantidVec(specLength - 1, 0));
    (*bigEnough)[0] = 1.2 * m_YSum * (0.5 * Nhist);

    for (int j = 0; j < Nhist; ++j) {
      m_2DWS->setX(j, x);
      boost::shared_ptr<MantidVec> spectrum(new MantidVec);
      // the spectravalues will be multiples of the random numbers above
      for (int l = 0; l < specLength - 1; ++l) {
        spectrum->push_back(j * yArray[l]);
      }
      boost::shared_ptr<MantidVec> errors = smallErrors;
      if (j == Nhist - 2)
        errors = almostBigEnough;
      if (j == SAVEDBYERRORBAR)
        errors = bigEnough;

      m_2DWS->setData(j, spectrum, errors);
      // Just set the spectrum number to match the index
      m_2DWS->getSpectrum(j)->setSpectrumNo(j + 1);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(m_IWSName, space);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", m_IWSName);
    loader.execute();

    m_2DWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // mask the detector
    m_2DWS->maskWorkspaceIndex(THEMASKED);
  }

private:
  bool runInit(MedianDetectorTest &alg) {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    bool good = alg.isInitialized();

    // Set the properties
    alg.setPropertyValue("InputWorkspace", m_IWSName);
    alg.setPropertyValue("OutputWorkspace", "MedianDetectorTestOutput");
    return good;
  }

  std::string m_IWSName, m_OFileName;
  Workspace2D_sptr m_2DWS;
  double m_YSum;
};

#endif /*WBVMEDIANTESTTEST_H_*/