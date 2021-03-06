#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/make_cow.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VMD.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "PropertyManagerHelper.h"

#include <cxxtest/TestSuite.h>

#include <boost/make_shared.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <numeric>

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Declare into the factory.
DECLARE_WORKSPACE(WorkspaceTester)

/** Create a workspace with numSpectra, with
 * each spectrum having one detector, at id = workspace index.
 * @param numSpectra
 * @return
 */
boost::shared_ptr<MatrixWorkspace> makeWorkspaceWithDetectors(size_t numSpectra,
                                                              size_t numBins) {
  boost::shared_ptr<MatrixWorkspace> ws2 =
      boost::make_shared<WorkspaceTester>();
  ws2->initialize(numSpectra, numBins, numBins);

  auto inst = boost::make_shared<Instrument>("TestInstrument");
  ws2->setInstrument(inst);
  // We get a 1:1 map by default so the detector ID should match the spectrum
  // number
  for (size_t i = 0; i < ws2->getNumberHistograms(); ++i) {
    // Create a detector for each spectra
    Detector *det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
    inst->add(det);
    inst->markAsDetector(det);
    ws2->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
  }
  return ws2;
}

class MatrixWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixWorkspaceTest *createSuite() {
    return new MatrixWorkspaceTest();
  }
  static void destroySuite(MatrixWorkspaceTest *suite) { delete suite; }

  MatrixWorkspaceTest() : ws(boost::make_shared<WorkspaceTester>()) {
    ws->initialize(1, 1, 1);
  }

  void test_toString_Produces_Expected_Contents() {
    auto testWS = boost::make_shared<WorkspaceTester>();
    testWS->initialize(1, 2, 1);
    testWS->setTitle("A test run");
    testWS->getAxis(0)->setUnit("TOF");
    testWS->setYUnitLabel("Counts");

    std::string expected = "WorkspaceTester\n"
                           "Title: A test run\n"
                           "Histograms: 1\n"
                           "Bins: 1\n"
                           "Histogram\n"
                           "X axis: Time-of-flight / microsecond\n"
                           "Y axis: Counts\n"
                           "Distribution: False\n"
                           "Instrument: None\n"
                           "Run start: not available\n"
                           "Run end:  not available\n";

    TS_ASSERT_EQUALS(expected, testWS->toString());
  }

  void testGetSetTitle() {
    TS_ASSERT_EQUALS(ws->getTitle(), "");
    ws->setTitle("something");
    TS_ASSERT_EQUALS(ws->getTitle(), "something");
    ws->setTitle("");
  }

  void testGetSetComment() {
    TS_ASSERT_EQUALS(ws->getComment(), "");
    ws->setComment("commenting");
    TS_ASSERT_EQUALS(ws->getComment(), "commenting");
    ws->setComment("");
  }

  void test_getIndicesFromDetectorIDs() {
    WorkspaceTester ws;
    ws.initialize(10, 1, 1);
    for (size_t i = 0; i < 10; i++)
      ws.getSpectrum(i).setDetectorID(detid_t(i * 10));
    std::vector<detid_t> dets;
    dets.push_back(60);
    dets.push_back(20);
    dets.push_back(90);
    std::vector<size_t> indices = ws.getIndicesFromDetectorIDs(dets);
    TS_ASSERT_EQUALS(indices.size(), 3);
    TS_ASSERT_EQUALS(indices[0], 6);
    TS_ASSERT_EQUALS(indices[1], 2);
    TS_ASSERT_EQUALS(indices[2], 9);
  }

  void
  test_That_A_Workspace_Gets_SpectraMap_When_Initialized_With_NVector_Elements() {
    WorkspaceTester testWS;
    const size_t nhist(10);
    testWS.initialize(nhist, 1, 1);
    for (size_t i = 0; i < testWS.getNumberHistograms(); i++) {
      TS_ASSERT_EQUALS(testWS.getSpectrum(i).getSpectrumNo(), specnum_t(i + 1));
      TS_ASSERT(testWS.getSpectrum(i).hasDetectorID(detid_t(i)));
    }
  }

  void test_updateSpectraUsing() {
    WorkspaceTester testWS;
    testWS.initialize(3, 1, 1);

    specnum_t specs[] = {1, 2, 2, 3};
    detid_t detids[] = {10, 99, 20, 30};
    TS_ASSERT_THROWS_NOTHING(
        testWS.updateSpectraUsing(SpectrumDetectorMapping(specs, detids, 4)));

    TS_ASSERT(testWS.getSpectrum(0).hasDetectorID(10));
    TS_ASSERT(testWS.getSpectrum(1).hasDetectorID(20));
    TS_ASSERT(testWS.getSpectrum(1).hasDetectorID(99));
    TS_ASSERT(testWS.getSpectrum(2).hasDetectorID(30));
  }

  void testDetectorMappingCopiedWhenAWorkspaceIsCopied() {
    boost::shared_ptr<MatrixWorkspace> parent =
        boost::make_shared<WorkspaceTester>();
    parent->initialize(1, 1, 1);
    parent->getSpectrum(0).setSpectrumNo(99);
    parent->getSpectrum(0).setDetectorID(999);

    MatrixWorkspace_sptr copied = WorkspaceFactory::Instance().create(parent);
    // Has it been copied?
    TS_ASSERT_EQUALS(copied->getSpectrum(0).getSpectrumNo(), 99);
    TS_ASSERT(copied->getSpectrum(0).hasDetectorID(999));
  }

  void testGetMemorySize() { TS_ASSERT_THROWS_NOTHING(ws->getMemorySize()); }

  void testHistory() { TS_ASSERT_THROWS_NOTHING(ws->history()); }

  void testAxes() { TS_ASSERT_EQUALS(ws->axes(), 2); }

  void testGetAxis() {
    TS_ASSERT_THROWS(ws->getAxis(-1), Exception::IndexError);
    TS_ASSERT_THROWS_NOTHING(ws->getAxis(0));
    TS_ASSERT(ws->getAxis(0));
    TS_ASSERT(ws->getAxis(0)->isNumeric());
    TS_ASSERT_THROWS(ws->getAxis(2), Exception::IndexError);
  }

  void testReplaceAxis() {
    Axis *ax = new SpectraAxis(ws.get());
    TS_ASSERT_THROWS(ws->replaceAxis(2, ax), Exception::IndexError);
    TS_ASSERT_THROWS_NOTHING(ws->replaceAxis(0, ax));
    TS_ASSERT(ws->getAxis(0)->isSpectra());
  }

  void testIsDistribution() {
    TS_ASSERT(!ws->isDistribution());
    ws->setDistribution(true);
    TS_ASSERT(ws->isDistribution());
  }

  void testGetSetYUnit() {
    TS_ASSERT_EQUALS(ws->YUnit(), "");
    TS_ASSERT_THROWS_NOTHING(ws->setYUnit("something"));
    TS_ASSERT_EQUALS(ws->YUnit(), "something");
  }

  void testGetSpectrum() {
    WorkspaceTester ws;
    ws.initialize(4, 1, 1);
    TS_ASSERT_THROWS_NOTHING(ws.getSpectrum(0));
    TS_ASSERT_THROWS_NOTHING(ws.getSpectrum(3));
  }

  /** Get a detector sptr for each spectrum */
  void testGetDetector() {
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    boost::shared_ptr<MatrixWorkspace> workspace(
        makeWorkspaceWithDetectors(3, 1));

    // Initially un masked
    for (int i = 0; i < numHist; ++i) {
      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if (det) {
        TS_ASSERT_EQUALS(det->getID(), i);
      } else {
        TS_FAIL("No detector defined");
      }
    }

    // Now a detector group
    auto &spec = workspace->getSpectrum(0);
    spec.addDetectorID(1);
    spec.addDetectorID(2);
    IDetector_const_sptr det;
    TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(0));
    TS_ASSERT(det);

    // Now an empty (no detector) pixel
    auto &spec2 = workspace->getSpectrum(1);
    spec2.clearDetectorIDs();
    IDetector_const_sptr det2;
    TS_ASSERT_THROWS_ANYTHING(det2 = workspace->getDetector(1));
    TS_ASSERT(!det2);
  }

  void testWholeSpectraMasking() {
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    boost::shared_ptr<MatrixWorkspace> workspace(
        makeWorkspaceWithDetectors(3, 1));

    // Initially un masked
    for (int i = 0; i < numHist; ++i) {
      TS_ASSERT_EQUALS(workspace->readY(i)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], 1.0);

      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if (det) {
        TS_ASSERT_EQUALS(det->isMasked(), false);
      } else {
        TS_FAIL("No detector defined");
      }
    }

    // Mask a spectra
    workspace->maskWorkspaceIndex(1);
    workspace->maskWorkspaceIndex(2);

    for (int i = 0; i < numHist; ++i) {
      double expectedValue(0.0);
      bool expectedMasked(false);
      if (i == 0) {
        expectedValue = 1.0;
        expectedMasked = false;
      } else {
        expectedMasked = true;
      }
      TS_ASSERT_EQUALS(workspace->readY(i)[0], expectedValue);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], expectedValue);

      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if (det) {
        TS_ASSERT_EQUALS(det->isMasked(), expectedMasked);
      } else {
        TS_FAIL("No detector defined");
      }
    }
  }

  void testWholeSpectraMasking_SpectrumInfo() {
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    auto workspace = makeWorkspaceWithDetectors(numHist, 1);
    workspace->maskWorkspaceIndex(1);
    workspace->maskWorkspaceIndex(2);

    const auto &spectrumInfo = workspace->spectrumInfo();
    for (int i = 0; i < numHist; ++i) {
      bool expectedMasked(false);
      if (i == 0) {
        expectedMasked = false;
      } else {
        expectedMasked = true;
      }
      TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), expectedMasked);
    }
  }

  void test_spectrumInfo_works_unthreaded() {
    const int numHist(3);
    auto workspace = makeWorkspaceWithDetectors(numHist, 1);
    std::atomic<bool> parallelException{false};
    for (int i = 0; i < numHist; ++i) {
      try {
        static_cast<void>(workspace->spectrumInfo());
      } catch (...) {
        parallelException = true;
      }
    }
    TS_ASSERT(!parallelException);
  }

  void test_spectrumInfo_works_threaded() {
    const int numHist(3);
    auto workspace = makeWorkspaceWithDetectors(numHist, 1);
    std::vector<const SpectrumInfo *> spectrumInfos(numHist);
    std::atomic<bool> parallelException{false};
    PARALLEL_FOR_IF(Kernel::threadSafe(*workspace))
    for (int i = 0; i < numHist; ++i) {
      try {
        spectrumInfos[i] = &(workspace->spectrumInfo());
      } catch (...) {
        parallelException = true;
      }
    }
    TS_ASSERT(!parallelException);
    for (int i = 0; i < numHist; ++i)
      TS_ASSERT_EQUALS(spectrumInfos[0], spectrumInfos[i]);
  }

  void testFlagMasked() {
    auto ws = makeWorkspaceWithDetectors(2, 2);
    // Now do a valid masking
    TS_ASSERT_THROWS_NOTHING(ws->flagMasked(0, 1, 0.75));
    TS_ASSERT(ws->hasMaskedBins(0));
    TS_ASSERT_EQUALS(ws->maskedBins(0).size(), 1);
    TS_ASSERT_EQUALS(ws->maskedBins(0).begin()->first, 1);
    TS_ASSERT_EQUALS(ws->maskedBins(0).begin()->second, 0.75);
    // flagMasked() shouldn't change the y-value maskBins() tested below does
    // that
    TS_ASSERT_EQUALS(ws->dataY(0)[1], 1.0);

    // Now mask a bin earlier than above and check it's sorting properly
    TS_ASSERT_THROWS_NOTHING(ws->flagMasked(1, 1))
    TS_ASSERT_EQUALS(ws->maskedBins(1).size(), 1)
    TS_ASSERT_EQUALS(ws->maskedBins(1).begin()->first, 1)
    TS_ASSERT_EQUALS(ws->maskedBins(1).begin()->second, 1.0)
    // Check the previous masking is still OK
    TS_ASSERT_EQUALS(ws->maskedBins(0).rbegin()->first, 1)
    TS_ASSERT_EQUALS(ws->maskedBins(0).rbegin()->second, 0.75)
  }

  void testMasking() {
    auto ws2 = makeWorkspaceWithDetectors(1, 2);

    TS_ASSERT(!ws2->hasMaskedBins(0));
    // Doesn't throw on invalid spectrum number, just returns false
    TS_ASSERT(!ws2->hasMaskedBins(1));
    TS_ASSERT(!ws2->hasMaskedBins(-1));

    // Will throw if nothing masked for spectrum
    TS_ASSERT_THROWS(ws2->maskedBins(0), Mantid::Kernel::Exception::IndexError);
    // Will throw if attempting to mask invalid spectrum
    TS_ASSERT_THROWS(ws2->maskBin(-1, 1),
                     Mantid::Kernel::Exception::IndexError);
    TS_ASSERT_THROWS(ws2->maskBin(1, 1), Mantid::Kernel::Exception::IndexError);
    // ...or an invalid bin
    TS_ASSERT_THROWS(ws2->maskBin(0, -1),
                     Mantid::Kernel::Exception::IndexError);
    TS_ASSERT_THROWS(ws2->maskBin(0, 2), Mantid::Kernel::Exception::IndexError);

    // Now do a valid masking
    TS_ASSERT_THROWS_NOTHING(ws2->maskBin(0, 1, 0.5));
    TS_ASSERT(ws2->hasMaskedBins(0));
    TS_ASSERT_EQUALS(ws2->maskedBins(0).size(), 1);
    TS_ASSERT_EQUALS(ws2->maskedBins(0).begin()->first, 1);
    TS_ASSERT_EQUALS(ws2->maskedBins(0).begin()->second, 0.5);
    TS_ASSERT_EQUALS(ws2->dataY(0)[1], 0.5);

    // Now mask a bin earlier than above and check it's sorting properly
    TS_ASSERT_THROWS_NOTHING(ws2->maskBin(0, 0));
    TS_ASSERT_EQUALS(ws2->maskedBins(0).begin()->first, 0);
    TS_ASSERT_EQUALS(ws2->maskedBins(0).begin()->second, 1.0);
    TS_ASSERT_EQUALS(ws2->dataY(0)[0], 0.0);
    // Check the previous masking is still OK
    TS_ASSERT_EQUALS(ws2->maskedBins(0).rbegin()->first, 1);
    TS_ASSERT_EQUALS(ws2->maskedBins(0).rbegin()->second, 0.5);
    TS_ASSERT_EQUALS(ws2->dataY(0)[1], 0.5);
  }

  void testMaskingNaNInf() {
    const size_t s = 4;
    const double y[s] = {NAN, INFINITY, -INFINITY, 2.};
    WorkspaceTester ws;
    ws.initialize(1, s + 1, s);

    // initialize and mask first with 0 weights
    // masking with 0 weight should be equiavalent to flagMasked
    // i.e. values should not change, even Inf and NaN
    for (size_t i = 0; i < s; ++i) {
      ws.mutableY(0)[i] = y[i];
      ws.maskBin(0, i, 0);
    }

    TS_ASSERT(std::isnan(ws.y(0)[0]));
    TS_ASSERT(std::isinf(ws.y(0)[1]));
    TS_ASSERT(std::isinf(ws.y(0)[2]));
    TS_ASSERT_EQUALS(ws.y(0)[3], 2.);

    // now mask w/o specifying weight (e.g. 1 by default)
    // in this case everything should be 0, even NaN and Inf
    for (size_t i = 0; i < s; ++i) {
      ws.maskBin(0, i);
      TS_ASSERT_EQUALS(ws.y(0)[i], 0.);
    }
  }

  void testSize() {
    WorkspaceTester wkspace;
    wkspace.initialize(1, 4, 3);
    TS_ASSERT_EQUALS(wkspace.blocksize(), 3);
    TS_ASSERT_EQUALS(wkspace.size(), 3);
  }

  void testBinIndexOf() {
    WorkspaceTester wkspace;
    wkspace.initialize(1, 4, 3);
    // Data is all 1.0s
    wkspace.dataX(0)[1] = 2.0;
    wkspace.dataX(0)[2] = 3.0;
    wkspace.dataX(0)[3] = 4.0;

    TS_ASSERT_EQUALS(wkspace.getNumberHistograms(), 1);

    // First bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(1.3), 0);
    // Bin boundary
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.0), 0);
    // Mid range
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.5), 1);
    // Still second bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.001), 1);
    // Last bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(3.1), 2);
    // Last value
    TS_ASSERT_EQUALS(wkspace.binIndexOf(4.0), 2);

    // Error handling

    // Bad index value
    TS_ASSERT_THROWS(wkspace.binIndexOf(2.5, 1), std::out_of_range);
    TS_ASSERT_THROWS(wkspace.binIndexOf(2.5, -1), std::out_of_range);

    // Bad X values
    TS_ASSERT_THROWS(wkspace.binIndexOf(5.), std::out_of_range);
    TS_ASSERT_THROWS(wkspace.binIndexOf(0.), std::out_of_range);
  }

  void test_nexus_spectraMap() {
    NexusTestHelper th(true);
    th.createFile("MatrixWorkspaceTest.nxs");
    auto ws = makeWorkspaceWithDetectors(100, 50);
    std::vector<int> spec;
    for (int i = 0; i < 100; i++) {
      // Give some funny numbers, so it is not the default
      ws->getSpectrum(size_t(i)).setSpectrumNo(i * 11);
      ws->getSpectrum(size_t(i)).setDetectorID(99 - i);
      spec.push_back(i);
    }
    // Save that to the NXS file
    TS_ASSERT_THROWS_NOTHING(ws->saveSpectraMapNexus(th.file, spec););
  }

  /** Properly, this tests a method on Instrument, not MatrixWorkspace, but they
   * are related.
   */
  void test_isDetectorMasked() {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    Instrument_const_sptr inst = ws->getInstrument();
    // Make sure the instrument is parametrized so that the test is thorough
    TS_ASSERT(inst->isParametrized());
    TS_ASSERT(!inst->isDetectorMasked(1));
    TS_ASSERT(!inst->isDetectorMasked(19));
    // Mask then check that it returns as masked
    TS_ASSERT(ws->getSpectrum(19).hasDetectorID(19));
    ws->maskWorkspaceIndex(19);
    TS_ASSERT(inst->isDetectorMasked(19));
  }

  /** Check if any of a list of detectors are masked */
  void test_isDetectorMasked_onASet() {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    Instrument_const_sptr inst = ws->getInstrument();
    // Make sure the instrument is parametrized so that the test is thorough
    TS_ASSERT(inst->isParametrized());

    // Mask detector IDs 8 and 9
    ws->maskWorkspaceIndex(8);
    ws->maskWorkspaceIndex(9);

    std::set<detid_t> dets;
    TSM_ASSERT("No detector IDs = not masked", !inst->isDetectorMasked(dets));
    dets.insert(6);
    TSM_ASSERT("Detector is not masked", !inst->isDetectorMasked(dets));
    dets.insert(7);
    TSM_ASSERT("Detectors are not masked", !inst->isDetectorMasked(dets));
    dets.insert(8);
    TSM_ASSERT("If any detector is not masked, return false",
               !inst->isDetectorMasked(dets));
    // Start again
    dets.clear();
    dets.insert(8);
    TSM_ASSERT("If all detectors are not masked, return true",
               inst->isDetectorMasked(dets));
    dets.insert(9);
    TSM_ASSERT("If all detectors are not masked, return true",
               inst->isDetectorMasked(dets));
    dets.insert(10);
    TSM_ASSERT("If any detector is not masked, return false",
               !inst->isDetectorMasked(dets));
  }

  void test_hasGroupedDetectors() {
    auto ws = makeWorkspaceWithDetectors(5, 1);
    TS_ASSERT_EQUALS(ws->hasGroupedDetectors(), false);

    ws->getSpectrum(0).addDetectorID(3);
    TS_ASSERT_EQUALS(ws->hasGroupedDetectors(), true);
  }

  void test_getSpectrumToWorkspaceIndexMap() {
    WorkspaceTester ws;
    ws.initialize(2, 1, 1);
    auto map = ws.getSpectrumToWorkspaceIndexMap();
    TS_ASSERT_EQUALS(0, map[1]);
    TS_ASSERT_EQUALS(1, map[2]);
    TS_ASSERT_EQUALS(map.size(), 2);

    // Check it throws for non-spectra axis
    ws.replaceAxis(1, new NumericAxis(1));
    TS_ASSERT_THROWS(ws.getSpectrumToWorkspaceIndexMap(), std::runtime_error);
  }

  void test_getDetectorIDToWorkspaceIndexMap() {
    auto ws = makeWorkspaceWithDetectors(5, 1);
    detid2index_map idmap = ws->getDetectorIDToWorkspaceIndexMap(true);

    TS_ASSERT_EQUALS(idmap.size(), 5);
    int i = 0;
    for (auto it = idmap.begin(); it != idmap.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(idmap.count(i), 1);
      TS_ASSERT_EQUALS(idmap[i], i);
    }

    ws->getSpectrum(2).addDetectorID(99); // Set a second ID on one spectrum
    TS_ASSERT_THROWS(ws->getDetectorIDToWorkspaceIndexMap(true),
                     std::runtime_error);
    detid2index_map idmap2 = ws->getDetectorIDToWorkspaceIndexMap();
    TS_ASSERT_EQUALS(idmap2.size(), 6);
  }

  void test_getDetectorIDToWorkspaceIndexVector() {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    std::vector<size_t> out;
    detid_t offset = -1234;
    TS_ASSERT_THROWS_NOTHING(
        out = ws->getDetectorIDToWorkspaceIndexVector(offset));
    TS_ASSERT_EQUALS(offset, 0);
    TS_ASSERT_EQUALS(out.size(), 100);
    TS_ASSERT_EQUALS(out[0], 0);
    TS_ASSERT_EQUALS(out[1], 1);
    TS_ASSERT_EQUALS(out[99], 99);

    // Create some discontinuities and check that the default value is there
    // Have to create a whole new instrument to keep things consistent, since
    // the detector ID
    // is stored in at least 3 places
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    ws->setInstrument(inst);
    // We get a 1:1 map by default so the detector ID should match the spectrum
    // number
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      detid_t detid = static_cast<detid_t>(i);
      // Create a detector for each spectra
      if (i == 0)
        detid = -1;
      if (i == 99)
        detid = 110;
      Detector *det = new Detector("pixel", detid, inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i).addDetectorID(detid);
    }
    ws->getSpectrum(66).clearDetectorIDs();

    TS_ASSERT_THROWS_NOTHING(
        out = ws->getDetectorIDToWorkspaceIndexVector(offset));
    TS_ASSERT_EQUALS(offset, 1);
    TS_ASSERT_EQUALS(out.size(), 112);
    TS_ASSERT_EQUALS(out[66 + offset], std::numeric_limits<size_t>::max());
    TS_ASSERT_EQUALS(out[99 + offset], 99);
    TS_ASSERT_EQUALS(out[105 + offset], std::numeric_limits<size_t>::max());
    TS_ASSERT_EQUALS(out[110 + offset], 99);
  }

  void test_getSpectrumToWorkspaceIndexVector() {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    std::vector<size_t> out;
    detid_t offset = -1234;
    TS_ASSERT_THROWS_NOTHING(out =
                                 ws->getSpectrumToWorkspaceIndexVector(offset));
    TS_ASSERT_EQUALS(offset, -1);
    TS_ASSERT_EQUALS(out.size(), 100);
    TS_ASSERT_EQUALS(out[0], 0);
    TS_ASSERT_EQUALS(out[1], 1);
    TS_ASSERT_EQUALS(out[99], 99);
  }

  void test_getSignalAtCoord_histoData() {
    // Create a test workspace
    const auto ws = createTestWorkspace(4, 6, 5);

    // Get signal at coordinates
    std::vector<coord_t> coords = {0.5, 1.0};
    TS_ASSERT_DELTA(
        ws.getSignalAtCoord(coords.data(), Mantid::API::NoNormalization), 0.0,
        1e-5);
    coords[0] = 1.5;
    TS_ASSERT_DELTA(
        ws.getSignalAtCoord(coords.data(), Mantid::API::NoNormalization), 1.0,
        1e-5);
  }

  void test_getSignalAtCoord_pointData() {
    // Create a test workspace
    const auto ws = createTestWorkspace(4, 5, 5);

    // Get signal at coordinates
    std::vector<coord_t> coords = {0.0, 1.0};
    TS_ASSERT_DELTA(
        ws.getSignalAtCoord(coords.data(), Mantid::API::NoNormalization), 0.0,
        1e-5);
    coords[0] = 1.0;
    TS_ASSERT_DELTA(
        ws.getSignalAtCoord(coords.data(), Mantid::API::NoNormalization), 1.0,
        1e-5);
  }

  void test_getCoordAtSignal_regression() {
    /*
    Having more spectrum numbers (acutally vertical axis increments) than x bins
    in VolumeNormalisation mode
    should not cause any issues.
    */
    WorkspaceTester ws;
    const int nVertical = 4;

    const int nBins = 2;
    const int nYValues = 1;
    ws.initialize(nVertical, nBins, nYValues);
    NumericAxis *verticalAxis = new NumericAxis(nVertical);
    for (int i = 0; i < nVertical; ++i) {
      for (int j = 0; j < nBins; ++j) {
        if (j < nYValues) {
          ws.dataY(i)[j] = 1.0; // All y values are 1.
          ws.dataE(i)[j] = j;
        }
        ws.dataX(i)[j] = j; // x increments by 1
      }
      verticalAxis->setValue(i, double(i)); // Vertical axis increments by 1.
    }
    ws.replaceAxis(1, verticalAxis);
    // Signal is always 1 and volume of each box is 1. Therefore normalized
    // signal values by volume should always be 1.

    // Test at the top right.
    coord_t coord_top_right[2] = {static_cast<float>(ws.readX(0).back()),
                                  float(0)};
    signal_t value = 0;
    TS_ASSERT_THROWS_NOTHING(
        value = ws.getSignalAtCoord(coord_top_right, VolumeNormalization));
    TS_ASSERT_EQUALS(1.0, value);

    // Test at another location just to be sure.
    coord_t coord_bottom_left[2] = {
        static_cast<float>(ws.readX(nVertical - 1)[1]), float(nVertical - 1)};
    TS_ASSERT_THROWS_NOTHING(
        value = ws.getSignalAtCoord(coord_bottom_left, VolumeNormalization));
    TS_ASSERT_EQUALS(1.0, value);
  }

  void test_setMDMasking() {
    WorkspaceTester ws;
    TSM_ASSERT_THROWS("Characterisation test. This is not implemented.",
                      ws.setMDMasking(NULL), std::runtime_error);
  }

  void test_clearMDMasking() {
    WorkspaceTester ws;
    TSM_ASSERT_THROWS("Characterisation test. This is not implemented.",
                      ws.clearMDMasking(), std::runtime_error);
  }

  void test_getSpecialCoordinateSystem_default() {
    WorkspaceTester ws;
    TSM_ASSERT_EQUALS("Should default to no special coordinate system.",
                      Mantid::Kernel::None, ws.getSpecialCoordinateSystem());
  }

  void test_getFirstPulseTime_getLastPulseTime() {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    DateAndTime startTime("2013-04-21T10:40:00");
    proton_charge->addValue(startTime, 1.0E-7);
    proton_charge->addValue(startTime + 1.0, 2.0E-7);
    proton_charge->addValue(startTime + 2.0, 3.0E-7);
    proton_charge->addValue(startTime + 3.0, 4.0E-7);
    ws.mutableRun().addLogData(proton_charge);

    TS_ASSERT_EQUALS(ws.getFirstPulseTime(), startTime);
    TS_ASSERT_EQUALS(ws.getLastPulseTime(), startTime + 3.0);
  }

  void test_getFirstPulseTime_getLastPulseTime_SNS1990bug() {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    DateAndTime startTime("1990-12-31T23:59:00");
    proton_charge->addValue(startTime, 1.0E-7);
    proton_charge->addValue(startTime + 1.0, 2.0E-7);
    ws.mutableRun().addLogData(proton_charge);

    // If fewer than 100 entries (unlikely to happen in reality), you just get
    // back the last one
    TS_ASSERT_EQUALS(ws.getFirstPulseTime(), startTime + 1.0);

    for (int i = 2; i < 62; ++i) {
      proton_charge->addValue(startTime + static_cast<double>(i), 1.0E-7);
    }
    TS_ASSERT_EQUALS(ws.getFirstPulseTime(),
                     DateAndTime("1991-01-01T00:00:00"));
  }

  void
  test_getFirstPulseTime_getLastPulseTime_throws_if_protoncharge_missing_or_empty() {
    WorkspaceTester ws;
    TS_ASSERT_THROWS(ws.getFirstPulseTime(), std::runtime_error);
    TS_ASSERT_THROWS(ws.getLastPulseTime(), std::runtime_error);
    ws.mutableRun().addLogData(new TimeSeriesProperty<double>("proton_charge"));
    TS_ASSERT_THROWS(ws.getFirstPulseTime(), std::runtime_error);
    TS_ASSERT_THROWS(ws.getLastPulseTime(), std::runtime_error);
  }

  void
  test_getFirstPulseTime_getLastPulseTime_throws_if_protoncharge_wrong_type() {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<int>("proton_charge");
    proton_charge->addValue("2013-04-21T10:19:10", 1);
    proton_charge->addValue("2013-04-21T10:19:12", 2);
    ws.mutableRun().addLogData(proton_charge);
    TS_ASSERT_THROWS(ws.getFirstPulseTime(), std::invalid_argument);
    TS_ASSERT_THROWS(ws.getLastPulseTime(), std::invalid_argument);

    ws.mutableRun().addProperty(
        new PropertyWithValue<double>("proton_charge", 99.0), true);
    TS_ASSERT_THROWS(ws.getFirstPulseTime(), std::invalid_argument);
    TS_ASSERT_THROWS(ws.getLastPulseTime(), std::invalid_argument);
  }

  void test_getXMinMax() {
    double xmin, xmax;
    ws->getXMinMax(xmin, xmax);
    TS_ASSERT_EQUALS(xmin, 1.0);
    TS_ASSERT_EQUALS(xmax, 1.0);
    TS_ASSERT_EQUALS(ws->getXMin(), 1.0);
    TS_ASSERT_EQUALS(ws->getXMax(), 1.0);
  }

  void test_monitorWorkspace() {
    auto ws = boost::make_shared<WorkspaceTester>();
    TSM_ASSERT("There should be no monitor workspace by default",
               !ws->monitorWorkspace())

    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws->setMonitorWorkspace(ws2);
    TSM_ASSERT_EQUALS("Monitor workspace not successfully set",
                      ws->monitorWorkspace(), ws2)

    ws->setMonitorWorkspace(boost::shared_ptr<MatrixWorkspace>());
    TSM_ASSERT("Monitor workspace not successfully reset",
               !ws->monitorWorkspace())
  }

  void test_getXIndex() {
    WorkspaceTester ws;
    ws.init(1, 4, 3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    X[3] = 4.0;

    auto ip = ws.getXIndex(0, 0.0, true);
    TS_ASSERT_EQUALS(ip.first, 0);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 0.0, false);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 1.0, true);
    TS_ASSERT_EQUALS(ip.first, 0);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 1.0, false);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 5.0, true);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 5.0, false);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, true);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, false);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 5.0, true, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 5.0, false, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 3.0, true, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 3.0, false, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, true, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, false, 5);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, true, 4);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, false, 4);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, true, 3);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, false, 3);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, true);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 4.0, false);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 2.0, true, 3);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 2.0, false, 3);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 1.0, true, 3);
    TS_ASSERT_EQUALS(ip.first, 4);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 1.0, false, 3);
    TS_ASSERT_EQUALS(ip.first, 3);
    TS_ASSERT_DELTA(ip.second, 0.0, 1e-15);

    ip = ws.getXIndex(0, 2.1, true);
    TS_ASSERT_EQUALS(ip.first, 1);
    TS_ASSERT_DELTA(ip.second, 0.1, 1e-15);

    ip = ws.getXIndex(0, 2.1, false);
    TS_ASSERT_EQUALS(ip.first, 2);
    TS_ASSERT_DELTA(ip.second, 0.9, 1e-15);
  }

  void test_getImage_0_width() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop = 8;
    size_t width = 0;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    width = 3;
    TS_ASSERT_THROWS_NOTHING(ws.getImageY(start, stop, width));
  }

  void test_getImage_wrong_start() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 10;
    size_t stop = 8;
    size_t width = 3;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    start = 9;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    start = 0;
    TS_ASSERT_THROWS_NOTHING(ws.getImageY(start, stop, width));
  }

  void test_getImage_wrong_stop() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 0;
    size_t stop = 18;
    size_t width = 3;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    stop = 9;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    stop = 8;
    TS_ASSERT_THROWS_NOTHING(ws.getImageY(start, stop, width));
  }

  void test_getImage_empty_set() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 1;
    size_t stop = 0;
    size_t width = 1;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
    stop = 1;
    TS_ASSERT_THROWS_NOTHING(ws.getImageY(start, stop, width));
  }

  void test_getImage_non_rectangular() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 0;
    size_t stop = 7;
    size_t width = 3;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width), std::runtime_error);
  }

  void test_getImage_wrong_indexStart() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    double startX = 3;
    double endX = 4;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width, startX, endX),
                     std::runtime_error);

    WorkspaceTester wsh;
    wsh.init(9, 1, 1);
    startX = 2;
    endX = 2;
    TS_ASSERT_THROWS(wsh.getImageY(start, stop, width, startX, endX),
                     std::runtime_error);
  }

  void test_getImage_wrong_indexEnd() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    double startX = 1.0;
    double endX = 0.0;
    TS_ASSERT_THROWS(ws.getImageY(start, stop, width, startX, endX),
                     std::runtime_error);

    WorkspaceTester wsh;
    wsh.init(9, 2, 2);
    auto &X1 = ws.dataX(0);
    X1[0] = 1.0;
    X1[1] = 2.0;
    startX = 1.0;
    endX = 0.0;
    TS_ASSERT_THROWS(wsh.getImageY(start, stop, width, startX, endX),
                     std::runtime_error);
  }

  void test_getImage_single_bin_histo() {
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      ws.dataY(i)[0] = static_cast<double>(i + 1);
    }
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    double startX = 0;
    double endX = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING(
        image = ws.getImageY(start, stop, width, startX, endX));
    if (!image)
      return;
    TS_ASSERT_EQUALS(image->size(), 3);
    TS_ASSERT_EQUALS((*image)[0].size(), 3);
    TS_ASSERT_EQUALS((*image)[1].size(), 3);
    TS_ASSERT_EQUALS((*image)[2].size(), 3);

    TS_ASSERT_EQUALS((*image)[0][0], 1);
    TS_ASSERT_EQUALS((*image)[0][1], 2);
    TS_ASSERT_EQUALS((*image)[0][2], 3);
    TS_ASSERT_EQUALS((*image)[1][0], 4);
    TS_ASSERT_EQUALS((*image)[1][1], 5);
    TS_ASSERT_EQUALS((*image)[1][2], 6);
    TS_ASSERT_EQUALS((*image)[2][0], 7);
    TS_ASSERT_EQUALS((*image)[2][1], 8);
    TS_ASSERT_EQUALS((*image)[2][2], 9);
  }

  void test_getImage_single_bin_points() {
    WorkspaceTester ws;
    ws.init(9, 1, 1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      ws.dataY(i)[0] = static_cast<double>(i + 1);
    }
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    double startX = 1;
    double endX = 1;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING(
        image = ws.getImageY(start, stop, width, startX, endX));
    if (!image)
      return;
    TS_ASSERT_EQUALS(image->size(), 3);
    TS_ASSERT_EQUALS((*image)[0].size(), 3);
    TS_ASSERT_EQUALS((*image)[1].size(), 3);
    TS_ASSERT_EQUALS((*image)[2].size(), 3);

    TS_ASSERT_EQUALS((*image)[0][0], 1);
    TS_ASSERT_EQUALS((*image)[0][1], 2);
    TS_ASSERT_EQUALS((*image)[0][2], 3);
    TS_ASSERT_EQUALS((*image)[1][0], 4);
    TS_ASSERT_EQUALS((*image)[1][1], 5);
    TS_ASSERT_EQUALS((*image)[1][2], 6);
    TS_ASSERT_EQUALS((*image)[2][0], 7);
    TS_ASSERT_EQUALS((*image)[2][1], 8);
    TS_ASSERT_EQUALS((*image)[2][2], 9);
  }

  void test_getImage_multi_bin_histo() {
    WorkspaceTester ws;
    ws.init(9, 4, 3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    X[3] = 4.0;
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      ws.dataY(i)[0] = static_cast<double>(i + 1);
      ws.dataY(i)[1] = static_cast<double>(i + 2);
      ws.dataY(i)[2] = static_cast<double>(i + 3);
    }
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING(image = ws.getImageY(start, stop, width));
    if (!image)
      return;
    TS_ASSERT_EQUALS(image->size(), 3);
    TS_ASSERT_EQUALS((*image)[0].size(), 3);
    TS_ASSERT_EQUALS((*image)[1].size(), 3);
    TS_ASSERT_EQUALS((*image)[2].size(), 3);

    TS_ASSERT_EQUALS((*image)[0][0], 6);
    TS_ASSERT_EQUALS((*image)[0][1], 9);
    TS_ASSERT_EQUALS((*image)[0][2], 12);
    TS_ASSERT_EQUALS((*image)[1][0], 15);
    TS_ASSERT_EQUALS((*image)[1][1], 18);
    TS_ASSERT_EQUALS((*image)[1][2], 21);
    TS_ASSERT_EQUALS((*image)[2][0], 24);
    TS_ASSERT_EQUALS((*image)[2][1], 27);
    TS_ASSERT_EQUALS((*image)[2][2], 30);
  }

  void test_getImage_multi_bin_points() {
    WorkspaceTester ws;
    ws.init(9, 3, 3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      ws.dataY(i)[0] = static_cast<double>(i + 1);
      ws.dataY(i)[1] = static_cast<double>(i + 2);
      ws.dataY(i)[2] = static_cast<double>(i + 3);
    }
    const size_t start = 0;
    const size_t stop = 8;
    const size_t width = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING(image = ws.getImageY(start, stop, width));
    if (!image)
      return;
    TS_ASSERT_EQUALS(image->size(), 3);
    TS_ASSERT_EQUALS((*image)[0].size(), 3);
    TS_ASSERT_EQUALS((*image)[1].size(), 3);
    TS_ASSERT_EQUALS((*image)[2].size(), 3);

    TS_ASSERT_EQUALS((*image)[0][0], 6);
    TS_ASSERT_EQUALS((*image)[0][1], 9);
    TS_ASSERT_EQUALS((*image)[0][2], 12);
    TS_ASSERT_EQUALS((*image)[1][0], 15);
    TS_ASSERT_EQUALS((*image)[1][1], 18);
    TS_ASSERT_EQUALS((*image)[1][2], 21);
    TS_ASSERT_EQUALS((*image)[2][0], 24);
    TS_ASSERT_EQUALS((*image)[2][1], 27);
    TS_ASSERT_EQUALS((*image)[2][2], 30);
  }

  void test_setImage_too_large() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(2, 2, 1);
    TS_ASSERT_THROWS(ws.setImageY(*image), std::runtime_error);
  }

  void test_setImage_not_single_bin() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(20, 3, 2);
    TS_ASSERT_THROWS(ws.setImageY(*image), std::runtime_error);
  }

  void test_setImageY() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(6, 2, 1);
    TS_ASSERT_THROWS_NOTHING(ws.setImageY(*image));
    TS_ASSERT_EQUALS(ws.readY(0)[0], 1);
    TS_ASSERT_EQUALS(ws.readY(1)[0], 2);
    TS_ASSERT_EQUALS(ws.readY(2)[0], 3);
    TS_ASSERT_EQUALS(ws.readY(3)[0], 4);
    TS_ASSERT_EQUALS(ws.readY(4)[0], 5);
    TS_ASSERT_EQUALS(ws.readY(5)[0], 6);
  }

  void test_setImageE() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(6, 2, 1);
    TS_ASSERT_THROWS_NOTHING(ws.setImageE(*image));
    TS_ASSERT_EQUALS(ws.readE(0)[0], 1);
    TS_ASSERT_EQUALS(ws.readE(1)[0], 2);
    TS_ASSERT_EQUALS(ws.readE(2)[0], 3);
    TS_ASSERT_EQUALS(ws.readE(3)[0], 4);
    TS_ASSERT_EQUALS(ws.readE(4)[0], 5);
    TS_ASSERT_EQUALS(ws.readE(5)[0], 6);
  }

  void test_setImageY_start() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    TS_ASSERT_THROWS_NOTHING(ws.setImageY(*image, 3));
    TS_ASSERT_EQUALS(ws.readY(3)[0], 1);
    TS_ASSERT_EQUALS(ws.readY(4)[0], 2);
    TS_ASSERT_EQUALS(ws.readY(5)[0], 3);
    TS_ASSERT_EQUALS(ws.readY(6)[0], 4);
    TS_ASSERT_EQUALS(ws.readY(7)[0], 5);
    TS_ASSERT_EQUALS(ws.readY(8)[0], 6);
  }

  void test_setImageE_start() {
    auto image = createImage(2, 3);
    WorkspaceTester ws;
    ws.init(9, 2, 1);
    TS_ASSERT_THROWS_NOTHING(ws.setImageE(*image, 2));
    TS_ASSERT_EQUALS(ws.readE(2)[0], 1);
    TS_ASSERT_EQUALS(ws.readE(3)[0], 2);
    TS_ASSERT_EQUALS(ws.readE(4)[0], 3);
    TS_ASSERT_EQUALS(ws.readE(5)[0], 4);
    TS_ASSERT_EQUALS(ws.readE(6)[0], 5);
    TS_ASSERT_EQUALS(ws.readE(7)[0], 6);
  }

  /**
  * Test declaring an input workspace and retrieving as const_sptr or sptr
  */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    MatrixWorkspace_sptr wsInput = boost::make_shared<WorkspaceTester>();
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    MatrixWorkspace_const_sptr wsConst;
    MatrixWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<MatrixWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != NULL);
    TS_ASSERT_THROWS_NOTHING(
        wsNonConst = manager.getValue<MatrixWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != NULL);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    MatrixWorkspace_const_sptr wsCastConst;
    MatrixWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (MatrixWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != NULL);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (MatrixWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != NULL);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  void test_x_uncertainty_can_be_set() {
    // Arrange
    WorkspaceTester ws;
    const size_t numspec = 4;
    const size_t j = 3;
    const size_t k = j;
    ws.init(numspec, j, k);

    double values[3] = {10, 11, 17};
    size_t workspaceIndexWithDx[3] = {0, 1, 2};

    Mantid::MantidVec dxSpec0(j, values[0]);
    auto dxSpec1 =
        Kernel::make_cow<Mantid::HistogramData::HistogramDx>(j, values[1]);
    auto dxSpec2 = boost::make_shared<Mantid::HistogramData::HistogramDx>(
        Mantid::MantidVec(j, values[2]));

    // Act
    for (size_t spec = 0; spec < numspec; ++spec) {
      TSM_ASSERT("Should not have any x resolution values", !ws.hasDx(spec));
    }
    ws.dataDx(workspaceIndexWithDx[0]) = dxSpec0;
    ws.setSharedDx(workspaceIndexWithDx[1], dxSpec1);
    ws.setSharedDx(workspaceIndexWithDx[2], dxSpec2);

    // Assert
    auto compareValue =
        [&values](double data, size_t index) { return data == values[index]; };
    for (auto &index : workspaceIndexWithDx) {
      TSM_ASSERT("Should have x resolution values", ws.hasDx(index));
      TSM_ASSERT_EQUALS("Should have a length of 3", ws.dataDx(index).size(),
                        j);
      auto compareValueForSpecificWorkspaceIndex =
          std::bind(compareValue, std::placeholders::_1, index);

      auto &dataDx = ws.dataDx(index);
      TSM_ASSERT("dataDx should allow access to the spectrum",
                 std::all_of(std::begin(dataDx), std::end(dataDx),
                             compareValueForSpecificWorkspaceIndex));

      auto &readDx = ws.readDx(index);
      TSM_ASSERT("readDx should allow access to the spectrum",
                 std::all_of(std::begin(readDx), std::end(readDx),
                             compareValueForSpecificWorkspaceIndex));

      auto refDx = ws.sharedDx(index);
      TSM_ASSERT("readDx should allow access to the spectrum",
                 std::all_of(std::begin(*refDx), std::end(*refDx),
                             compareValueForSpecificWorkspaceIndex));
    }

    TSM_ASSERT("Should not have any x resolution values", !ws.hasDx(3));
  }

private:
  Mantid::API::MantidImage_sptr createImage(const size_t width,
                                            const size_t height) {
    auto image =
        boost::make_shared<Mantid::API::MantidImage>(height, MantidVec(width));
    double startingValue = 1.0;
    for (auto &row : *image) {
      std::iota(row.begin(), row.end(), startingValue);
      startingValue += static_cast<double>(width);
    }
    return image;
  }

  /**
   * Create a test workspace. Can be histo or points depending on x/yLength.
   * @param nVectors :: [input] Number of vectors
   * @param xLength :: [input] Length of X vector
   * @param yLength :: [input] Length of Y, E vectors
   * @returns :: workspace
   */
  WorkspaceTester createTestWorkspace(size_t nVectors, size_t xLength,
                                      size_t yLength) {
    WorkspaceTester ws;
    ws.initialize(nVectors, xLength, yLength);
    // X data
    std::vector<double> xData(xLength);
    std::iota(xData.begin(), xData.end(), 0.0);

    // Y data
    const auto yCounts = [&yLength](size_t wi) {
      std::vector<double> v(yLength);
      std::iota(v.begin(), v.end(), static_cast<double>(wi) * 10.0);
      return v;
    };

    // E data
    const auto errors = [&yLength](size_t wi) {
      std::vector<double> v(yLength);
      std::generate(v.begin(), v.end(), [&wi]() {
        return std::sqrt(static_cast<double>(wi) * 10.0);
      });
      return v;
    };

    for (size_t wi = 0; wi < nVectors; ++wi) {
      if (xLength == yLength) {
        ws.setPoints(wi, xData);
      } else if (xLength == yLength + 1) {
        ws.setBinEdges(wi, xData);
      } else {
        throw std::invalid_argument(
            "yLength must either be equal to xLength or xLength - 1");
      }
      ws.setCounts(wi, yCounts(wi));
      ws.setCountStandardDeviations(wi, errors(wi));
    }
    return ws;
  }

  boost::shared_ptr<MatrixWorkspace> ws;
};

class MatrixWorkspaceTestPerformance : public CxxTest::TestSuite {

public:
  static MatrixWorkspaceTestPerformance *createSuite() {
    return new MatrixWorkspaceTestPerformance();
  }
  static void destroySuite(MatrixWorkspaceTestPerformance *suite) {
    delete suite;
  }

  MatrixWorkspaceTestPerformance() : m_workspace() {
    using namespace Mantid::Geometry;

    size_t numberOfHistograms = 10000;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);

    Mantid::Kernel::V3D sourcePos(0, 0, 0);
    Mantid::Kernel::V3D samplePos(0, 0, 1);
    Mantid::Kernel::V3D trolley1Pos(0, 0, 3);
    Mantid::Kernel::V3D trolley2Pos(0, 0, 6);
    m_paramMap = boost::make_shared<Mantid::Geometry::ParameterMap>();

    auto baseInstrument = ComponentCreationHelper::sansInstrument(
        sourcePos, samplePos, trolley1Pos, trolley2Pos);

    auto sansInstrument =
        boost::make_shared<Instrument>(baseInstrument, m_paramMap);

    // See component creation helper for instrument definition
    m_sansBank = sansInstrument->getComponentByName("Bank1");

    numberOfHistograms = sansInstrument->getNumberDetectors();
    m_workspaceSans.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    m_workspaceSans.setInstrument(sansInstrument);
    m_workspaceSans.getAxis(0)->setUnit("TOF");
    m_workspaceSans.rebuildSpectraMapping();

    m_zRotation =
        Mantid::Kernel::Quat(180, V3D(0, 0, 1)); // rotate 180 degrees around z

    m_pos = Mantid::Kernel::V3D(1, 1, 1);
  }
  /// This test is equivalent to GeometryInfoFactoryTestPerformance, see there.
  void test_typical() {
    auto instrument = m_workspace.getInstrument();
    auto source = instrument->getSource();
    auto sample = instrument->getSample();
    auto L1 = source->getDistance(*sample);
    double result = 0.0;
    for (size_t i = 0; i < 10000; ++i) {
      auto detector = m_workspace.getDetector(i);
      result += L1;
      result += detector->getDistance(*sample);
      result += m_workspace.detectorTwoTheta(*detector);
    }
    // We are computing and using the result to fool the optimizer.
    TS_ASSERT_DELTA(result, 5214709.740869, 1e-6);
  }

  void test_calculateL2() {

    /*
     * Simulate the L2 calculation performed via the Workspace/Instrument
     * interface.
     */
    auto instrument = m_workspaceSans.getInstrument();
    auto sample = instrument->getSample();
    double l2 = 0;
    for (size_t i = 0; i < m_workspaceSans.getNumberHistograms(); ++i) {
      auto detector = m_workspaceSans.getDetector(i);
      l2 += detector->getDistance(*sample);
    }
    // Prevent optimization
    TS_ASSERT(l2 > 0);
  }

  void test_calculateL2_x10() {

    /*
     * Simulate the L2 calculation performed via the Workspace/Instrument
     * interface. Repeat several times to benchmark any caching/optmisation that
     * might be taken place in parameter maps.
     */
    auto instrument = m_workspaceSans.getInstrument();
    auto sample = instrument->getSample();
    double l2 = 0;
    int count = 0;
    while (count < 10) {
      for (size_t i = 0; i < m_workspaceSans.getNumberHistograms(); ++i) {
        auto detector = m_workspaceSans.getDetector(i);
        l2 += detector->getDistance(*sample);
      }
      ++count;
    }
    // Prevent optimization
    TS_ASSERT(l2 > 0);
  }

  /*
   * Rotate a bank in the workspace and read the positions out again. Very
   * typical.
   */
  void test_rotate_bank_and_read_positions_x10() {

    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    int count = 0;
    // Repeated execution to improve statistics and for comparison purposes with
    // future updates
    while (count < 10) {
      // Rotate the bank
      ComponentHelper::rotateComponent(
          *m_sansBank, *m_paramMap, m_zRotation,
          Mantid::Geometry::ComponentHelper::Relative);

      V3D pos;
      for (size_t i = 1; i < m_workspaceSans.getNumberHistograms(); ++i) {
        pos += m_workspaceSans.getDetector(i)->getPos();
      }
      ++count;
    }
  }

  /*
   * Move a bank in the workspace and read the positions out again. Very
   * typical.
   */
  void test_move_bank_and_read_positions_x10() {

    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    int count = 0;
    // Repeated execution to improve statistics and for comparison purposes with
    // future updates
    while (count < 10) {
      // move the bank
      ComponentHelper::moveComponent(
          *m_sansBank, *m_paramMap, m_pos,
          Mantid::Geometry::ComponentHelper::Relative);

      V3D pos;
      for (size_t i = 1; i < m_workspaceSans.getNumberHistograms(); ++i) {
        pos += m_workspaceSans.getDetector(i)->getPos();
      }
      ++count;
    }
  }

  // As test_rotate_bank_and_read_positions_x10 but based on SpectrumInfo.
  void test_rotate_bank_and_read_positions_SpectrumInfo_x10() {
    int count = 0;
    while (count < 10) {
      // Rotate the bank
      ComponentHelper::rotateComponent(
          *m_sansBank, *m_paramMap, m_zRotation,
          Mantid::Geometry::ComponentHelper::Relative);

      V3D pos;
      const auto &spectrumInfo = m_workspaceSans.spectrumInfo();
      for (size_t i = 1; i < m_workspaceSans.getNumberHistograms(); ++i) {
        pos += spectrumInfo.position(i);
      }
      ++count;
    }
  }

  // As test_move_bank_and_read_positions_x10 but based on SpectrumInfo.
  void test_move_bank_and_read_positions_SpectrumInfo_x10() {
    int count = 0;
    while (count < 10) {
      // move the bank
      ComponentHelper::moveComponent(
          *m_sansBank, *m_paramMap, m_pos,
          Mantid::Geometry::ComponentHelper::Relative);

      V3D pos;
      const auto &spectrumInfo = m_workspaceSans.spectrumInfo();
      for (size_t i = 1; i < m_workspaceSans.getNumberHistograms(); ++i) {
        pos += spectrumInfo.position(i);
      }
      ++count;
    }
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceSans;
  Mantid::Kernel::Quat m_zRotation;
  Mantid::Kernel::V3D m_pos;
  Mantid::Geometry::IComponent_const_sptr m_sansBank;
  boost::shared_ptr<Mantid::Geometry::ParameterMap> m_paramMap;
};

#endif /*WORKSPACETEST_H_*/
