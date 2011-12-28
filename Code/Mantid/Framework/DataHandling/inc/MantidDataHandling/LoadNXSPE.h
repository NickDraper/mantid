#ifndef MANTID_DATAHANDLING_LOADNXSPE_H_
#define MANTID_DATAHANDLING_LOADNXSPE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadNXSPE : Algorithm to load an NXSPE file into a workspace2D. It will create a "new" instrument, that can be overwritten later by the LoadInstrument algorithm
    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    </ul>
    @author Andrei Savici, ORNL
    @date 2011-08-14

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadNXSPE  : public API::IDataFileChecker
  {
  public:
    LoadNXSPE();
    ~LoadNXSPE();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadNXSPE";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling\\Nexus;DataHandling\\SPE;Inelastic";}
    /// Do a quick check that this file can be loaded
    virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    /// check the structure of the file and  return a value between
    /// 0 and 100 of how much this file can be loaded
    virtual int fileCheck(const std::string& filePath);

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADNXSPE_H_ */
