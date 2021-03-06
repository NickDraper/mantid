#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabView.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::Geometry;

/** Constructor
* @param view :: The view we are handling
*/
ReflSettingsTabPresenter::ReflSettingsTabPresenter(IReflSettingsTabView *view)
    : m_view(view), m_mainPresenter() {

  // Create the 'HintingLineEdits'
  createStitchHints();
}

/** Destructor
*/
ReflSettingsTabPresenter::~ReflSettingsTabPresenter() {}

/** Accept a main presenter
* @param mainPresenter :: [input] The main presenter
*/
void ReflSettingsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Used by the view to tell the presenter something has changed
*/
void ReflSettingsTabPresenter::notify(IReflSettingsTabPresenter::Flag flag) {
  switch (flag) {
  case IReflSettingsTabPresenter::ExpDefaultsFlag:
    getExpDefaults();
    break;
  case IReflSettingsTabPresenter::InstDefaultsFlag:
    getInstDefaults();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Sets the current instrument name and changes accessibility status of
* the polarisation corrections option in the view accordingly
* @param instName :: [input] The name of the instrument to set to
*/
void ReflSettingsTabPresenter::setInstrumentName(const std::string instName) {
  m_currentInstrumentName = instName;
  m_view->setPolarisationOptionsEnabled(instName != "INTER" &&
                                        instName != "SURF");
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string ReflSettingsTabPresenter::getTransmissionOptions() const {

  std::vector<std::string> options;

  // Add analysis mode
  auto analysisMode = m_view->getAnalysisMode();
  if (!analysisMode.empty())
    options.push_back("AnalysisMode=" + analysisMode);

  // Add monitor integral min
  auto monIntMin = m_view->getMonitorIntegralMin();
  if (!monIntMin.empty())
    options.push_back("MonitorIntegrationWavelengthMin=" + monIntMin);

  // Add monitor integral max
  auto monIntMax = m_view->getMonitorIntegralMax();
  if (!monIntMax.empty())
    options.push_back("MonitorIntegrationWavelengthMax=" + monIntMax);

  // Add monitor background min
  auto monBgMin = m_view->getMonitorBackgroundMin();
  if (!monBgMin.empty())
    options.push_back("MonitorBackgroundWavelengthMin=" + monBgMin);

  // Add monitor background max
  auto monBgMax = m_view->getMonitorBackgroundMax();
  if (!monBgMax.empty())
    options.push_back("MonitorBackgroundWavelengthMax=" + monBgMax);

  // Add lambda min
  auto lamMin = m_view->getLambdaMin();
  if (!lamMin.empty())
    options.push_back("WavelengthMin=" + lamMin);

  // Add lambda max
  auto lamMax = m_view->getLambdaMax();
  if (!lamMax.empty())
    options.push_back("WavelengthMax=" + lamMax);

  // Add I0MonitorIndex
  auto I0MonitorIndex = m_view->getI0MonitorIndex();
  if (!I0MonitorIndex.empty())
    options.push_back("I0MonitorIndex=" + I0MonitorIndex);

  // Add detector limits
  auto procInst = m_view->getProcessingInstructions();
  if (!procInst.empty())
    options.push_back("ProcessingInstructions=" + procInst);

  return boost::algorithm::join(options, ",");
}

/** Returns global options for 'ReflectometryReductionOneAuto'
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
std::string ReflSettingsTabPresenter::getReductionOptions() const {

  std::vector<std::string> options;

  // Add analysis mode
  auto analysisMode = m_view->getAnalysisMode();
  if (!analysisMode.empty())
    options.push_back("AnalysisMode=" + analysisMode);

  // Add CRho
  auto crho = m_view->getCRho();
  if (!crho.empty())
    options.push_back("CRho=" + crho);

  // Add CAlpha
  auto calpha = m_view->getCAlpha();
  if (!calpha.empty())
    options.push_back("CAlpha=" + calpha);

  // Add CAp
  auto cap = m_view->getCAp();
  if (!cap.empty())
    options.push_back("CAp=" + cap);

  // Add CPp
  auto cpp = m_view->getCPp();
  if (!cpp.empty())
    options.push_back("CPp=" + cpp);

  // Add direct beam
  auto dbnr = m_view->getDirectBeam();
  if (!dbnr.empty())
    options.push_back("RegionOfDirectBeam=" + dbnr);

  // Add polarisation corrections
  auto polCorr = m_view->getPolarisationCorrections();
  if (!polCorr.empty())
    options.push_back("PolarizationAnalysis=" + polCorr);

  // Add integrated monitors option
  auto intMonCheck = m_view->getIntMonCheck();
  if (!intMonCheck.empty())
    options.push_back("NormalizeByIntegratedMonitors=" + intMonCheck);

  // Add monitor integral min
  auto monIntMin = m_view->getMonitorIntegralMin();
  if (!monIntMin.empty())
    options.push_back("MonitorIntegrationWavelengthMin=" + monIntMin);

  // Add monitor integral max
  auto monIntMax = m_view->getMonitorIntegralMax();
  if (!monIntMax.empty())
    options.push_back("MonitorIntegrationWavelengthMax=" + monIntMax);

  // Add monitor background min
  auto monBgMin = m_view->getMonitorBackgroundMin();
  if (!monBgMin.empty())
    options.push_back("MonitorBackgroundWavelengthMin=" + monBgMin);

  // Add monitor background max
  auto monBgMax = m_view->getMonitorBackgroundMax();
  if (!monBgMax.empty())
    options.push_back("MonitorBackgroundWavelengthMax=" + monBgMax);

  // Add lambda min
  auto lamMin = m_view->getLambdaMin();
  if (!lamMin.empty())
    options.push_back("WavelengthMin=" + lamMin);

  // Add lambda max
  auto lamMax = m_view->getLambdaMax();
  if (!lamMax.empty())
    options.push_back("WavelengthMax=" + lamMax);

  // Add I0MonitorIndex
  auto I0MonitorIndex = m_view->getI0MonitorIndex();
  if (!I0MonitorIndex.empty())
    options.push_back("I0MonitorIndex=" + I0MonitorIndex);

  // Add scale factor
  auto scaleFactor = m_view->getScaleFactor();
  if (!scaleFactor.empty())
    options.push_back("ScaleFactor=" + scaleFactor);

  // Add momentum transfer limits
  auto qTransStep = m_view->getMomentumTransferStep();
  if (!qTransStep.empty()) {
    options.push_back("MomentumTransferStep=" + qTransStep);
  }

  // Add detector limits
  auto procInst = m_view->getProcessingInstructions();
  if (!procInst.empty())
    options.push_back("ProcessingInstructions=" + procInst);

  // Add transmission runs
  auto transRuns = this->getTransmissionRuns();
  if (!transRuns.empty())
    options.push_back(transRuns);

  return boost::algorithm::join(options, ",");
}

/** Receives specified transmission runs from the view and loads them into the
*ADS. Returns a string with transmission runs so that they are considered in the
*reduction
*
* @return :: transmission run(s) as a string that will be used for the reduction
*/
std::string ReflSettingsTabPresenter::getTransmissionRuns() const {

  auto runs = m_view->getTransmissionRuns();
  if (runs.empty())
    return "";

  std::vector<std::string> transRuns;
  boost::split(transRuns, runs, boost::is_any_of(","));

  if (transRuns.size() > 2)
    throw std::invalid_argument("Only one transmission run or two "
                                "transmission runs separated by ',' "
                                "are allowed.");

  for (const auto &run : transRuns) {
    if (AnalysisDataService::Instance().doesExist("TRANS_" + run))
      continue;
    // Load transmission runs and put them in the ADS
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadISISNexus");
    alg->setProperty("Filename", run);
    alg->setPropertyValue("OutputWorkspace", "TRANS_" + run);
    alg->execute();
  }

  // Return them as options for reduction
  std::string options = "FirstTransmissionRun=TRANS_" + transRuns[0];
  if (transRuns.size() > 1)
    options += ",SecondTransmissionRun=TRANS_" + transRuns[1];

  return options;
}

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string ReflSettingsTabPresenter::getStitchOptions() const {

  return m_view->getStitchOptions();
}

/** Creates hints for 'Stitch1DMany'
*/
void ReflSettingsTabPresenter::createStitchHints() {

  // The algorithm
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Stitch1DMany");
  // The blacklist
  std::set<std::string> blacklist = {"InputWorkspaces", "OutputWorkspace",
                                     "OutputWorkspace"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createStitchHints(strategy.createHints());
}

/** Fills experiment settings with default values
*/
void ReflSettingsTabPresenter::getExpDefaults() {
  // Algorithm and instrument
  auto alg = createReductionAlg();
  auto inst = createEmptyInstrument(m_currentInstrumentName);

  // Collect all default values and set them in view
  std::vector<std::string> defaults(7);
  defaults[0] = alg->getPropertyValue("AnalysisMode");
  defaults[1] = alg->getPropertyValue("PolarizationAnalysis");

  auto cRho = inst->getStringParameter("crho");
  if (!cRho.empty())
    defaults[2] = cRho[0];

  auto cAlpha = inst->getStringParameter("calpha");
  if (!cAlpha.empty())
    defaults[3] = cAlpha[0];

  auto cAp = inst->getStringParameter("cAp");
  if (!cAp.empty())
    defaults[4] = cAp[0];

  auto cPp = inst->getStringParameter("cPp");
  if (!cPp.empty())
    defaults[5] = cPp[0];

  defaults[6] = alg->getPropertyValue("ScaleFactor");

  m_view->setExpDefaults(defaults);
}

/** Fills instrument settings with default values
*/
void ReflSettingsTabPresenter::getInstDefaults() {
  // Algorithm and instrument
  auto alg = createReductionAlg();
  auto inst = createEmptyInstrument(m_currentInstrumentName);

  // Collect all default values
  std::vector<double> defaults(8);
  defaults[0] = boost::lexical_cast<double>(
      alg->getPropertyValue("NormalizeByIntegratedMonitors"));
  defaults[1] = inst->getNumberParameter("MonitorIntegralMin")[0];
  defaults[2] = inst->getNumberParameter("MonitorIntegralMax")[0];
  defaults[3] = inst->getNumberParameter("MonitorBackgroundMin")[0];
  defaults[4] = inst->getNumberParameter("MonitorBackgroundMax")[0];
  defaults[5] = inst->getNumberParameter("LambdaMin")[0];
  defaults[6] = inst->getNumberParameter("LambdaMax")[0];
  defaults[7] = inst->getNumberParameter("I0MonitorIndex")[0];

  m_view->setInstDefaults(defaults);
}

/** Generates and returns an instance of the ReflectometryReductionOne algorithm
*/
IAlgorithm_sptr ReflSettingsTabPresenter::createReductionAlg() {
  return AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
}

/** Creates and returns an example empty instrument given an instrument name
*/
Instrument_const_sptr
ReflSettingsTabPresenter::createEmptyInstrument(std::string instName) {
  IAlgorithm_sptr loadInst =
      AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadInst->setChild(true);
  loadInst->setProperty("OutputWorkspace", "outWs");
  loadInst->setProperty("InstrumentName", instName);
  loadInst->execute();
  MatrixWorkspace_const_sptr ws = loadInst->getProperty("OutputWorkspace");
  return ws->getInstrument();
}
}
}