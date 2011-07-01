# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise()
    from mantidsimple import *
    HAS_MANTID = True
except:
    HAS_MANTID = False 
    
class DataProxy(object):
    """
        Class used to load a data file temporarily to extract header information
    """
    data_ws = None
    
    ## Error log
    errors = []
    
    def __init__(self, data_file, workspace_name=None):
        self.errors = []
        if HAS_MANTID:
            print "loading", data_file
            try:
                if workspace_name is None:
                    self.data_ws = "__raw_data_file"
                else:
                    self.data_ws = str(workspace_name)
                LoadEventNexus(Filename=data_file, OutputWorkspace=workspace_name)
            except:
                self.data_ws = None
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
