#include "MantidVatesAPI/PresenterFactories.h"
#include "MantidVatesAPI/FactoryChains.h"

#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/make_unique.h"

#include <vtkBox.h>

#include <chrono>
#include <ctime>
#include <algorithm>

namespace {
/// Static logger
Mantid::Kernel::Logger g_log_presenter_utilities("PresenterUtilities");
}

namespace Mantid {
namespace VATES {

/**
 * Gets a clipped object
 * @param dataSet: the unclipped data set
 * @returns a clipped object
 */
vtkSmartPointer<vtkPVClipDataSet>
getClippedDataSet(vtkSmartPointer<vtkDataSet> dataSet) {
  auto box = vtkSmartPointer<vtkBox>::New();
  box->SetBounds(dataSet->GetBounds());
  auto clipper = vtkSmartPointer<vtkPVClipDataSet>::New();
  clipper->SetInputData(dataSet);
  clipper->SetClipFunction(box);
  clipper->SetInsideOut(true);
  clipper->Update();
  return clipper;
}

/**
 * Applies the correct change of basis matrix to the vtk data set. This is
 * especially important for
 * non-orthogonal data sets.
 * @param presenter: a pointer to a presenter instance
 * @param dataSet: the data set which holds the COB information
 * @param workspaceProvider: provides one or multiple workspaces
 */
void applyCOBMatrixSettingsToVtkDataSet(
    Mantid::VATES::MDLoadingPresenter *presenter, vtkDataSet *dataSet,
    std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider) {
  try {
    presenter->makeNonOrthogonal(dataSet, std::move(workspaceProvider));
  } catch (std::invalid_argument &e) {
    std::string error = e.what();
    g_log_presenter_utilities.warning()
        << "PresenterUtilities: Workspace does not have correct "
           "information to "
        << "plot non-orthogonal axes: " << error;
    // Add the standard change of basis matrix and set the boundaries
    presenter->setDefaultCOBandBoundaries(dataSet);
  } catch (...) {
    g_log_presenter_utilities.warning()
        << "PresenterUtilities: Workspace does not have correct "
           "information to "
        << "plot non-orthogonal axes. Non-orthogonal axes features require "
           "three dimensions.";
  }
}

/**
 * Creates a factory chain for MDEvent workspaces
 * @param threshold: the threshold range
 * @param normalization: the normalization option
 * @param time: the time slice time
 * @returns a factory chain
 */
std::unique_ptr<vtkMDHexFactory>
createFactoryChainForEventWorkspace(ThresholdRange_scptr threshold,
                                    VisualNormalization normalization,
                                    double time) {
  auto factory =
      Mantid::Kernel::make_unique<vtkMDHexFactory>(threshold, normalization);
  factory->setSuccessor(Mantid::Kernel::make_unique<vtkMDQuadFactory>(
                            threshold, normalization))
      .setSuccessor(Mantid::Kernel::make_unique<vtkMDLineFactory>(
          threshold, normalization))
      .setSuccessor(Mantid::Kernel::make_unique<vtkMD0DFactory>());
  factory->setTime(time);
  return factory;
}

/**
* Creates a factory chain for MDHisto workspaces
* @param threshold: the threshold range
* @param normalization: the normalization option
* @param time: the time slice time
* @returns a factory chain
*/
std::unique_ptr<vtkMDHistoHex4DFactory<TimeToTimeStep>>
createFactoryChainForHistoWorkspace(ThresholdRange_scptr threshold,
                                    VisualNormalization normalization,
                                    double time) {
  auto factory =
      Mantid::Kernel::make_unique<vtkMDHistoHex4DFactory<TimeToTimeStep>>(
          threshold, normalization, time);
  factory->setSuccessor(Mantid::Kernel::make_unique<vtkMDHistoHexFactory>(
                            threshold, normalization))
      .setSuccessor(Mantid::Kernel::make_unique<vtkMDHistoQuadFactory>(
          threshold, normalization))
      .setSuccessor(Mantid::Kernel::make_unique<vtkMDHistoLineFactory>(
          threshold, normalization))
      .setSuccessor(Mantid::Kernel::make_unique<vtkMD0DFactory>());
  return factory;
}

/**
* Creates a time stamped name
* @param name: the input name
* @return a name with a time stamp
*/
std::string createTimeStampedName(const std::string &name) {
  auto currentTime =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::string timeInReadableFormat = std::string(std::ctime(&currentTime));
  // Replace all white space with double underscore
  std::replace(timeInReadableFormat.begin(), timeInReadableFormat.end(), ' ',
               '_');
  // Replace all colons with single underscore
  std::replace(timeInReadableFormat.begin(), timeInReadableFormat.end(), ':',
               '_');
  timeInReadableFormat.erase(std::remove(timeInReadableFormat.begin(),
                                         timeInReadableFormat.end(), '\n'),
                             timeInReadableFormat.end());
  std::string stampedName = name + "_";
  stampedName = stampedName + timeInReadableFormat;
  return stampedName;
}
}
}
