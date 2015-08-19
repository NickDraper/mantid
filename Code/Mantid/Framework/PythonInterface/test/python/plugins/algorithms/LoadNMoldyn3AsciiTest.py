#pylint: disable=too-many-public-methods

import unittest
from mantid.simpleapi import *
from mantid.api import *

class LoadNMoldyn3AsciiTest(unittest.TestCase):

    _cdl_filename = 'NaF_DISF.cdl'
    _dat_filename = 'WSH_test.dat'


    def test_load_fqt_from_cdl(self):
        """
        Load an F(Q, t) function from an nMOLDYN 3 .cdl file
        """

        moldyn_group = LoadNMoldyn3Ascii(Filename=self._cdl_filename,
                                         Functions=['Fqt-total'],
                                         OutputWorkspace='__LoadNMoldyn3Ascii_test')

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), 'NaF_DISF_Fqt-total')

        iqt_ws = moldyn_group[0]
        self.assertTrue(isinstance(iqt_ws, MatrixWorkspace))
        self.assertTrue(iqt_ws.getNumberHistograms(), 1)

        # X axis should be in TOF for an Fqt function
        units = iqt_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(units, 'TOF')


    def test_load_sqw_from_cdl(self):
        """
        Load an S(Q, w) function from a nMOLDYN 3 .cdl file
        """

        moldyn_group = LoadNMoldyn3Ascii(Filename=self._cdl_filename,
                                         Functions=['Sqw-total'],
                                         OutputWorkspace='__LoadNMoldyn3Ascii_test')

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(moldyn_group[0].name(), 'NaF_DISF_Sqw-total')

        sqw_ws = moldyn_group[0]
        self.assertTrue(isinstance(sqw_ws, MatrixWorkspace))
        self.assertTrue(sqw_ws.getNumberHistograms(), 1)

        # X axis should be in Energy for an Sqw function
        units = sqw_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(units, 'Energy')


    def test_load_from_dat(self):
        """
        Load a function from an nMOLDYN 3 .dat file
        """

        moldyn_ws = LoadNMoldyn3Ascii(Filename=self._dat_filename,
                                      OutputWorkspace='__LoadNMoldyn3Ascii_test')

        self.assertTrue(isinstance(moldyn_ws, MatrixWorkspace))
        self.assertTrue(moldyn_ws.getNumberHistograms(), 12)


    def test_function_validation_cdl(self):
        """
        Tests that the algorithm cannot be run when no functions are specified
        when loading a .cdl file.
        """
        self.assertRaises(RuntimeError,
                          LoadNMoldyn3Ascii,
                          Filename=self._cdl_filename,
                          OutputWorkspace='__LoadNMoldyn3Ascii_test')


    def test_function_validation_dat(self):
        """
        Tests that the algorithm cannot be run when functions are specified
        when loading a .dat file.
        """
        self.assertRaises(RuntimeError,
                          LoadNMoldyn3Ascii,
                          Filename=self._dat_filename,
                          Functions=['Sqw-total'],
                          OutputWorkspace='__LoadNMoldyn3Ascii_test')


if __name__ == '__main__':
    unittest.main()
