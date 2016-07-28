
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveDiffFittingAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "Poco/File.h"
#include <boost/tokenizer.hpp>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveDiffFittingAscii)

using namespace Kernel;
using namespace API;

/// Empty constructor
SaveDiffFittingAscii::SaveDiffFittingAscii()
    : Mantid::API::Algorithm(), m_sep(','), m_endl('\n') {}

/// Initialisation method.
void SaveDiffFittingAscii::init() {

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the workspace containing the data you want to "
                  "save to a TBL file.");

  // Declare required parameters, filename with ext {.his} and input
  // workspace
  const std::vector<std::string> exts{".txt", ".csv", ""};
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");

  declareProperty("RunNumber", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Run number of the focused file used to generate the "
                  "parameters table workspace.");

  declareProperty("Bank", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Bank of the focused file used to generate the parameters.");
}

/**
*   Executes the algorithm.
*/
void SaveDiffFittingAscii::exec() {
  // Get the workspace

  // Process properties

  // Retrieve the input workspace
  /// Workspace
  ITableWorkspace_sptr tbl_ws = getProperty("InputWorkspace");
  if (!tbl_ws)
    throw std::runtime_error("Please provide an input workspace to be saved.");

  std::string filename = getProperty("Filename");
  std::ofstream file(filename.c_str());

  if (!file) {
    throw Exception::FileError("Unable to create file: ", filename);
  }

  std::string runNum = getProperty("RunNumber");
  file << "run number: " << runNum << m_endl;

  std::string bank = getProperty("Bank");
  file << "bank: " << bank << m_endl;

  std::vector<std::string> columnHeadings = tbl_ws->getColumnNames();
  for (auto &heading : columnHeadings) {
    if (heading == "Chi") {
      writeVal<std::string>(heading, file, true);
    } else {
      writeVal<std::string>(heading, file, false);
    }
  }
}

template <class T>
void SaveDiffFittingAscii::writeVal(T &val, std::ofstream &file, bool endline) {
  std::string valStr = boost::lexical_cast<std::string>(val);

  // checking if it needs to be surrounded in
  // quotes due to a comma being included
  size_t comPos = valStr.find(',');
  if (comPos != std::string::npos) {
    file << '"' << val << '"';
  } else {
    file << boost::lexical_cast<T>(val);
  }

  if (endline) {
    file << m_endl;
  } else {
    file << m_sep;
  }
}

} // namespace DataHandling
} // namespace Mantid
