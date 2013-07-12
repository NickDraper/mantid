#ifndef MANTID_DATAHANDLING_LOADRAWHELPER_H_
#define MANTID_DATAHANDLING_LOADRAWHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidAPI/Run.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW;
class ISISRAW2;

namespace Mantid
{
  namespace API
  {
    class SpectrumDetectorMapping;
  }

  namespace DataHandling
  {
    /** @class LoadRawHelper DataHandling/LoadRawHelper.h

	Helper class for LoadRaw algorithms.


	@author Sofia Antony, ISIS,RAL
	@date 14/04/2010

	Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

	File change history is stored at: <https://github.com/mantidproject/mantid>.
	Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport LoadRawHelper: public API::IFileLoader
    {
    public:
      /// Default constructor
      LoadRawHelper();
      /// Destructor
      ~LoadRawHelper();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRawHelper"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      ///Opens Raw File
      FILE* openRawFile(const std::string & fileName);
      /// Read in run parameters Public so that LoadRaw2 can use it
      void loadRunParameters(API::MatrixWorkspace_sptr localWorkspace, ISISRAW * const = NULL) const;

      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    protected:
      /// Overwrites Algorithm method.
      void init();
      ///checks the file is an ascii file
      bool isAscii(FILE* file) const;
      /// Reads title from the isisraw class
      void readTitle(FILE* file,std::string & title);
      /// reads workspace parameters like number of histograms,size of vectors etc
      void readworkspaceParameters(specid_t &numberOfSpectra,int & numberOfPeriods,int64_t& lengthIn,int64_t& noTimeRegimes );

      /// skips histrogram data from raw file.
      void skipData(FILE* file,int hist);
      void skipData(FILE* file,int64_t hist);

      ///calls isisRaw ioraw
      void ioRaw(FILE* file,bool from_file );

      /// reads data
      bool readData(FILE* file,int histToRead);
      bool readData(FILE* file,int64_t histToRead);

      ///creates shared pointer to workspace from parent workspace
      DataObjects::Workspace2D_sptr createWorkspace(DataObjects::Workspace2D_sptr ws_sptr,
          int64_t nVectors=-1,int64_t xLengthIn=-1,int64_t yLengthIn=-1);

      /// overloaded method to create shared pointer to workspace
      DataObjects::Workspace2D_sptr createWorkspace(int64_t nVectors,int64_t xlengthIn,int64_t ylengthIn,const std::string& title);
      ///creates monitor workspace
      void createMonitorWorkspace(DataObjects::Workspace2D_sptr& monws_sptr,
          DataObjects::Workspace2D_sptr& ws_sptr,API::WorkspaceGroup_sptr& mongrp_sptr,
          const int64_t mwsSpecs,const int64_t nwsSpecs,const int64_t numberOfPeriods,const int64_t lenthIn,std::string title);

      /// creates  shared pointer to group workspace 
      API::WorkspaceGroup_sptr createGroupWorkspace();

      //Constructs the time channel (X) vector(s)     
      std::vector<boost::shared_ptr<MantidVec> > getTimeChannels(const int64_t& regimes, const int64_t& lengthIn);
      /// loadinstrument Child Algorithm
      void runLoadInstrument(const std::string& fileName,DataObjects::Workspace2D_sptr, double, double );
      /// loadinstrumentfromraw algorithm
      void runLoadInstrumentFromRaw(const std::string& fileName,DataObjects::Workspace2D_sptr);
      /// loadinstrumentfromraw Child Algorithm
      void runLoadMappingTable(const std::string& fileName,DataObjects::Workspace2D_sptr);
      /// load log algorithm
      void runLoadLog(const std::string& fileName,DataObjects::Workspace2D_sptr, double, double);

      /// Create the period specific logs
      void createPeriodLogs(int64_t period, DataObjects::Workspace2D_sptr local_workspace);

      ///gets the monitor spectrum list from the workspace
      std::vector<specid_t> getmonitorSpectrumList(const API::SpectrumDetectorMapping& mapping);

      /// sets the workspace property 
      void setWorkspaceProperty(const std::string & propertyName,const std::string& title,
          API::WorkspaceGroup_sptr grpws_sptr,DataObjects::Workspace2D_sptr ws_sptr,int64_t numberOfPeriods,bool bMonitor);

      /// overloaded method to set the workspace property 
      void setWorkspaceProperty(DataObjects::Workspace2D_sptr ws_sptr,API::WorkspaceGroup_sptr grpws_sptr,const int64_t period,bool bmonitors);

      /// This method sets the raw file data to workspace vectors
      void setWorkspaceData(DataObjects::Workspace2D_sptr newWorkspace,const std::vector<boost::shared_ptr<MantidVec> >& 
          timeChannelsVec,int64_t wsIndex,specid_t nspecNum,int64_t noTimeRegimes,int64_t lengthIn,int64_t binStart);


      /// ISISRAW class instance which does raw file reading. Shared pointer to prevent memory leak when an exception is thrown.
      boost::shared_ptr<ISISRAW2> isisRaw;

      /// get proton charge from raw file
      float getProtonCharge()const ;
      /// set proton charge
      void setProtonCharge(API::Run& run);
      /// Stores the run number in the sample's logs
      void setRunNumber(API::Run& run);

      /// number of time regimes
      int getNumberofTimeRegimes();
      /// resets the isisraw shared pointer
      void reset();

      ///sets optional properties like spec_min,spec_max etc
      void setOptionalProperties(const int& spec_min,const int& spec_max,const std::vector<int>& spec_list);
      /// Validates the optional 'spectra to read' properties, if they have been set
      void checkOptionalProperties();
      /// calculate workspace size
      specid_t  calculateWorkspaceSize();
      /// calculate workspace sizes if separate or exclude monitors are selected
      void calculateWorkspacesizes(const std::vector<specid_t>& monitorSpecList,
          specid_t& normalwsSpecs, specid_t& monitorwsSpecs);
      /// load the specra
      void loadSpectra(FILE* file,const int& period, const int& m_total_specs,
          DataObjects::Workspace2D_sptr ws_sptr,std::vector<boost::shared_ptr<MantidVec> >);

      /// Has the spectrum_list property been set?
      bool m_list;
      /// Have the spectrum_min/max properties been set?
      bool m_interval;
      /// The value of the spectrum_list property
      std::vector<specid_t> m_spec_list;
      /// The value of the spectrum_min property
      specid_t m_spec_min;
      /// The value of the spectrum_max property
      specid_t m_spec_max;
      /// The number of periods in the raw file
      int m_numberOfPeriods;

    private:

      /// Overwrites Algorithm method
      void exec();
      /// convert month label to int string
      std::string convertMonthLabelToIntStr(std::string month) const;

      /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;
      /// A map for storing the time regime for each spectrum
      std::map<specid_t,specid_t> m_specTimeRegimes;
      /// The current value of the progress counter
      double m_prog;

      /// number of spectra
      specid_t m_numberOfSpectra;

      /// a vector holding the indexes of monitors
      std::vector<specid_t> m_monitordetectorList;

      /// boolean for list spectra options
      bool m_bmspeclist;

      ///the total nuumber of spectra
      specid_t m_total_specs;

      /// A ptr to the log creator
      boost::scoped_ptr<ISISRunLogs> m_logCreator;

      /// Search for the log files in the workspace, and output their names as a set.
      std::set<std::string> searchForLogFiles(const std::string& fileName);
      /// convert string to lower case
      std::string stringToLower(std::string strToConvert);
      /// Checks if the file is an ASCII file
      bool isAscii(const std::string& filenamePart);
      /// check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
      bool isDateTimeString(const std::string& str) const;
      /// Return the name of the three column log file for associated with the specified file. Empty string if one doesn't exist
      std::string getThreeColumnName() const;
      /// Check for SNS-style text file
      bool SNSTextFormatColumns(const std::string& str, std::vector<double> & out) const;
      /// create timeseries property from .log file and adds taht to sample object
      std::set<std::string> createthreecolumnFileLogProperty(const std::string& logfile, API::Run& run);
      /// if a file with the second column(block column) name in .log file exists in the raw file directory
      bool blockcolumnFileExists(const std::string& fileName);
      /// if  alternate data stream named checksum exists for the raw file
      bool adsExists();
      /// returns the list of log files from ADS checksum
      std::set<std::string> getLogfilenamesfromADS();
      /// The name and path of an input file. This may be the filename of a raw datafile or the name of a specific log file.
      std::string m_filename;
      /// type returned by classify
      enum kind { empty, string, number };
      /// Takes as input a string and try to determine what type it is
      kind classify(const std::string& s) const;
      /// SNS text
      bool LoadSNSText();
    };

  } // namespace DataHandling
} // namespace Mantid
#endif /*MANTID_DATAHANDLING_LOADRAW_H_*/
