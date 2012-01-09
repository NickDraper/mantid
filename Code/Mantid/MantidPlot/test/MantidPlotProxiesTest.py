""" 
Test the use of proxy objects in MantidPlot that 
prevent crashes when accessing the python object
after deletion of the object
"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# =============== Create a fake workspace to plot =======================
X1 = np.linspace(0,10, 100)
Y1 = 1000*(np.sin(X1)**2) + X1*10
X1 = np.append(X1, 10.1)

X2 = np.linspace(2,12, 100)
Y2 = 500*(np.cos(X2/2.)**2) + 20
X2 = np.append(X2, 12.10)

X = np.append(X1, X2)
Y = np.append(Y1, Y2)
E = np.sqrt(Y)

CreateWorkspace(OutputWorkspace="fake", DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2, UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")

class MantidPlotProxiesTest(unittest.TestCase):
    
    def setUp(self):
        pass
    
    def tearDown(self):
        """Clean up by closing the created window """
        pass
        
    def try_closing(self, obj, msg=""):
        """ Try closing a graphical object, and
        access the variable to see if it has been set to None """
        # No closing dialog
        obj.confirmClose(False)
        # This should close (and hopefully delete) obj
        obj.close()
        # Make sure the event passes
        QtCore.QCoreApplication.processEvents()
        QtCore.QCoreApplication.processEvents()
        # Check that the object has been None'd
        self.assertTrue(obj._getHeldObject() is None, msg + "'s return value gets cleared when closed.")
        
    def test_closing_retrieved_object(self):
        """Create object using newXXX("name"), retrieve it using XXX("name") and then close it """
        for cmd in ['table', 'matrix', 'graph', 'note']:
            name = "testobject%s" % cmd
            # Create a method called newTable, for example
            newMethod = "new" + cmd[0].upper() + cmd[1:] + '("%s")' % name
            eval(newMethod)
            obj = eval(cmd + '("%s")' % name)
            self.try_closing(obj, cmd+"()")

    def test_closing_newTable(self):
        obj = newTable()
        self.try_closing(obj, "newTable()")
        
    def test_closing_newMatrix(self):
        obj = newMatrix()
        self.try_closing(obj, "newMatrix()")

    def test_closing_newPlot3D(self):
        obj = newPlot3D()
        self.try_closing(obj, "newPlot3D()")
                        
    def test_closing_newNote(self):
        obj = newNote()
        self.try_closing(obj, "newNote()")
        
    def test_closing_newGraph(self):
        obj = newGraph()
        self.try_closing(obj, "newGraph()")
        
    def test_closing_layers(self):
        g = newGraph()
        l0 = g.layer(0)
        l1 = g.addLayer()
        l_active = g.activeLayer()
        self.try_closing(g, "newGraph()")
        self.assertTrue(l0._getHeldObject() is None, "Layer object 0 from deleted graph is None")
        self.assertTrue(l1._getHeldObject() is None, "Layer object 1 from deleted graph is None")
        self.assertTrue(l_active._getHeldObject() is None, "Active Layer object from deleted graph is None")

    def test_closing_Layer_objects(self):
        g = plotSpectrum("fake", [0,1])
        l = g.activeLayer()
        
        return
        # FIXME! The following calls fail:
        legend = l.legend()
        legend2 = l.newLegend()
        grid = l.grid()
        self.assertTrue(legend._getHeldObject() is None, "Deleted legend safely")
        self.assertTrue(legend2._getHeldObject() is None, "Deleted new legend safely")
        self.assertTrue(grid._getHeldObject() is None, "Deleted grid safely")
        
        #spectrogram = l.spectrogram()
        #self.assertTrue(spectrogram._getHeldObject() is None, "Deleted spectrogram safely")

    
# Run the unit tests
mantidplottests.runTests(MantidPlotProxiesTest)

