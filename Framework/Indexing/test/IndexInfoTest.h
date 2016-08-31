#ifndef MANTID_INDEXING_INDEXINFOTEST_H_
#define MANTID_INDEXING_INDEXINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexInfo.h"

using namespace Mantid;
using namespace Indexing;

class IndexInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexInfoTest *createSuite() { return new IndexInfoTest(); }
  static void destroySuite(IndexInfoTest *suite) { delete suite; }

  void test_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo(3)); }

  void test_size() { TS_ASSERT_EQUALS(IndexInfo(3).size(), 3); }

  void test_setSpectrumNumbers_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setSpectrumNumbers({1, 2}), std::runtime_error);
  }

  void test_setDetectorIDs_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setDetectorIDs({1, 2}), std::runtime_error);
  }

  void test_setSpectrumNumbers() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumNumbers({3, 4, 5}));
    TS_ASSERT_EQUALS(t.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(t.spectrumNumber(1), 4);
    TS_ASSERT_EQUALS(t.spectrumNumber(2), 5);
  }

  void test_setDetectorIDs() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setDetectorIDs({6, 7, 8}));
    TS_ASSERT_EQUALS(t.detectorIDs(0), std::vector<detid_t>{6});
    TS_ASSERT_EQUALS(t.detectorIDs(1), std::vector<detid_t>{7});
    TS_ASSERT_EQUALS(t.detectorIDs(2), std::vector<detid_t>{8});
  }
};

#endif /* MANTID_INDEXING_INDEXINFOTEST_H_ */