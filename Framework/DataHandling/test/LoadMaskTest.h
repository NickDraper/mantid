#ifndef MANTID_DATAHANDLING_LOADMASKTEST_H_
#define MANTID_DATAHANDLING_LOADMASKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <sstream>

#include "MantidDataHandling/LoadMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadMaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskTest *createSuite() { return new LoadMaskTest(); }
  static void destroySuite(LoadMaskTest *suite) { delete suite; }

  void test_LoadXML() {
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try {
      TS_ASSERT_EQUALS(loadfile.execute(), true);
      DataObjects::MaskWorkspace_sptr maskws =
          AnalysisDataService::Instance()
              .retrieveWS<DataObjects::MaskWorkspace>("PG3Mask");
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
  } // test_LoadXML

  /*
   * By given a non-existing instrument's name, exception should be thrown.
   */
  void test_LoadXMLThrow() {
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "WhatEver");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try {
      TS_ASSERT_EQUALS(loadfile.execute(), false);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

  } // test_LoadXML

  /*
   * Test mask by detector ID
   * For VULCAN:
   * workspaceindex:  detector ID
   * 34           :   26284
   * 1000         :   27250
   * 2000         :   28268
   */
  void test_DetectorIDs() {
    // 1. Generate masking files
    std::vector<int> banks1;
    std::vector<int> detids;
    detids.push_back(26284);
    detids.push_back(27250);
    detids.push_back(28268);
    auto maskDetFile = genMaskingFile("maskingdet.xml", detids, banks1);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "VULCAN");
    loadfile.setProperty("InputFile", maskDetFile.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
            "VULCAN_Mask_Detectors");

    // 3. Check
    for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      double y = maskws->dataY(iws)[0];
      if (iws == 34 || iws == 1000 || iws == 2000) {
        // These 3 workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      }
    }
  }

  /*
   * Test mask by detector ID
   * For VULCAN:
   * workspaceindex:  detector ID  :  Spectrum No
   * 34           :   26284        :  35
   * 1000         :   27250        :  1001
   * 2000         :   28268        :  2001
   * 36-39                            37-40
   * 1001-1004                        1002-1005
   */
  void test_ISISFormat() {
    // 1. Generate masking files
    std::vector<specnum_t> singlespectra;
    singlespectra.push_back(35);
    singlespectra.push_back(1001);
    singlespectra.push_back(2001);
    std::vector<specnum_t> pairspectra;
    pairspectra.push_back(1002);
    pairspectra.push_back(1005);
    pairspectra.push_back(37);
    pairspectra.push_back(40);

    auto isisMaskFile =
        genISISMaskingFile("isismask.msk", singlespectra, pairspectra);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "VULCAN");
    loadfile.setProperty("InputFile", isisMaskFile.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
            "VULCAN_Mask_Detectors");

    // 3. Check
    size_t errorcounts = 0;
    for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      double y = maskws->dataY(iws)[0];
      if (iws == 34 || iws == 1000 || iws == 2000 || (iws >= 36 && iws <= 39) ||
          (iws >= 1001 && iws <= 1004)) {
        // All these workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
        if (fabs(y) > 1.0E-5) {
          errorcounts++;
          std::cout << "Workspace Index " << iws
                    << " has a wrong set on masks\n";
        }
      }
    }
    std::cout << "Total " << errorcounts << " errors \n";
  }

  void test_ISISWithRefWS() {
      auto ws_creator =
          AlgorithmManager::Instance().create("CreateSimulationWorkspace");
      ws_creator->initialize();
      ws_creator->setPropertyValue("Instrument", "MARI");
      ws_creator->setPropertyValue("BinParams","100,100,300");
      ws_creator->setPropertyValue("OutputWorkspace","testWS");
      ws_creator->setPropertyValue("UnitX", "TOF");

      ws_creator->execute();
      Workspace_sptr source = ws_creator->getProperty("OutputWorkspace");
      auto copier =
          AlgorithmManager::Instance().create("CloneWorkspace");
      copier->initialize();
      copier->setProperty("InputWorkspace", source);
      copier->setPropertyValue("OutputWorkspace","testWSClone");
      copier->execute();
      Workspace_sptr sample = copier->getProperty("OutputWorkspace");


      auto masker =
          AlgorithmManager::Instance().create("MaskDetectors");
      masker->initialize();
      masker->setProperty("Workspace", source);
      std::vector<int> masked_spectra(10);
      masked_spectra[0] = 10;  masked_spectra[1] = 11;  masked_spectra[2] = 13;
      masked_spectra[3] = 100; masked_spectra[4] = 110; masked_spectra[5] = 120;
      masked_spectra[6] = 130; masked_spectra[7] = 140; masked_spectra[8] = 200;
      masked_spectra[9] = 300;
      masker->setProperty("SpectraList", masked_spectra);
      masker->execute();

 
      auto exporter =
          AlgorithmManager::Instance().create("ExportSpectraMask");
      exporter->initialize();
      exporter->setProperty("Workspace", source);
      exporter->execute();

      // modify spectra-detector map on the workspace clone to check masking


      //      auto isisMaskFile =
      //    genISISMaskingFile("isismask.msk", singlespectra, pairspectra);

      //Load

      //// 2. Run
      //LoadMask loadfile;
      //loadfile.initialize();

      //loadfile.setProperty("Instrument", "VULCAN");
      //loadfile.setProperty("InputFile", isisMaskFile.getFileName());
      //loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

      //TS_ASSERT_EQUALS(loadfile.execute(), true);
      //DataObjects::MaskWorkspace_sptr maskws =
      //    AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
      //        "VULCAN_Mask_Detectors");

      //// 3. Check
      //size_t errorcounts = 0;
      //for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      //    double y = maskws->dataY(iws)[0];
      //    if (iws == 34 || iws == 1000 || iws == 2000 || (iws >= 36 && iws <= 39) ||
      //        (iws >= 1001 && iws <= 1004)) {
      //        // All these workspace index are masked
      //        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      //    }
      //    else {
      //        // Unmasked
      //        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      //        if (fabs(y) > 1.0E-5) {
      //            errorcounts++;
      //            std::cout << "Workspace Index " << iws
      //                << " has a wrong set on masks\n";
      //        }
      //    }
      //}
      //std::cout << "Total " << errorcounts << " errors \n";
  }


  /*
   * Load "testingmasking.xml" and "regionofinterest.xml"
   * These two xml files will generate two opposite Workspaces, i.e.,
   * Number(masked detectors of WS1) = Number(unmasked detectors of WS2)
   *
   * by BinaryOperation
   */
  void test_Banks() {
    // 0. Generate masking files
    std::vector<int> banks1;
    banks1.push_back(21);
    banks1.push_back(22);
    banks1.push_back(2200);
    std::vector<int> detids;
    auto maskFile1 = genMaskingFile("masking01.xml", detids, banks1);

    std::vector<int> banks2;
    banks2.push_back(23);
    banks2.push_back(26);
    banks2.push_back(27);
    banks2.push_back(28);
    auto maskFile2 = genMaskingFile("masking02.xml", detids, banks2);

    // 1. Generate Mask Workspace
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "VULCAN");
    loadfile.setProperty("InputFile", maskFile1.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask1");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
            "VULCAN_Mask1");

    // 2. Generate Region of Interest Workspace
    LoadMask loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "VULCAN");
    loadfile2.setProperty("InputFile", maskFile2.getFileName());
    loadfile2.setProperty("OutputWorkspace", "VULCAN_Mask2");

    TS_ASSERT_EQUALS(loadfile2.execute(), true);
    DataObjects::MaskWorkspace_sptr interestws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(
            "VULCAN_Mask2");

    // 3. Check
    size_t sizemask = maskws->getNumberHistograms();
    size_t sizeinterest = interestws->getNumberHistograms();
    TS_ASSERT(sizemask == sizeinterest);

    if (sizemask == sizeinterest) {
      // number1: number of masked detectors of maskws
      // number2: number of used detectors of interestws
      size_t number1 = 0;
      size_t number0 = 0;
      for (size_t ih = 0; ih < maskws->getNumberHistograms(); ih++) {
        bool v1 = maskws->isMaskedIndex(ih);
        bool v2 = interestws->isMaskedIndex(ih);
        if (v1) {
          number0++;
          TS_ASSERT(!v2);
        }
        if (!v2) {
          number1++;
          TS_ASSERT(v1);
        }
      }

      TS_ASSERT_EQUALS(number0 > 0, true);
      TS_ASSERT_EQUALS(number1 > 0, true);
      TS_ASSERT_EQUALS(number1 - number0, 0);
    }

    return;
  } // test_Openfile

  void testSpecifyInstByIDF() {
    const std::string oldEmuIdf = "EMU_Definition_32detectors.xml";
    const std::string newEmuIdf = "EMU_Definition_96detectors.xml";

    const std::vector<int> detIDs = {2, 10};
    auto maskFile = genMaskingFile("emu_mask.xml", detIDs, std::vector<int>());

    auto byInstName =
        loadMask("EMU", maskFile.getFileName(), "LoadedByInstName");
    auto withOldIDF =
        loadMask(oldEmuIdf, maskFile.getFileName(), "LoadWithOldIDF");
    auto withNewIDF =
        loadMask(newEmuIdf, maskFile.getFileName(), "LoadWithNewIDF");

    TS_ASSERT_EQUALS(byInstName->getNumberHistograms(), 96);
    TS_ASSERT_EQUALS(withOldIDF->getNumberHistograms(), 32);
    TS_ASSERT_EQUALS(withNewIDF->getNumberHistograms(), 96);

    TS_ASSERT(byInstName->isMasked(2));
    TS_ASSERT(withOldIDF->isMasked(2));
    TS_ASSERT(withNewIDF->isMasked(2));
    TS_ASSERT(byInstName->isMasked(10));
    TS_ASSERT(withOldIDF->isMasked(10));
    TS_ASSERT(withNewIDF->isMasked(10));

    TS_ASSERT_EQUALS(byInstName->getNumberMasked(), 2);
    TS_ASSERT_EQUALS(withOldIDF->getNumberMasked(), 2);
    TS_ASSERT_EQUALS(withNewIDF->getNumberMasked(), 2);
  }

  DataObjects::MaskWorkspace_sptr loadMask(const std::string &instrument,
                                           const std::string &inputFile,
                                           const std::string &outputWsName) {
    LoadMask loadMask;
    loadMask.initialize();

    loadMask.setProperty("Instrument", instrument);
    loadMask.setProperty("InputFile", inputFile);
    loadMask.setProperty("OutputWorkspace", outputWsName);

    TS_ASSERT_EQUALS(loadMask.execute(), true);

    return AnalysisDataService::Instance()
        .retrieveWS<DataObjects::MaskWorkspace>(outputWsName);
  }

  /*
   * Create a masking file
   */
  ScopedFileHelper::ScopedFile genMaskingFile(std::string maskfilename,
                                              std::vector<int> detids,
                                              std::vector<int> banks) {
    std::stringstream ss;

    // 1. Header
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    ss << "  <detector-masking>\n";
    ss << "    <group>\n";

    // 2. "detids" & component
    if (detids.size() > 0) {
      ss << "    <detids>";
      for (size_t i = 0; i < detids.size(); i++) {
        if (i < detids.size() - 1)
          ss << detids[i] << ",";
        else
          ss << detids[i];
      }
      ss << "</detids>\n";
    }

    for (size_t i = 0; i < banks.size(); i++) {
      ss << "<component>bank" << banks[i] << "</component>\n";
    }

    // 4. End of file
    ss << "  </group>\n</detector-masking>\n";

    return ScopedFileHelper::ScopedFile(ss.str(), maskfilename);
  }

  /*
   * Create an ISIS format masking file
   */
  ScopedFileHelper::ScopedFile
  genISISMaskingFile(std::string maskfilename,
                     std::vector<specnum_t> singlespectra,
                     std::vector<specnum_t> pairspectra) {
    std::stringstream ss;

    // 1. Single spectra
    for (size_t i = 0; i < singlespectra.size(); i++) {
      ss << singlespectra[i] << " ";
    }
    ss << '\n';

    // 2. Spectra pair
    // a) Make the list really has complete pairs
    if (pairspectra.size() % 2 == 1)
      pairspectra.pop_back();

    for (size_t i = 0; i < pairspectra.size(); i += 2) {
      ss << pairspectra[i] << "-" << pairspectra[i + 1] << "  ";
    }
    ss << '\n';

    return ScopedFileHelper::ScopedFile(ss.str(), maskfilename);
  }
};

#endif /* MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_ */
