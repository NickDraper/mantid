//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropWorkspace)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

// Get a reference to the logger.
Logger& CropWorkspace::g_log = Logger::get("CropWorkspace");

/// Default constructor
CropWorkspace::CropWorkspace() : Algorithm(), m_minX(0), m_maxX(0), m_minSpec(0), m_maxSpec(0)
{}

/// Destructor
CropWorkspace::~CropWorkspace() {}

void CropWorkspace::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,new CommonBinsValidator<Workspace2D>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("XMin",0.0);
  declareProperty("XMax",0.0);
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartSpectrum",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndSpectrum",0, mustBePositive->clone());
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void CropWorkspace::exec()
{
  // Get the input workspace
  m_inputWorkspace = getProperty("InputWorkspace");
  const bool histogram = m_inputWorkspace->isHistogramData();

  // Retrieve and validate the input properties
  this->checkProperties();

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace =
    WorkspaceFactory::Instance().create(m_inputWorkspace,m_maxSpec-m_minSpec+1,m_maxX-m_minX,m_maxX-m_minX-histogram);
  DataObjects::Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(outputWorkspace);

  // If this is a Workspace2D, get the spectra axes for copying in the spectraNo later
  Axis *specAxis = NULL, *outAxis = NULL;
  if (m_inputWorkspace->axes() > 1)
  {
    specAxis = m_inputWorkspace->getAxis(1);
    outAxis = outputWorkspace->getAxis(1);
  }

  DataObjects::Histogram1D::RCtype newX;
  const MantidVec& oldX = m_inputWorkspace->readX(m_minSpec);
  newX.access().assign(oldX.begin()+m_minX,oldX.begin()+m_maxX);

  // Loop over the required spectra, copying in the desired bins
  for (int i = m_minSpec, j = 0; i <= m_maxSpec; ++i,++j)
  {
    output2D->setX(j,newX);
    const MantidVec& oldY = m_inputWorkspace->readY(i);
    outputWorkspace->dataY(j).assign(oldY.begin()+m_minX,oldY.begin()+(m_maxX-histogram));
    const MantidVec& oldE = m_inputWorkspace->readE(i);
    outputWorkspace->dataE(j).assign(oldE.begin()+m_minX,oldE.begin()+(m_maxX-histogram));
    if (specAxis) outAxis->spectraNo(j) = specAxis->spectraNo(i);
    
    // Propagate bin masking if there is any
    if ( m_inputWorkspace->hasMaskedBins(i) )
    {
      const MatrixWorkspace::MaskList& inputMasks = m_inputWorkspace->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it)
      {
        outputWorkspace->maskBin(j,(*it).first - m_minX,(*it).second);
      }
    }
  }

  setProperty("OutputWorkspace", outputWorkspace);
  // Reset the input workspace member variable
  m_inputWorkspace = DataObjects::Workspace2D_const_sptr();
}

/** Retrieves the optional input properties and checks that they have valid values.
 *  Assigns to the defaults if any property has not been set.
 *  @throw std::invalid_argument If the input workspace does not have common binning
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void CropWorkspace::checkProperties()
{
  // Do the full check on common workspace binning
  if ( ! WorkspaceHelpers::commonBoundaries(m_inputWorkspace) )
  {
    g_log.error("The input workspace must have common X values across all spectra");
    throw std::invalid_argument("The input workspace must have common X values across all spectra");
  }
  
  this->getXMin();
  this->getXMax();

  if ( m_minX > m_maxX )
  {
    g_log.error("XMin must be less than XMax");
    throw std::out_of_range("XMin must be less than XMax");
  }
  if ( m_minX == m_maxX )
  {
    g_log.error("The X range given lies entirely within a single bin");
    throw std::out_of_range("The X range given lies entirely within a single bin");
  }

  m_minSpec = getProperty("StartSpectrum");
  const int numberOfSpectra = m_inputWorkspace->getNumberHistograms();
  Property *p = getProperty("EndSpectrum");
  if ( p->isDefault() ) m_maxSpec = numberOfSpectra-1;
    else m_maxSpec = getProperty("EndSpectrum");

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_minSpec > numberOfSpectra-1 )
  {
    g_log.error("StartSpectrum out of range!");
    throw std::out_of_range("StartSpectrum out of range!");
  }
  if ( m_maxSpec > numberOfSpectra-1 )
  {
    g_log.error("EndSpectrum out of range!");
    throw std::out_of_range("EndSpectrum out of range!");
  }
  if ( m_maxSpec < m_minSpec )
  {
    g_log.error("StartSpectrum must be less than or equal to EndSpectrum");
    throw std::out_of_range("StartSpectrum must be less than or equal to EndSpectrum");
  }
}

/** Find the X index corresponding to (or just within) the value given in the XMin property.
 *  Sets the default if the property has not been set.
 */
void CropWorkspace::getXMin()
{
  Property *minX = getProperty("XMin");
  if ( minX->isDefault() )
  {
    m_minX = 0;
  }
  else
  {
    const double minX_val = getProperty("XMin");
    const MantidVec& X = m_inputWorkspace->readX(0);
    if ( minX_val > X.back() )
    {
      g_log.error("XMin is greater than the largest X value");
      throw std::out_of_range("XMin is greater than the largest X value");
    }
    m_minX = std::lower_bound(X.begin(),X.end(),minX_val) - X.begin();
  }
}

/** Find the X index corresponding to (or just within) the value given in the XMax property.
 *  Sets the default if the property has not been set.
 */
void CropWorkspace::getXMax()
{
  const MantidVec& X = m_inputWorkspace->readX(0);
  Property *maxX = getProperty("XMax");
  if ( maxX->isDefault() )
  {
    m_maxX = X.size();
  }
  else
  {
    const double maxX_val = getProperty("XMax");
    if ( maxX_val < X.front() )
    {
      g_log.error("XMax is less than the smallest X value");
      throw std::out_of_range("XMax is less than the smallest X value");
    }
    m_maxX = std::upper_bound(X.begin(),X.end(),maxX_val) - X.begin();
  }
}

} // namespace Algorithms
} // namespace Mantid
