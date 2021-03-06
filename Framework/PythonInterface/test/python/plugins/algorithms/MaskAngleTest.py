from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *

class MaskAngleTest(unittest.TestCase):

    def testMaskAngle(self):
        w=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w',w)
        masklist = MaskAngle(w,10,20)
        for i in arange(w.getNumberHistograms())+1:
            if (i<10) or (i>19):
                self.assertTrue(not w.getInstrument().getDetector(int(i)).isMasked())
            else:
                self.assertTrue(w.getInstrument().getDetector(int(i)).isMasked())
        DeleteWorkspace(w)
        self.assertTrue(array_equal(masklist,arange(10)+10))

    def testGroupMaskAngle(self):
        ws1=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('ws1',ws1)
        ws2=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('ws2',ws2)

        group = GroupWorkspaces(['ws1', 'ws2'])
        MaskAngle(group, 10, 20)

        for w in group:
            for i in arange(w.getNumberHistograms())+1:
                if(i<10) or (i>19):
                    self.assertTrue(not w.getInstrument().getDetector(int(i)).isMasked())
                else:
                    self.assertTrue(w.getInstrument().getDetector(int(i)).isMasked())

        DeleteWorkspace(group)

    def testFailNoInstrument(self):
        w1=CreateWorkspace(arange(5),arange(5))
        try:
            MaskAngle(w1,10,20)
            self.fail("Should not have got here. Should throw because no instrument.")
        except (RuntimeError, ValueError):
            pass
        finally:
            DeleteWorkspace(w1)

    def testGroupFailNoInstrument(self):
        ws1=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('ws1',ws1)
        ws2 = CreateWorkspace(arange(5), arange(5))

        group = GroupWorkspaces(['ws1', 'ws2'])

        try:
            MaskAngle(group, 10, 20)
            self.fail("Should not have gotten here. Should throw because no instrument.")
        except (RuntimeError, ValueError, TypeError):
            pass
        finally:
            DeleteWorkspace(group)

    def testFailLimits(self):
        w2=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w2',w2)
        w3=CloneWorkspace('w2')
        w4=CloneWorkspace('w2')
        try:
            MaskAngle(w2,-100,20)
            self.fail("Should not have got here. Wrong angle.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace('w2')
        try:
            MaskAngle(w3,10,200)
            self.fail("Should not have got here. Wrong angle.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace('w3')
        try:
            MaskAngle(w4,100,20)
            self.fail("Should not have got here. Wrong angle.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace('w4')

if __name__ == '__main__':
    unittest.main()
