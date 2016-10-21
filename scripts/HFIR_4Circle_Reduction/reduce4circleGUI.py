#pylint: disable=invalid-name,relative-import,W0611,R0921,R0902,R0904,R0921,C0302,R0912
################################################################################
#
# MainWindow application for reducing HFIR 4-circle
#
################################################################################
import os
import sys
import math
import csv
import time
import datetime
import random
import numpy
from scipy.optimize import curve_fit


from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    from mantidqtpython import MantidQt
except ImportError as e:
    NO_SCROLL = True
else:
    NO_SCROLL = False

# import reduce4circleControl as r4c
import guiutility as gutil
import fourcircle_utility as hb3a
import plot3dwindow
from multi_threads_helpers import *


# import line for the UI python class
from ui_MainWindow import Ui_MainWindow


class MainWindow(QtGui.QMainWindow):
    """ Class of Main Window (top)
    """
    TabPage = {'View Raw Data': 2,
               'Calculate UB': 3,
               'UB Matrix': 4,
               'Peak Integration': 6}

    def __init__(self, parent=None):
        """ Initialization and set up
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # UI Window (from Qt Designer)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Make UI scrollable
        if NO_SCROLL is False:
            self._scrollbars = MantidQt.API.WidgetScrollbarDecorator(self)
            self._scrollbars.setEnabled(True)  # Must follow after setupUi(self)!

        self._init_widgets()

        # thread
        self._myIntegratePeaksThread = None
        self._addUBPeaksThread = None

        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())
        # config = ConfigService.Instance()
        # self._instrument = config["default.instrument"]

        # Event handling definitions
        # Top
        self.connect(self.ui.pushButton_setExp, QtCore.SIGNAL('clicked()'),
                     self.do_set_experiment)

        # Tab 'Data Access'
        self.connect(self.ui.pushButton_applySetup, QtCore.SIGNAL('clicked()'),
                     self.do_apply_setup)
        self.connect(self.ui.pushButton_browseLocalDataDir, QtCore.SIGNAL('clicked()'),
                     self.do_browse_local_spice_data)
        self.connect(self.ui.pushButton_testURLs, QtCore.SIGNAL('clicked()'),
                     self.do_test_url)
        self.connect(self.ui.pushButton_ListScans, QtCore.SIGNAL('clicked()'),
                     self.do_list_scans)
        self.connect(self.ui.pushButton_downloadExpData, QtCore.SIGNAL('clicked()'),
                     self.do_download_spice_data)
        self.connect(self.ui.comboBox_mode, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.do_change_data_access_mode)

        # Tab 'View Raw Data'
        self.connect(self.ui.pushButton_setScanInfo, QtCore.SIGNAL('clicked()'),
                     self.do_load_scan_info)
        self.connect(self.ui.pushButton_plotRawPt, QtCore.SIGNAL('clicked()'),
                     self.do_plot_pt_raw)
        self.connect(self.ui.pushButton_prevPtNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_prev_pt_raw)
        self.connect(self.ui.pushButton_nextPtNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_next_pt_raw)
        self.connect(self.ui.pushButton_showPtList, QtCore.SIGNAL('clicked()'),
                     self.show_scan_pt_list)
        self.connect(self.ui.pushButton_usePt4UB, QtCore.SIGNAL('clicked()'),
                     self.do_add_peak_to_find)
        self.connect(self.ui.pushButton_addROI, QtCore.SIGNAL('clicked()'),
                     self.do_add_roi)
        self.connect(self.ui.pushButton_saveROI, QtCore.SIGNAL('clicked()'),
                     self.do_apply_roi)
        self.connect(self.ui.pushButton_cancelROI, QtCore.SIGNAL('clicked()'),
                     self.do_del_roi)
        self.connect(self.ui.pushButton_nextScanNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_next_scan)
        self.connect(self.ui.pushButton_prevScanNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_prev_scan)
        self.connect(self.ui.pushButton_maskScanPt, QtCore.SIGNAL('clicked()'),
                     self.do_mask_pt_2d)
        self.connect(self.ui.pushButton_saveMask, QtCore.SIGNAL('clicked()'),
                     self.do_save_roi)

        # Tab 'calculate ub matrix'
        self.connect(self.ui.pushButton_findPeak, QtCore.SIGNAL('clicked()'),
                     self.do_find_peak)
        self.connect(self.ui.pushButton_addPeakToCalUB, QtCore.SIGNAL('clicked()'),
                     self.do_add_ub_peak)
        self.connect(self.ui.pushButton_calUB, QtCore.SIGNAL('clicked()'),
                     self.do_cal_ub_matrix)
        self.connect(self.ui.pushButton_acceptUB, QtCore.SIGNAL('clicked()'),
                     self.do_accept_ub)
        self.connect(self.ui.pushButton_indexUBPeaks, QtCore.SIGNAL('clicked()'),
                     self.do_index_ub_peaks)
        self.connect(self.ui.pushButton_deleteUBPeak, QtCore.SIGNAL('clicked()'),
                     self.do_del_ub_peaks)
        self.connect(self.ui.pushButton_clearUBPeakTable, QtCore.SIGNAL('clicked()'),
                     self.do_clear_ub_peaks)
        self.connect(self.ui.pushButton_resetPeakHKLs, QtCore.SIGNAL('clicked()'),
                     self.do_reset_ub_peaks_hkl)
        self.connect(self.ui.pushButton_selectAllPeaks, QtCore.SIGNAL('clicked()'),
                     self.do_select_all_peaks)
        self.connect(self.ui.pushButton_viewScan3D, QtCore.SIGNAL('clicked()'),
                     self.do_view_data_3d)
        self.connect(self.ui.pushButton_plotSelectedData, QtCore.SIGNAL('clicked()'),
                     self.do_view_data_set_3d)

        self.connect(self.ui.pushButton_refineUB, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub)
        self.connect(self.ui.pushButton_refineUBFFT, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub_fft)
        self.connect(self.ui.pushButton_findUBLattice, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub_lattice)

        # Tab 'Setup'
        self.connect(self.ui.pushButton_useDefaultDir, QtCore.SIGNAL('clicked()'),
                     self.do_setup_dir_default)
        self.connect(self.ui.pushButton_browseLocalCache, QtCore.SIGNAL('clicked()'),
                     self.do_browse_local_cache_dir)
        self.connect(self.ui.pushButton_browseWorkDir, QtCore.SIGNAL('clicked()'),
                     self.do_browse_working_dir)
        self.connect(self.ui.comboBox_instrument, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.do_change_instrument_name)

        # Tab 'UB Matrix'
        self.connect(self.ui.pushButton_showUB2Edit, QtCore.SIGNAL('clicked()'),
                     self.do_show_ub_in_box)
        self.connect(self.ui.pushButton_syncUB, QtCore.SIGNAL('clicked()'),
                     self.do_sync_ub)
        self.connect(self.ui.pushButton_saveUB, QtCore.SIGNAL('clicked()'),
                     self.do_save_ub)

        # Tab 'Merge'
        self.connect(self.ui.pushButton_addScanSliceView, QtCore.SIGNAL('clicked()'),
                     self.do_add_scans_merge)
        self.connect(self.ui.pushButton_mergeScans, QtCore.SIGNAL('clicked()'),
                     self.do_merge_scans)
        self.connect(self.ui.pushButton_integratePeaks, QtCore.SIGNAL('clicked()'),
                     self.do_integrate_peaks)
        self.connect(self.ui.pushButton_setupPeakIntegration, QtCore.SIGNAL('clicked()'),
                     self.do_switch_tab_peak_int)
        self.connect(self.ui.pushButton_refreshMerged, QtCore.SIGNAL('clicked()'),
                     self.do_refresh_merged_scans_table)
        self.connect(self.ui.pushButton_plotMergedScans, QtCore.SIGNAL('clicked()'),
                     self.do_view_merged_scans_3d)
        self.connect(self.ui.pushButton_showUB, QtCore.SIGNAL('clicked()'),
                     self.do_view_ub)
        self.connect(self.ui.checkBox_mergeScanTabLorentzCorr, QtCore.SIGNAL('stateChanged(int)'),
                     self.evt_apply_lorentz_correction_mt)
        self.connect(self.ui.pushButton_exportPeaks, QtCore.SIGNAL('clicked()'),
                     self.do_export_to_fp)
        self.connect(self.ui.pushButton_selectAllScans2Merge, QtCore.SIGNAL('clicked()'),
                     self.do_select_merged_scans)
        self.connect(self.ui.pushButton_indexMergedScans, QtCore.SIGNAL('clicked()'),
                     self.do_set_peaks_hkl)
        self.connect(self.ui.pushButton_applyKShift, QtCore.SIGNAL('clicked()'),
                     self.do_apply_k_shift)

        # Tab 'Integrate Peaks'
        self.connect(self.ui.pushButton_integratePt, QtCore.SIGNAL('clicked()'),
                     self.do_integrate_per_pt)
        self.connect(self.ui.comboBox_ptCountType, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.do_plot_pt_peak)

        self.connect(self.ui.pushButton_integratePeak, QtCore.SIGNAL('clicked()'),
                     self.do_integrate_peak)

        self.connect(self.ui.pushButton_fitBkgd, QtCore.SIGNAL('clicked()'),
                     self.do_fit_bkgd)
        self.connect(self.ui.pushButton_handPickBkgd, QtCore.SIGNAL('clicked()'),
                     self.do_manual_bkgd)
        self.connect(self.ui.pushButton_calBkgd, QtCore.SIGNAL('clicked()'),
                     self.do_cal_background)

        # Tab survey
        self.connect(self.ui.pushButton_survey, QtCore.SIGNAL('clicked()'),
                     self.do_survey)
        self.connect(self.ui.pushButton_saveSurvey, QtCore.SIGNAL('clicked()'),
                     self.do_save_survey)
        self.connect(self.ui.pushButton_loadSurvey, QtCore.SIGNAL('clicked()'),
                     self.do_load_survey)
        self.connect(self.ui.pushButton_viewSurveyPeak, QtCore.SIGNAL('clicked()'),
                     self.do_view_survey_peak)
        self.connect(self.ui.pushButton_addPeaksToRefine, QtCore.SIGNAL('clicked()'),
                     self.do_add_peaks_for_ub)
        self.connect(self.ui.pushButton_selectAllSurveyPeaks, QtCore.SIGNAL('clicked()'),
                     self.do_select_all_survey)
        self.connect(self.ui.pushButton_sortInfoTable, QtCore.SIGNAL('clicked()'),
                     self.do_filter_sort_survey_table)
        self.connect(self.ui.pushButton_clearSurvey, QtCore.SIGNAL('clicked()'),
                     self.do_clear_survey)

        self.connect(self.ui.lineEdit_numSurveyOutput, QtCore.SIGNAL('editingFinished()'),
                     self.evt_show_survey)
        self.connect(self.ui.lineEdit_numSurveyOutput, QtCore.SIGNAL('returnPressed()'),
                     self.evt_show_survey)
        self.connect(self.ui.lineEdit_numSurveyOutput, QtCore.SIGNAL('textEdited(const QString&)'),
                     self.evt_show_survey)

        # Tab k-shift vector
        self.connect(self.ui.pushButton_addKShift, QtCore.SIGNAL('clicked()'),
                     self.do_add_k_shift_vector)

        # Menu
        self.connect(self.ui.actionExit, QtCore.SIGNAL('triggered()'),
                     self.menu_quit)

        self.connect(self.ui.actionSave_Session, QtCore.SIGNAL('triggered()'),
                     self.save_current_session)
        self.connect(self.ui.actionLoad_Session, QtCore.SIGNAL('triggered()'),
                     self.load_session)

        # Validator ... (NEXT)

        # Declaration of class variable
        # some configuration
        self._homeSrcDir = os.getcwd()
        self._homeDir = os.getcwd()

        # Control
        self._myControl = r4c.CWSCDReductionControl(self._instrument)
        self._allowDownload = True
        self._dataAccessMode = 'Download'
        self._surveyTableFlag = True
        self._ubPeakTableFlag = True

        # Sub window
        self._my3DWindow = None
        self._refineConfigWindow = None

        # Timing and thread 'global'
        self._startMeringScans = time.clock()
        self._errorMessageEnsemble = ''

        # QSettings
        self.load_settings()

        return

    @property
    def controller(self):
        """ Parameter controller
        """
        assert self._myControl is not None, 'Controller cannot be None.'
        assert isinstance(self._myControl, r4c.CWSCDReductionControl), \
            'My controller must be of type %s, but not %s.' % ('CWSCDReductionControl',
                                                               self._myControl.__class__.__name__)

        return self._myControl

    def evt_show_survey(self):
        """
        Show survey result
        :return:
        """
        if self.ui.tableWidget_surveyTable.rowCount() == 0:
            # do nothing if the table is empty
            return

        max_number = int(self.ui.lineEdit_numSurveyOutput.text())
        if max_number != self.ui.tableWidget_surveyTable.rowCount():
            # re-show survey
            self.ui.tableWidget_surveyTable.remove_all_rows()
            self.ui.tableWidget_surveyTable.show_reflections(max_number)

        return

    def _init_widgets(self):
        """ Initialize the table widgets
        :return:
        """
        # Table widgets
        self.ui.tableWidget_peaksCalUB.setup()
        self.ui.tableWidget_ubMatrix.setup()
        self.ui.tableWidget_surveyTable.setup()
        self.ui.tableWidget_peakIntegration.setup()
        self.ui.tableWidget_mergeScans.setup()
        self.ui.tableWidget_ubInUse.setup()
        self.ui.tableWidget_kShift.setup()

        # Radio buttons
        self.ui.radioButton_hklFromSPICE.setChecked(True)
        self.ui.radioButton_indexFromCalUB.setChecked(False)

        # combo-box
        self.ui.comboBox_kVectors.clear()
        self.ui.comboBox_kVectors.addItem('0: (0, 0, 0)')

        # tab
        self.ui.tabWidget.setCurrentIndex(0)

        self.ui.radioButton_ubMantidStyle.setChecked(True)
        self.ui.lineEdit_numSurveyOutput.setText('50')
        self.ui.checkBox_loadHKLfromFile.setChecked(True)
        self.ui.checkBox_sortDescending.setChecked(False)
        self.ui.radioButton_sortByCounts.setChecked(True)

        # Tab 'Access'
        self.ui.lineEdit_url.setText('http://neutron.ornl.gov/user_data/hb3a/')
        self.ui.comboBox_mode.setCurrentIndex(0)
        self.ui.lineEdit_localSpiceDir.setEnabled(True)
        self.ui.pushButton_browseLocalDataDir.setEnabled(True)

        return

    def _build_peak_info_list(self):
        """ Build a list of PeakInfo to build peak workspace
        :return:
        """
        # Collecting all peaks that will be used to refine UB matrix
        row_index_list = self.ui.tableWidget_peaksCalUB.get_selected_rows(True)
        if len(row_index_list) < 3:
            err_msg = 'At least 3 peaks must be selected to refine UB matrix.' \
                      'Now it is only %d selected.' % len(row_index_list)
            self.pop_one_button_dialog(err_msg)
            return

        # loop over all peaks for peak information
        peak_info_list = list()
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status
        for i_row in row_index_list:
            scan_num, pt_num = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
            try:
                if pt_num < 0:
                    pt_num = None
                peak_info = self._myControl.get_peak_info(exp_number, scan_num, pt_num)
            except AssertionError as ass_err:
                raise RuntimeError('Unable to retrieve PeakInfo due to %s.' % str(ass_err))
            assert isinstance(peak_info, r4c.PeakProcessHelper)
            peak_info_list.append(peak_info)
        # END-FOR

        return peak_info_list

    def _show_refined_ub_result(self):
        """
        Show the result from refined UB matrix
        :return:
        """
        # Deal with result
        ub_matrix, lattice, lattice_error = self._myControl.get_refined_ub_matrix()
        # ub matrix
        self.ui.tableWidget_ubMatrix.set_from_matrix(ub_matrix)

        # lattice parameter
        assert isinstance(lattice, list)
        assert len(lattice) == 6
        self.ui.lineEdit_aUnitCell.setText('%.5f' % lattice[0])
        self.ui.lineEdit_bUnitCell.setText('%.5f' % lattice[1])
        self.ui.lineEdit_cUnitCell.setText('%.5f' % lattice[2])
        self.ui.lineEdit_alphaUnitCell.setText('%.5f' % lattice[3])
        self.ui.lineEdit_betaUnitCell.setText('%.5f' % lattice[4])
        self.ui.lineEdit_gammaUnitCell.setText('%.5f' % lattice[5])

        assert isinstance(lattice_error, list)
        assert len(lattice_error) == 6
        self.ui.lineEdit_aError.setText('%.5f' % lattice_error[0])
        self.ui.lineEdit_bError.setText('%.5f' % lattice_error[1])
        self.ui.lineEdit_cError.setText('%.5f' % lattice_error[2])
        self.ui.lineEdit_alphaError.setText('%.5f' % lattice_error[3])
        self.ui.lineEdit_betaError.setText('%.5f' % lattice_error[4])
        self.ui.lineEdit_gammaError.setText('%.5f' % lattice_error[5])

        return

    def closeEvent(self, QCloseEvent):
        """
        Close event
        :param QCloseEvent:
        :return:
        """
        print '[QCloseEvent=]', str(QCloseEvent)
        self.menu_quit()

    def do_accept_ub(self):
        """ Accept the calculated UB matrix and thus put to controller
        """
        # get the experiment number
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        # get matrix
        curr_ub = self.ui.tableWidget_ubMatrix.get_matrix()

        # synchronize UB matrix to tableWidget_ubInUse
        self.ui.tableWidget_ubInUse.set_from_matrix(curr_ub)

        # set UB matrix to system
        self._myControl.set_ub_matrix(exp_number, curr_ub)

        return

    def do_add_peak_to_find(self):
        """
        Add the scan/pt to the next
        :return:
        """
        # peak finding will use all points in the selected scan.
        scan_no = self.ui.lineEdit_run.text()
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['Calculate UB'])
        self.ui.lineEdit_scanNumber.setText(scan_no)

        return

    def do_add_peaks_for_ub(self):
        """ In tab-survey, merge selected scans, find peaks in merged data and
         switch to UB matrix calculation tab and add to table
        :return:
        """
        # get selected scans
        selected_row_index_list = self.ui.tableWidget_surveyTable.get_selected_rows(True)
        scan_number_list = self.ui.tableWidget_surveyTable.get_scan_numbers(selected_row_index_list)
        if len(scan_number_list) == 0:
            self.pop_one_button_dialog('No scan is selected.')
            return

        # get experiment number
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        if not status:
            self.pop_one_button_dialog('Unable to get experiment number\n  due to %s.' % str(exp_number))
            return

        # switch to tab-3
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['Calculate UB'])

        # prototype for a new thread
        self.ui.progressBar_add_ub_peaks.setRange(0, len(scan_number_list))
        self._addUBPeaksThread = AddPeaksThread(self, exp_number, scan_number_list)
        self._addUBPeaksThread.start()

        return

    def do_add_roi(self):
        """ Add region of interest to 2D image
        :return:
        """
        # set the button to next mode
        if str(self.ui.pushButton_addROI.text()) == 'Add ROI':
            # enter adding ROI mode
            self.ui.graphicsView.enter_roi_mode(state=True)
            # rename the button
            self.ui.pushButton_addROI.setText('Quit ROI')
        else:
            # quit editing ROI mode
            self.ui.graphicsView.enter_roi_mode(state=False)
            # rename the button
            self.ui.pushButton_addROI.setText('Add ROI')
        # END-IF-ELSE

        return

    def do_add_scans_merge(self):
        """ Add scans to merge
        :return:
        """
        # Get list of scans
        scan_list = gutil.parse_integer_list(str(self.ui.lineEdit_listScansSliceView.text()))
        if len(scan_list) == 0:
            self.pop_one_button_dialog('Scan list is empty.')

        # Set table
        self.ui.tableWidget_mergeScans.append_scans(scans=scan_list, allow_duplicate_scans=False)

        return

    def do_add_ub_peak(self):
        """ Add current to ub peaks
        :return:
        """
        # Add peak
        status, int_list = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                         self.ui.lineEdit_scanNumber])
        if status is False:
            self.pop_one_button_dialog(int_list)
            return
        exp_no, scan_no = int_list

        # Get HKL from GUI
        status, float_list = gutil.parse_float_editors([self.ui.lineEdit_H,
                                                        self.ui.lineEdit_K,
                                                        self.ui.lineEdit_L])
        if status is False:
            err_msg = float_list
            self.pop_one_button_dialog(err_msg)
            return
        h, k, l = float_list

        try:
            peak_info_obj = self._myControl.get_peak_info(exp_no, scan_no)
        except AssertionError as ass_err:
            self.pop_one_button_dialog(str(ass_err))
            return

        assert isinstance(peak_info_obj, r4c.PeakProcessHelper)
        if self.ui.checkBox_roundHKLInt.isChecked():
            h = math.copysign(1, h)*int(abs(h)+0.5)
            k = math.copysign(1, k)*int(abs(k)+0.5)
            l = math.copysign(1, l)*int(abs(l)+0.5)
        peak_info_obj.set_user_hkl(h, k, l)
        self.set_ub_peak_table(peak_info_obj)

        # Clear
        self.ui.lineEdit_scanNumber.setText('')

        self.ui.lineEdit_sampleQx.setText('')
        self.ui.lineEdit_sampleQy.setText('')
        self.ui.lineEdit_sampleQz.setText('')

        self.ui.lineEdit_H.setText('')
        self.ui.lineEdit_K.setText('')
        self.ui.lineEdit_L.setText('')

        return

    def do_apply_setup(self):
        """
        Purpose:
         - Apply the setup to controller.
        Requirements:
         - data directory, working directory must be given; but not necessarily correct
         - URL must be given; but not necessary to be correct
        :return:
        """
        # get data directory, working directory and data server URL from GUI
        local_data_dir = str(self.ui.lineEdit_localSpiceDir.text()).strip()
        working_dir = str(self.ui.lineEdit_workDir.text()).strip()
        data_server = str(self.ui.lineEdit_url.text()).strip()

        # set to my controller
        self._myControl.set_local_data_dir(local_data_dir)
        self._myControl.set_working_directory(working_dir)
        self._myControl.set_server_url(data_server, check_link=False)

        # check
        error_message = ''

        # local data dir
        if local_data_dir == '':
            error_message += 'Local data directory is not specified!\n'
        elif os.path.exists(local_data_dir) is False:
            try:
                os.mkdir(local_data_dir)
            except OSError as os_error:
                error_message += 'Unable to create local data directory %s due to %s.\n' % (
                    local_data_dir, str(os_error))
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: red;")
            else:
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: green;")
        else:
            self.ui.lineEdit_localSpiceDir.setStyleSheet("color: green;")
        # END-IF-ELSE

        # working directory
        if working_dir == '':
            error_message += 'Working directory is not specified!\n'
        elif os.path.exists(working_dir) is False:
            try:
                os.mkdir(working_dir)
                self.ui.lineEdit_workDir.setStyleSheet("color: green;")
            except OSError as os_error:
                error_message += 'Unable to create working directory %s due to %s.\n' % (
                    working_dir, str(os_error))
                self.ui.lineEdit_workDir.setStyleSheet("color: red;")
        else:
            self.ui.lineEdit_workDir.setStyleSheet("color: green;")
        # END-IF-ELSE

        # Set the URL red as it is better not check at this stage. Leave it to user
        self.ui.lineEdit_url.setStyleSheet("color: black;")

        if len(error_message) > 0:
            self.pop_one_button_dialog(error_message)

        return

    def do_browse_local_cache_dir(self):
        """ Browse local cache directory
        :return:
        """
        local_cache_dir = str(QtGui.QFileDialog.getExistingDirectory(self,
                                                                     'Get Local Cache Directory',
                                                                     self._homeSrcDir))

        # Set local directory to control
        status, error_message = self._myControl.set_local_data_dir(local_cache_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Synchronize to local data/spice directory and local cache directory
        if str(self.ui.lineEdit_localSpiceDir.text()) != '':
            prev_dir = str(self.ui.lineEdit_localSrcDir.text())
            self.pop_one_button_dialog('Local data directory was set up as %s' %
                                       prev_dir)
        self.ui.lineEdit_localSrcDir.setText(local_cache_dir)
        self.ui.lineEdit_localSpiceDir.setText(local_cache_dir)

        return

    def do_browse_local_spice_data(self):
        """ Browse local source SPICE data directory
        """
        src_spice_dir = str(QtGui.QFileDialog.getExistingDirectory(self, 'Get Directory',
                                                                   self._homeSrcDir))
        # Set local data directory to controller
        status, error_message = self._myControl.set_local_data_dir(src_spice_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        self._homeSrcDir = src_spice_dir
        self.ui.lineEdit_localSpiceDir.setText(src_spice_dir)

        return

    def do_browse_working_dir(self):
        """
        Browse and set up working directory
        :return:
        """
        work_dir = str(QtGui.QFileDialog.getExistingDirectory(self, 'Get Working Directory', self._homeDir))
        status, error_message = self._myControl.set_working_directory(work_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.ui.lineEdit_workDir.setText(work_dir)

        return

    def do_cal_background(self):
        """
        calculate background
        algorithm 1: average the selected pt's intensity.
        :return:
        """
        # get the selected rows in table
        background_rows = self.ui.tableWidget_peakIntegration.get_selected_rows(True)

        # loop through the selected rows and do the average
        intensity_sum = 0.
        for i_row in background_rows:
            tmp_intensity = self.ui.tableWidget_peakIntegration.get_cell_value(i_row, 2)
            intensity_sum += tmp_intensity

        # calculate background value
        background = intensity_sum / float(len(background_rows))

        # set the value
        self.ui.lineEdit_background.setText('%.7f' % background)

        return

    def do_cal_ub_matrix(self):
        """ Calculate UB matrix by 2 or 3 reflections
        """
        # Get reflections selected to calculate UB matrix
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        peak_info_list = list()
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status
        for i_row in xrange(num_rows):
            if self.ui.tableWidget_peaksCalUB.is_selected(i_row) is True:
                scan_num, pt_num = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
                if pt_num < 0:
                    pt_num = None
                peak_info = self._myControl.get_peak_info(exp_number, scan_num, pt_num)
                assert isinstance(peak_info, r4c.PeakProcessHelper)
                peak_info_list.append(peak_info)
        # END-FOR

        # Get lattice
        status, ret_obj = self._get_lattice_parameters()
        if status is True:
            a, b, c, alpha, beta, gamma = ret_obj
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog(err_msg)
            return

        # Calculate UB matrix
        status, ub_matrix = self._myControl.calculate_ub_matrix(peak_info_list, a, b, c,
                                                                alpha, beta, gamma)

        # Deal with result
        if status is True:
            self.ui.tableWidget_ubMatrix.set_from_matrix(ub_matrix)

        else:
            err_msg = ub_matrix
            self.pop_one_button_dialog(err_msg)

        return

    def do_change_data_access_mode(self):
        """ Change data access mode between downloading from server and local
        Event handling methods
        :return:
        """
        new_mode = str(self.ui.comboBox_mode.currentText())
        self._dataAccessMode = new_mode

        if new_mode.startswith('Local') is True:
            self.ui.lineEdit_localSpiceDir.setEnabled(True)
            self.ui.pushButton_browseLocalDataDir.setEnabled(True)
            self.ui.lineEdit_url.setEnabled(False)
            self.ui.lineEdit_localSrcDir.setEnabled(False)
            self.ui.pushButton_browseLocalCache.setEnabled(False)
            self._allowDownload = False
        else:
            self.ui.lineEdit_localSpiceDir.setEnabled(False)
            self.ui.pushButton_browseLocalDataDir.setEnabled(False)
            self.ui.lineEdit_url.setEnabled(True)
            self.ui.lineEdit_localSrcDir.setEnabled(True)
            self.ui.pushButton_browseLocalCache.setEnabled(True)
            self._allowDownload = True

        return

    def do_change_instrument_name(self):
        """ Handing the event as the instrument name is changed
        :return:
        """
        new_instrument = str(self.ui.comboBox_instrument.currentText())
        self.pop_one_button_dialog('Change of instrument during data processing is dangerous.')
        status, error_message = self._myControl.set_instrument_name(new_instrument)
        if status is False:
            self.pop_one_button_dialog(error_message)

        return

    def do_clear_survey(self):
        """
        Clear survey and survey table.
        As myController does not store any survey information,
        there is no need to clear any data structure in myController
        :return:
        """
        # Clear table
        self.ui.tableWidget_surveyTable.remove_all_rows()
        self.ui.tableWidget_surveyTable.reset()

        return

    def do_clear_ub_peaks(self):
        """
        Clear all peaks in UB-Peak table
        :return:
        """
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        row_number_list = range(num_rows)
        self.ui.tableWidget_peaksCalUB.delete_rows(row_number_list)

        return

    def do_del_roi(self):
        """ Delete ROI
        :return:
        """
        self.ui.graphicsView.remove_roi()

        return

    def do_del_ub_peaks(self):
        """
        Delete a peak in UB-Peak table
        :return:
        """
        # Find out the lines to get deleted
        row_num_list = self.ui.tableWidget_peaksCalUB.get_selected_rows()

        # Delete
        self.ui.tableWidget_peaksCalUB.delete_rows(row_num_list)

        return

    def do_download_spice_data(self):
        """ Download SPICE data
        :return:
        """
        # Check scans to download
        scan_list_str = str(self.ui.lineEdit_downloadScans.text())
        if len(scan_list_str) > 0:
            # user specifies scans to download
            valid, scan_list = hb3a.parse_int_array(scan_list_str)
            if valid is False:
                error_message = scan_list
                self.pop_one_button_dialog(error_message)
        else:
            # Get all scans
            status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
            if status is False:
                self.pop_one_button_dialog(ret_obj)
                return
            exp_no = ret_obj
            assert isinstance(exp_no, int)
            server_url = str(self.ui.lineEdit_url.text())
            scan_list = hb3a.get_scans_list(server_url, exp_no, return_list=True)
        self.pop_one_button_dialog('Going to download scans %s.' % str(scan_list))

        # Check location
        destination_dir = str(self.ui.lineEdit_localSrcDir.text())
        status, error_message = self._myControl.set_local_data_dir(destination_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.pop_one_button_dialog('Spice files will be downloaded to %s.' % destination_dir)

        # Set up myControl for downloading data
        exp_no = int(self.ui.lineEdit_exp.text())
        self._myControl.set_exp_number(exp_no)

        server_url = str(self.ui.lineEdit_url.text())
        status, error_message = self._myControl.set_server_url(server_url)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Download
        self._myControl.download_data_set(scan_list)

        return

    def do_find_peak(self):
        """ Find peak in a given scan and record it
        """
        # Get experiment, scan and pt
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_scanNumber])
        if status is True:
            exp_no, scan_no = ret_obj
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # merge peak if necessary
        if self._myControl.has_merged_data(exp_no, scan_no) is False:
            status, err_msg = self._myControl.merge_pts_in_scan(exp_no, scan_no, [], 'q-sample')
            if status is False:
                self.pop_one_button_dialog(err_msg)

        # Find peak
        status, err_msg = self._myControl.find_peak(exp_no, scan_no)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return

        # Get information from the latest (integrated) peak
        if self.ui.checkBox_loadHKLfromFile.isChecked() is True:
            # This is the first time that in the workflow to get HKL from MD workspace
            peak_info = self._myControl.get_peak_info(exp_no, scan_no)
            try:
                peak_info.retrieve_hkl_from_spice_table()
            except RuntimeError as run_err:
                self.pop_one_button_dialog('Unable to locate peak info due to %s.' % str(run_err))
        # END-IF

        # Set up correct values to table tableWidget_peaksCalUB
        peak_info = self._myControl.get_peak_info(exp_no, scan_no)
        h, k, l = peak_info.get_spice_hkl()
        self.ui.lineEdit_H.setText('%.2f' % h)
        self.ui.lineEdit_K.setText('%.2f' % k)
        self.ui.lineEdit_L.setText('%.2f' % l)

        q_x, q_y, q_z = peak_info.get_peak_centre()
        self.ui.lineEdit_sampleQx.setText('%.5E' % q_x)
        self.ui.lineEdit_sampleQy.setText('%.5E' % q_y)
        self.ui.lineEdit_sampleQz.setText('%.5E' % q_z)

        return

    def do_fit_bkgd(self):
        """ Purpose: fit the Pt.-integrated peak intensity curve with Gaussian to find out the background
        :return:
        """
        def gauss(x, a, b, c):
            return c*numpy.exp(-(x-a)**2/b)

        def gauss4(x, a, b, c, d):
            return c*numpy.exp(-(x-a)**2/b)+d

        # get the curve
        vec_x, vec_y, vec_e = self.ui.graphicsView_integratedPeakView.get_xye()

        # fit Gaussian for starting value of a, b and c
        fit_result1 = curve_fit(gauss, vec_x, vec_y)
        popt = fit_result1[0]  # popt, pcov
        # gauss_fit = gauss(vec_x, popt[0], popt[1], popt[2])

        # fit Gaussian again including background
        p0 = [popt[0], popt[1], popt[2], 0.]
        fit_result2 = curve_fit(gauss4, vec_x, vec_y, sigma=vec_e,  p0=p0)
        popt2 = fit_result2[0]  # popt2, pcov2
        gauss_fit4 = gauss4(vec_x, popt2[0], popt2[1], popt2[2], popt2[3])

        # plot the result
        self.ui.graphicsView_integratedPeakView.add_plot_1d(vec_x, gauss_fit4, color='red', marker='-')

        # write out the result
        background_value = popt2[3]
        self.ui.lineEdit_background.setText('%.7f' % background_value)

        return

    def do_export_to_fp(self):
        """ Export selected reflections to Fullprof single crystal data file for analysis
        :return:
        """
        # get selected lines
        selected_rows = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        if len(selected_rows) == 0:
            self.pop_one_button_dialog('There isn\'t any peak selected.')
            return

        # get the file name
        fp_name = str(QtGui.QFileDialog.getSaveFileName(self, 'Save to Fullprof File'))

        # return due to cancel
        if len(fp_name) == 0:
            return

        # collect information
        exp_number = int(self.ui.lineEdit_exp.text())
        scan_number_list = list()
        for i_row in selected_rows:
            scan_number_list.append(self.ui.tableWidget_mergeScans.get_scan_number(i_row))

        # write
        user_header = str(self.ui.lineEdit_fpHeader.text())
        try:
            status, file_content = self._myControl.export_to_fullprof(exp_number, scan_number_list,
                                                                      user_header, fp_name)
            self.ui.plainTextEdit_fpContent.setPlainText(file_content)
            if status is False:
                error_msg = file_content
                if error_msg.startswith('Peak index error'):
                    error_msg = 'You may forget to index peak\n' + error_msg
                self.pop_one_button_dialog(error_msg)
        except AssertionError as a_err:
            self.pop_one_button_dialog(str(a_err))
            return

        return

    def do_filter_sort_survey_table(self):
        """
        Sort and filter survey table by specified field
        Requirements:
        Guarantees: the original table is cleared and a new list is appended
        :return:
        """
        # Get column name
        if self.ui.radioButton_sortByScan.isChecked():
            column_name = 'Scan'
        elif self.ui.radioButton_sortByCounts.isChecked():
            column_name = 'Max Counts'
        else:
            self.pop_one_button_dialog('No column is selected to sort.')
            return

        # Get filters
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_filterScanLower,
                                                        self.ui.lineEdit_filterScanUpper],
                                                       allow_blank=True)

        # return with error
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return

        # set up default with return as None
        start_scan_number = ret_obj[0]
        if start_scan_number is None:
            start_scan_number = 0
        end_scan_number = ret_obj[1]
        if end_scan_number is None:
            end_scan_number = sys.maxint

        status, ret_obj = gutil.parse_float_editors([self.ui.lineEdit_filterCountsLower,
                                                     self.ui.lineEdit_filterCountsUpper],
                                                    allow_blank=True)
        if status is False:
            # return with error message
            self.pop_one_button_dialog(ret_obj)
            return

        # set up default with return as None
        min_counts = ret_obj[0]
        if min_counts is None:
            min_counts = -0.0
        max_counts = ret_obj[1]
        if max_counts is None:
            max_counts = sys.float_info.max

        # filter and sort
        ascending_order = not self.ui.checkBox_sortDescending.isChecked()
        if ascending_order:
            sort_order = 0
        else:
            sort_order = 1
        self.ui.tableWidget_surveyTable.filter_and_sort(start_scan_number, end_scan_number,
                                                        min_counts, max_counts,
                                                        column_name, sort_order)

        return

    def do_integrate_peak(self):
        """ Integrate a peak in tab peak integration
        :return:
        """
        # only support the simple cuboid counts summing algorithm

        # get experiment number and scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_scanIntegratePeak],
                                                       allow_blank=False)
        if status is False:
            err_msg = ret_obj
            self.pop_one_button_dialog(err_msg)
            return

        # check table
        table_exp, table_scan = self.ui.tableWidget_peakIntegration.get_exp_info()
        if (table_exp, table_scan) != tuple(ret_obj):
            err_msg = 'Table has value of a different experiment/scan (%d/%d vs %d/%d). Integrate Pt. first!' \
                      '' % (table_exp, table_scan, ret_obj[0], ret_obj[1])
            self.pop_one_button_dialog(err_msg)
            return

        # integrate by take account of background value
        status, ret_obj = gutil.parse_float_editors(self.ui.lineEdit_background, allow_blank=True)
        assert status, ret_obj
        if ret_obj is None:
            background = 0.
        else:
            background = ret_obj
        peak_intensity = self.ui.tableWidget_peakIntegration.simple_integrate_peak(background)

        # write result to label
        norm_type = str(self.ui.comboBox_ptCountType.currentText())
        label_str = 'Experiment %d Scan %d: Peak intensity = %.7f, Normalized by %s, Background = %.7f.' \
                    '' % (table_exp, table_scan, peak_intensity, norm_type, background)
        self.ui.label_peakIntegraeInfo.setText(label_str)

        # set value to previous table
        self.ui.tableWidget_mergeScans.set_peak_intensity(None, table_scan, peak_intensity)

        return

    def do_integrate_per_pt(self):
        """
        Integrate and plot per Pt.
        :return:
        """
        # VZ-FUTURE: consider to compare and merge with method do_plot_pt_peak()
        # get experiment and scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_scanIntegratePeak])
        if not status:
            self.pop_one_button_dialog('Unable to integrate peak due to %s.' % ret_obj)
            return
        else:
            exp_number, scan_number = ret_obj

        normalization = str(self.ui.comboBox_ptCountType.currentText())
        if normalization.count('Time') > 0:
            norm_type = 'time'
        elif normalization.count('Monitor') > 0:
            norm_type = 'monitor'
        else:
            norm_type = ''

        # get peak center (weighted)
        status, ret_obj = self._myControl.calculate_peak_center(exp_number, scan_number)
        if status is False:
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
            return
        else:
            this_peak_centre = ret_obj

        # scale factor
        try:
            intensity_scale_factor = float(self.ui.lineEdit_scaleFactor.text())
        except ValueError:
            intensity_scale_factor = 1.

        # get masked workspace
        mask_name = str(self.ui.comboBox_maskNames2.currentText())
        if mask_name.startswith('No Mask'):
            mask_name = None
        # mask workspace?
        mask_detectors = mask_name is not None

        status, ret_obj = self._myControl.integrate_scan_peaks(exp=exp_number,
                                                               scan=scan_number,
                                                               peak_radius=1.0,
                                                               peak_centre=this_peak_centre,
                                                               merge_peaks=False,
                                                               use_mask=mask_detectors,
                                                               mask_ws_name=mask_name,
                                                               normalization=norm_type,
                                                               scale_factor=intensity_scale_factor)

        # result due to error
        if status is False:
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
            return

        # process result
        pt_dict = ret_obj
        assert isinstance(pt_dict, dict)

        # clear table
        if self.ui.tableWidget_peakIntegration.rowCount() > 0:
            self.ui.tableWidget_peakIntegration.remove_all_rows()

        # Append new lines
        pt_list = sorted(pt_dict.keys())
        intensity_list = list()
        for pt in pt_list:
            pt_intensity = pt_dict[pt]
            intensity_list.append(pt_intensity)
            status, msg = self.ui.tableWidget_peakIntegration.append_pt(pt, -1, pt_intensity)
            if not status:
                error_msg = '[Error!] Unable to add Pt %d due to %s.' % (pt, msg)
                self.pop_one_button_dialog(error_msg)

        # Set up the experiment information to table
        self.ui.tableWidget_peakIntegration.set_exp_info(exp_number, scan_number)

        # Clear previous line and plot the Pt.
        self.ui.graphicsView_integratedPeakView.clear_all_lines()
        x_array = numpy.array(pt_list)
        y_array = numpy.array(intensity_list)
        self.ui.graphicsView_integratedPeakView.add_plot_1d(x_array, y_array,
                                                            color='blue')
        self.ui.graphicsView_integratedPeakView.set_smart_y_limit(y_array)

        return

    def do_integrate_peaks(self):
        """ Integrate selected peaks.  If any scan is not merged, then it will merge the scan first.
        Integrate peaks from the table of merged peak.
        It will so the simple cuboid integration with region of interest and background subtraction.
        :return:
        """
        # get rows to merge
        row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        if len(row_number_list) == 0:
            self.pop_one_button_dialog('No scan is selected for scan')
            return

        # get experiment number
        status, ret_obj = gutil.parse_integers_editors(self.ui.lineEdit_exp, allow_blank=False)
        if status:
            exp_number = ret_obj
        else:
            self.pop_one_button_dialog('Unable to get valid experiment number due to %s.' % ret_obj)
            return

        # mask workspace
        selected_mask = str(self.ui.comboBox_maskNames1.currentText())
        if selected_mask.lower().startswith('no mask'):
            selected_mask = ''
            mask_det = False
        else:
            mask_det = True

        # normalization
        norm_str = str(self.ui.comboBox_mergePeakNormType.currentText())
        if norm_str.lower().count('time') > 0:
            norm_type = 'time'
        elif norm_str.lower().count('monitor') > 0:
            norm_type = 'monitor'
        else:
            norm_type = ''

        # background Pt.
        status, num_bg_pt = gutil.parse_integers_editors(self.ui.lineEdit_numPt4Background, allow_blank=False)
        if not status or num_bg_pt == 0:
            self.pop_one_button_dialog('Number of Pt number for background must be larger than 0: %s!' % str(num_bg_pt))
            return

        # get the merging information: each item should be a tuple as (scan number, pt number list, merged)
        scan_number_list = list()
        for row_number in row_number_list:
            # get scan number and pt numbers
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            status, pt_number_list = self._myControl.get_pt_numbers(exp_number, scan_number)

            # set intensity to zero and error message
            if status is False:
                error_msg = 'Unable to get Pt. of experiment %d scan %d due to %s.' % (exp_number, scan_number,
                                                                                       str(pt_number_list))
                self.controller.set_peak_intensity(exp_number, scan_number, 0.)
                self.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0., False)
                self.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # merge all Pt. of the scan if they are not merged.
            merged = self.ui.tableWidget_mergeScans.get_merged_status(row_number)

            # add to list
            scan_number_list.append((scan_number, pt_number_list, merged))
            self.ui.tableWidget_mergeScans.set_status_by_row(row_number, 'Waiting')
        # END-FOR

        # set the progress bar
        self.ui.progressBar_mergeScans.setRange(0, len(scan_number_list))
        self.ui.progressBar_mergeScans.setValue(0)
        self.ui.progressBar_mergeScans.setTextVisible(True)
        self.ui.progressBar_mergeScans.setStatusTip('Hello')

        # process background setup
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_numPt4Background,
                                                        self.ui.lineEdit_numPt4BackgroundRight],
                                                       allow_blank=False)
        if not status:
            error_msg = str(ret_obj)
            self.pop_one_button_dialog(error_msg)
            return
        num_pt_bg_left = ret_obj[0]
        num_pt_bg_right = ret_obj[1]

        self._myIntegratePeaksThread = IntegratePeaksThread(self, exp_number, scan_number_list,
                                                            mask_det, selected_mask, norm_type,
                                                            num_pt_bg_left, num_pt_bg_right)
        self._myIntegratePeaksThread.start()

        return

    def do_index_ub_peaks(self):
        """ Index the peaks in the UB matrix peak table
        :return:
        """
        # Get UB matrix
        ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix()
        print '[Info] Get UB matrix from table ', ub_matrix

        # Index all peaks
        num_peaks = self.ui.tableWidget_peaksCalUB.rowCount()
        err_msg = ''
        for i_peak in xrange(num_peaks):
            scan_no = self.ui.tableWidget_peaksCalUB.get_exp_info(i_peak)[0]
            status, ret_obj = self._myControl.index_peak(ub_matrix, scan_number=scan_no)
            if status is True:
                hkl_value = ret_obj[0]
                hkl_error = ret_obj[1]
                self.ui.tableWidget_peaksCalUB.set_hkl(i_peak, hkl_value, hkl_error)
            else:
                err_msg += ret_obj + '\n'
        # END-FOR

        if len(err_msg) > 0:
            self.pop_one_button_dialog(err_msg)

        return

    def do_list_scans(self):
        """ List all scans available
        :return:
        """
        # Experiment number
        exp_no = int(self.ui.lineEdit_exp.text())

        access_mode = str(self.ui.comboBox_mode.currentText())
        if access_mode == 'Local':
            spice_dir = str(self.ui.lineEdit_localSpiceDir.text())
            message = hb3a.get_scans_list_local_disk(spice_dir, exp_no)
        else:
            url = str(self.ui.lineEdit_url.text())
            message = hb3a.get_scans_list(url, exp_no)

        self.pop_one_button_dialog(message)

        return

    def do_load_scan_info(self):
        """ Load SIICE's scan file
        :return:
        """
        # Get scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_run])
        if status is True:
            scan_no = ret_obj[0]
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog('Unable to get scan number in raw data tab due to %s.' % err_msg)
            return

        status, err_msg = self._myControl.load_spice_scan_file(exp_no=None, scan_no=scan_no)
        if status is False:
            self.pop_one_button_dialog(err_msg)

        return

    def do_load_survey(self):
        """ Load csv file containing experiment-scan survey's result.
        :return:
        """
        # check validity
        num_rows = int(self.ui.lineEdit_numSurveyOutput.text())

        # get the csv file
        file_filter = 'CSV Files (*.csv);;All Files (*.*)'
        csv_file_name = str(QtGui.QFileDialog.getOpenFileName(self, 'Open Exp-Scan Survey File', self._homeDir,
                                                              file_filter))
        if csv_file_name is None or len(csv_file_name) == 0:
            # return if file selection is cancelled
            return

        # call controller to load
        survey_tuple = self._myControl.load_scan_survey_file(csv_file_name)
        scan_sum_list = survey_tuple[1]
        assert isinstance(scan_sum_list, list), 'Returned value from load scan survey file must be a dictionary.'

        # set the table
        self.ui.tableWidget_surveyTable.set_survey_result(scan_sum_list)
        self.ui.tableWidget_surveyTable.remove_all_rows()
        self.ui.tableWidget_surveyTable.show_reflections(num_rows)

        return

    def do_plot_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Call to plot 2D
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_plot_pt_peak(self):
        """
        Integrate Pt. vs integrated intensity of detectors of that Pt. if it is not calculated before
        and then plot pt vs. integrated intensity on
        :return:
        """
        # Find out the current condition including (1) absolute (2) normalized by time
        # (3) normalized by monitor counts
        be_norm_str = str(self.ui.comboBox_ptCountType.currentText())

        norm_by_time = False
        norm_by_monitor = False

        if be_norm_str.startswith('Absolute'):
            # no normalization
            pass
        elif be_norm_str.count('Time') > 0:
            # norm by time
            norm_by_time = True
        elif be_norm_str.count('Monitor') > 0:
            # norm by monitor counts
            norm_by_monitor = True
        else:
            # exception!
            raise RuntimeError('Normalization mode %s is not supported.' % be_norm_str)

        # Integrate peak if the integrated peak workspace does not exist
        # get experiment number and scan number from the table
        exp_number, scan_number = self.ui.tableWidget_peakIntegration.get_exp_info()

        mask_name = str(self.ui.comboBox_maskNames2.currentText())
        masked = not mask_name.startswith('No Mask')
        has_integrated = self._myControl.has_integrated_peak(exp_number, scan_number, pt_list=None,
                                                             normalized_by_monitor=norm_by_monitor,
                                                             normalized_by_time=norm_by_time,
                                                             masked=masked)

        # integrate and/or plot
        if has_integrated:
            self.plot_pt_intensity()
        else:
            # VZ-FUTURE: implement this new method!
            self.do_integrate_per_pt()

        return

    def do_plot_prev_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Previous one
        pt_no -= 1
        if pt_no <= 0:
            self.pop_one_button_dialog('Pt. = 1 is the first one.')
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText('%d' % pt_no)

        # Plot
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_plot_prev_scan(self):
        """ Plot the previous scan while keeping the same Pt.
        :return:
        """
        # get current exp number, scan number and pt number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is False:
            error_msg = ret_obj
            self.pop_one_button_dialog(error_msg)
            return

        exp_number, scan_number, pt_number = ret_obj

        # get next scan
        scan_number -= 1
        if scan_number < 0:
            self.pop_one_button_dialog('Scan number cannot be negative!')
            return

        # plot
        try:
            self._plot_raw_xml_2d(exp_number, scan_number, pt_number)
        except RuntimeError as err:
            error_msg = 'Unable to plot next scan %d due to %s.' % (scan_number, str(err))
            self.pop_one_button_dialog(error_msg)
            return

        # update line edits
        self.ui.lineEdit_run.setText(str(scan_number))

        return

    def do_plot_next_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Next Pt
        pt_no += 1
        # get last Pt. number
        status, last_pt_no = self._myControl.get_pt_numbers(exp_no, scan_no)
        if status is False:
            error_message = last_pt_no
            self.pop_one_button_dialog('Unable to access Spice table for scan %d. Reason" %s.' % (
                scan_no, error_message))
        if pt_no > last_pt_no:
            self.pop_one_button_dialog('Pt. = %d is the last one of scan %d.' % (pt_no, scan_no))
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText('%d' % pt_no)

        # Plot
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_plot_next_scan(self):
        """ Plot the next scan while keeping the same Pt.
        :return:
        """
        # get current exp number, scan number and pt number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is False:
            error_msg = ret_obj
            self.pop_one_button_dialog(error_msg)
            return

        exp_number, scan_number, pt_number = ret_obj

        # get next scan
        scan_number += 1

        # plot
        try:
            self._plot_raw_xml_2d(exp_number, scan_number, pt_number)
        except RuntimeError as err:
            error_msg = 'Unable to plot next scan %d due to %s.' % (scan_number, str(err))
            self.pop_one_button_dialog(error_msg)
            return

        # update line edits
        self.ui.lineEdit_run.setText(str(scan_number))

        return

    def do_manual_bkgd(self):
        """ Select background by moving indicator manually
        :return:
        """
        if str(self.ui.pushButton_handPickBkgd.text()) == 'Customize Bkgd':
            # get into customize background mode.  add an indicator to the line and make it movable
            self.ui.graphicsView_integratedPeakView.add_background_indictor()

            # modify the push buttons status
            self.ui.pushButton_handPickBkgd.setText('Done')

        elif str(self.ui.pushButton_handPickBkgd.text()) == 'Done':
            # get out from the customize-background mode.  get the vertical indicator's position as background
            background_value = self.ui.graphicsView_integratedPeakView.get_indicator_position(self._bkgdIndicatorKey)

            # set the ground value to UI
            self._myControl.set_background_value(background_value)
            self.ui.lineEdit_bkgdValue.setText('%.7f' % background_value)

            # modify the push button status
            self.ui.pushButton_handPickBkgd.setText('Customize Bkgd')

        else:
            raise RuntimeError('Push button in state %s is not supported.' %
                               str(self.ui.pushButton_handPickBkgd.text()))

        return

    def do_mask_pt_2d(self):
        """ Mask a Pt and re-plot
        :return:
        """
        # get experiment, scan
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo],
                                                       allow_blank=False)
        if status:
            exp, scan, pt = ret_obj
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # get the mask
        status, ret_obj = self._myControl.get_region_of_interest(exp, scan)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return

        # create mask workspace
        status, error = self._myControl.generate_mask_workspace(exp, scan, ret_obj[0], ret_obj[1])
        if status is False:
            self.pop_one_button_dialog(error)
            return

        # re-load data file and mask
        self._myControl.load_spice_xml_file(exp, scan, pt)
        self._myControl.apply_mask(exp, scan, pt)

        # plot
        self._plot_raw_xml_2d(exp, scan, pt)

        return

    def do_merge_scans(self):
        """ Process data for slicing view
        :return:
        """
        # Get UB matrix
        ub_matrix = self.ui.tableWidget_ubInUse.get_matrix()
        self._myControl.set_ub_matrix(exp_number=None, ub_matrix=ub_matrix)

        # Warning
        self.pop_one_button_dialog('Data processing is long. Be patient!')

        # Process
        scan_row_list = self.ui.tableWidget_mergeScans.get_selected_scans()
        frame = str(self.ui.comboBox_mergeScanFrame.currentText())
        for tup2 in scan_row_list:
            #
            scan_no, i_row = tup2

            # Download/check SPICE file
            self._myControl.download_spice_file(None, scan_no, over_write=False)

            # Get some information
            status, pt_list = self._myControl.get_pt_numbers(None, scan_no)
            if status is False:
                err_msg = pt_list
                self.pop_one_button_dialog('Failed to get Pt. number: %s' % err_msg)
                return
            else:
                # Set information to table
                err_msg = self.ui.tableWidget_mergeScans.set_scan_pt(scan_no, pt_list)
                if len(err_msg) > 0:
                    self.pop_one_button_dialog(err_msg)

            self.ui.tableWidget_mergeScans.set_status_by_row(i_row, 'In Processing')

            status, ret_tup = self._myControl.merge_pts_in_scan(exp_no=None, scan_no=scan_no,
                                                                pt_num_list=[], target_frame=frame)

            # process output
            if status:
                assert len(ret_tup) == 2
                merge_status = 'Done'
                merged_name = ret_tup[0]
                group_name = ret_tup[1]
            else:
                merge_status = 'Failed. Reason: %s' % ret_tup
                merged_name = 'x'
                group_name = 'x'
                print merge_status

            # update table
            self.ui.tableWidget_mergeScans.set_status_by_row(i_row, merge_status)
            self.ui.tableWidget_mergeScans.set_ws_names_by_row(i_row, merged_name, group_name)

            # Sleep for a while
            time.sleep(0.1)
        # END-FOR

        return

    def do_refine_ub(self):
        """
        Refine UB matrix
        :return:
        """
        peak_info_list = self._build_peak_info_list()
        set_hkl_int = self.ui.checkBox_roundHKLInt.isChecked()

        # Refine UB matrix
        try:
            self._myControl.refine_ub_matrix_indexed_peaks(peak_info_list, set_hkl_int)
        except AssertionError as error:
            self.pop_one_button_dialog(str(error))
            return

        # show result
        self._show_refined_ub_result()

        return

    def do_refine_ub_lattice(self):
        """
        Calculate UB matrix constrained by lattice parameters
        :return:
        """
        import optimizelatticewindow as ol_window

        # launch the set up window
        self._refineConfigWindow = ol_window.OptimizeLatticeWindow(self)
        self._refineConfigWindow.show()

        return

    # add slot for UB refinement configuration window's signal to connect to
    @QtCore.pyqtSlot(dict)
    def refine_ub_lattice(self, val):
        """
        :param val:
        :return:
        """
        assert val == 1000, 'It is not an authorized signal.'

        # it is supposed to get the information back from the window
        unit_cell_type = self._refineConfigWindow.get_unit_cell_type()

        # get peak information list
        peak_info_list = self._build_peak_info_list()
        set_hkl_int = self.ui.checkBox_roundHKLInt.isChecked()

        # get the UB matrix value
        ub_src_tab = self._refineConfigWindow.get_ub_source()
        if ub_src_tab == 3:
            ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix_str()
        elif ub_src_tab == 4:
            ub_matrix = self.ui.tableWidget_ubInUse.get_matrix_str()
        else:
            self.pop_one_button_dialog('UB source tab %s is not supported.' % str(ub_src_tab))
            return

        # refine UB matrix by constraint on lattice parameters
        status, error_message = self._myControl.refine_ub_matrix_by_lattice(peak_info_list, set_hkl_int,
                                                                            ub_matrix, unit_cell_type)
        if status:
            # successfully refine the lattice and UB matrix
            self._show_refined_ub_result()
        else:
            self.pop_one_button_dialog(error_message)

        return

    def do_refine_ub_fft(self):
        """
        Refine UB matrix by calling FFT method
        :return:
        """
        # get PeakInfo list and check
        peak_info_list = self._build_peak_info_list()
        assert isinstance(peak_info_list, list), \
            'PeakInfo list must be a list but not %s.' % str(type(peak_info_list))
        assert len(peak_info_list) >= 3, \
            'PeakInfo must be larger or equal to 3 (.now given %d) to refine UB matrix' % len(peak_info_list)

        # get lattice range information
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_minD,
                                                        self.ui.lineEdit_maxD])
        if status is False:
            self.pop_one_button_dialog('Must specify Min D and max D to refine UB using FFT.')
            return
        min_d, max_d = ret_obj
        if (0 < min_d < max_d) is False:
            self.pop_one_button_dialog('Range of d is not correct!')
            return

        # friendly suggestion
        if len(peak_info_list) <= 9:
            self.pop_one_button_dialog('It is recommended to use at least 9 reflections'
                                       'to refine UB matrix without prior knowledge.')

        # refine
        self._myControl.refine_ub_matrix_least_info(peak_info_list, min_d, max_d)

        # set value
        self._show_refined_ub_result()

        return

    def do_refresh_merged_scans_table(self):
        """ Find the merged
        :return:
        """
        # find out the merged runs
        scan_info_tup_list = self._myControl.get_merged_scans()

        # get existing scan numbers
        existing_scan_number_list = self.ui.tableWidget_mergeScans.get_selected_scans()

        # append the row to the merged scan table
        for scan_info_tup in scan_info_tup_list:
            scan_number = scan_info_tup[1]
            # skip if existing
            if scan_number in existing_scan_number_list:
                continue

            exp_number = scan_info_tup[0]
            pt_number_list = scan_info_tup[2]
            ws_name = hb3a.get_merged_md_name('HB3A', exp_number, scan_number, pt_number_list)
            self.ui.tableWidget_mergeScans.add_new_merged_data(exp_number, scan_number, ws_name)

        return

    def do_reset_ub_peaks_hkl(self):
        """
        Reset user specified HKL value to peak table
        :return:
        """
        # get experiment number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        assert status, ret_obj
        exp_number = ret_obj[0]

        # reset all rows back to SPICE HKL
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        for i_row in xrange(num_rows):
            scan, pt = self.ui.tableWidget_peaksCalUB.get_scan_pt(i_row)
            if pt < 0:
                pt = None
            peak_info = self._myControl.get_peak_info(exp_number, scan, pt)
            h, k, l = peak_info.get_spice_hkl()
            self.ui.tableWidget_peaksCalUB.update_hkl(i_row, h, k, l)
        # END-FOR

        return

    def do_save_roi(self):
        """
        Save region of interest to a specific name and reflects in combo boxes for future use,
        especially used as a general ROI for multiple scans
        :return:
        """
        # get the experiment and scan value
        status, par_val_list = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run])
        assert status
        exp_number = par_val_list[0]
        scan_number = par_val_list[1]

        # get the user specified name from ...
        roi_name, ok = QtGui.QInputDialog.getText(self, 'Input Mask Name', 'Enter mask name:')

        # return if cancelled
        if not ok:
            return

        # get current ROI
        status, roi = self._myControl.get_region_of_interest(exp_number=exp_number, scan_number=scan_number)
        assert status, str(roi)
        roi_name = str(roi_name)
        self._myControl.save_roi(roi_name, roi)

        # set it to combo-box
        self.ui.comboBox_maskNames1.addItem(roi_name)
        self.ui.comboBox_maskNames2.addItem(roi_name)

        return

    def do_add_k_shift_vector(self):
        """ Add a k-shift vector
        :return:
        """
        # parse the k-vector
        status, ret_obj = gutil.parse_float_editors([self.ui.lineEdit_kX, self.ui.lineEdit_kY, self.ui.lineEdit_kZ],
                                                    allow_blank=False)
        if status is False:
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
            return
        else:
            k_x, k_y, k_z = ret_obj

        # add to controller
        k_index = self._myControl.add_k_shift_vector(k_x, k_y, k_z)

        # add to table and combo-box
        self.ui.tableWidget_kShift.add_k_vector(k_index, k_x, k_y, k_z)

        combo_message = '%d: (%.5f, %.5f, %.5f)' % (k_index, k_x, k_y, k_z)
        self.ui.comboBox_kVectors.addItem(combo_message)

        return

    def do_apply_k_shift(self):
        """ Apply k-shift to selected reflections
        :return:
        """
        # get the selected scans
        scan_list = list()
        selected_row_numbers = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        for row_index in selected_row_numbers:
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_index)
            scan_list.append(scan_number)

        # get the k-vector
        k_shift_message = str(self.ui.comboBox_kVectors.currentText())
        k_index = int(k_shift_message.split(':')[0])

        # set to controller
        self._myControl.set_k_shift(scan_list, k_index)

        # set to table
        for row_index in selected_row_numbers:
            self.ui.tableWidget_mergeScans.set_k_shift_index(row_index, k_index)

        return

    def do_apply_roi(self):
        """ Save current selection of region of interest
        :return:
        """
        lower_left_c, upper_right_c = self.ui.graphicsView.get_roi()

        status, par_val_list = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run])
        assert status, str(par_val_list)
        exp_number = par_val_list[0]
        scan_number = par_val_list[1]

        self._myControl.set_roi(exp_number, scan_number, lower_left_c, upper_right_c)

        return

    def do_save_survey(self):
        """
        Save the survey to a file
        :return:
        """
        # Get file name
        file_filter = 'CSV Files (*.csv);;All Files (*.*)'
        out_file_name = str(QtGui.QFileDialog.getSaveFileName(self, 'Save scan survey result',
                                                              self._homeDir, file_filter))

        # Save file
        self._myControl.save_scan_survey(out_file_name)

        return

    def do_save_ub(self):
        """ Save the in-use Matrix to an ASCII file
        :return:
        """
        # get file name
        file_filter = 'Data Files (*.dat);;All Files (*.*)'
        ub_file_name = str(QtGui.QFileDialog.getSaveFileName(self, 'ASCII File To Save UB Matrix', self._homeDir,
                                                             file_filter))

        # early return if user cancels selecting a file name to save
        if ub_file_name is None:
            return

        # get UB matrix
        in_use_ub = self.ui.tableWidget_ubInUse.get_matrix()
        ub_shape = in_use_ub.shape
        assert len(ub_shape) == 2 and ub_shape[0] == ub_shape[1] == 3

        # construct string
        ub_str = '# UB matrix\n# %s\n' % str(self.ui.label_ubInfoLabel.text())
        for i_row in xrange(3):
            for j_col in xrange(3):
                ub_str += '%.15f  ' % in_use_ub[i_row][j_col]
            ub_str += '\n'

        # write to file
        ub_file = open(ub_file_name, 'w')
        ub_file.write(ub_str)
        ub_file.close()

        return

    def do_select_all_peaks(self):
        """
        Purpose: select all peaks in table tableWidget_peaksCalUB
        :return:
        """
        self.ui.tableWidget_peaksCalUB.select_all_rows(self._ubPeakTableFlag)
        self._ubPeakTableFlag = not self._ubPeakTableFlag

        return

    def do_select_all_survey(self):
        """
        Select or de-select all rows in survey items
        :return:
        """
        self.ui.tableWidget_surveyTable.select_all_rows(self._surveyTableFlag)
        self._surveyTableFlag = not self._surveyTableFlag

        return

    def do_select_merged_scans(self):
        """ Select or deselect all rows in the merged scan table
        :return:
        """
        # get current status
        curr_state = str(self.ui.pushButton_selectAllScans2Merge.text()).startswith('Select')

        # select/deselect all
        self.ui.tableWidget_mergeScans.select_all_rows(curr_state)

        # set the text to the push button
        if curr_state:
            self.ui.pushButton_selectAllScans2Merge.setText('Deselect All')
        else:
            self.ui.pushButton_selectAllScans2Merge.setText('Select All')

        return

    def do_set_experiment(self):
        """ Set experiment
        :return:
        """
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        if status is True:
            exp_number = ret_obj[0]
            curr_exp_number = self._myControl.get_experiment()
            if curr_exp_number is not None and exp_number != curr_exp_number:
                self.pop_one_button_dialog('Changing experiment to %d.  Clean previous experiment %d\'s result'
                                           ' in Mantid manually.' % (exp_number, curr_exp_number))
            self._myControl.set_exp_number(exp_number)
            self.ui.lineEdit_exp.setStyleSheet('color: black')
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog('Unable to set experiment as %s' % err_msg)
            self.ui.lineEdit_exp.setStyleSheet('color: red')

        self.ui.tabWidget.setCurrentIndex(0)

        return

    def do_set_peaks_hkl(self):
        """ Set all peaks' HKL value in the merged-peak tab
        :return:
        """
        # get the parameters
        exp_number = int(self.ui.lineEdit_exp.text())
        hkl_from_spice = self.ui.radioButton_hklFromSPICE.isChecked()
        round_hkl = self.ui.checkBox_roundHKL.isChecked()

        # loop through all rows
        num_rows = self.ui.tableWidget_mergeScans.rowCount()
        for row_index in range(num_rows):
            # get scan number
            scan_i = self.ui.tableWidget_mergeScans.get_scan_number(row_index)

            # get or calculate HKL
            if hkl_from_spice:
                # get HKL from SPICE
                hkl_i = self._myControl.get_peak_info(exp_number, scan_number=scan_i).get_spice_hkl()
            else:
                # calculate HKL from SPICE
                ub_matrix = self._myControl.get_ub_matrix(exp_number)
                index_status, ret_tup = self._myControl.index_peak(ub_matrix, scan_i)
                if index_status:
                    hkl_i = ret_tup[0]
                else:
                    # unable to index peak. use fake hkl and set error message
                    hkl_i = [0, 0, 0]
                    error_msg = 'Scan %d: %s' % (scan_i, str(ret_tup))
                    self.ui.tableWidget_mergeScans.set_status(scan_i, error_msg)
                # END-IF-ELSE(index)
            # END-IF-ELSE (hkl_from_spice)

            # round
            if round_hkl:
                hkl_i = hb3a.round_hkl(hkl_i)

            # set & show
            self._myControl.get_peak_info(exp_number, scan_i).set_hkl(hkl_i)
            self.ui.tableWidget_mergeScans.set_hkl(row_index, hkl_i)
        # END-FOR

        return

    def do_setup_dir_default(self):
        """
        Set up default directory for storing data and working
        :return:
        """
        home_dir = os.path.expanduser('~')

        # Data cache directory
        data_cache_dir = os.path.join(home_dir, 'Temp/HB3ATest')
        self.ui.lineEdit_localSpiceDir.setText(data_cache_dir)
        self.ui.lineEdit_localSrcDir.setText(data_cache_dir)

        work_dir = os.path.join(data_cache_dir, 'Workspace')
        self.ui.lineEdit_workDir.setText(work_dir)

        return

    def set_ub_from_file(self):
        """ Get UB matrix from an Ascii file
        :return:
        """
        file_filter = 'Data Files (*.dat);;Text Files (*.txt);;All Files (*.*)'
        file_name = QtGui.QFileDialog.getOpenFileName(self, 'Open UB ASCII File', self._homeDir,
                                                      file_filter)
        # quit if cancelled
        if file_name is None:
            return

        # parse file
        ub_file = open(file_name, 'r')
        raw_lines = ub_file.readlines()
        ub_file.close()

        # convert to ub string
        ub_str = ''
        for line in raw_lines:
            line = line.strip()
            if len(line) == 0:
                # empty line
                continue
            elif line[0] == '#':
                # comment line
                continue
            else:
                # regular line
                ub_str += '%s ' % line

        # convert to matrix
        ub_matrix = gutil.convert_str_to_matrix(ub_str, (3, 3))

        return ub_matrix

    def set_ub_from_text(self):
        """ Purpose: Set UB matrix in use from plain text edit plainTextEdit_ubInput.
        Requirements:
          1. the string in the plain text edit must be able to be split to 9 floats by ',', ' ', '\t' and '\n'
        Guarantees: the matrix will be set up the UB matrix in use
        :return:
        """
        # get the string for ub matrix
        ub_str = str(self.ui.plainTextEdit_ubInput.toPlainText())
        status, ret_obj = gutil.parse_float_array(ub_str)

        # check whether the ub matrix in text editor is valid
        if status is False:
            # unable to parse to float arrays
            self.pop_one_button_dialog(ret_obj)
            return
        elif len(ret_obj) != 9:
            # number of floats is not 9
            self.pop_one_button_dialog('Requiring 9 floats for UB matrix.  Only %d are given.' % len(ret_obj))
            return

        # in good UB matrix format
        ub_str = ret_obj
        if self.ui.radioButton_ubMantidStyle.isChecked():
            # UB matrix in mantid style
            mantid_ub = gutil.convert_str_to_matrix(ub_str, (3, 3))

        elif self.ui.radioButton_ubSpiceStyle.isChecked():
            # UB matrix in SPICE style
            spice_ub = gutil.convert_str_to_matrix(ub_str, (3, 3))
            mantid_ub = r4c.convert_spice_ub_to_mantid(spice_ub)

        else:
            # not defined
            self.pop_one_button_dialog('Neither Mantid or SPICE-styled UB is checked!')
            return

        return mantid_ub

    def do_show_ub_in_box(self):
        """ Get UB matrix in table tableWidget_ubMergeScan and write to plain text edit plainTextEdit_ubInput
        :return:
        """
        ub_matrix = self.ui.tableWidget_ubInUse.get_matrix()

        text = ''
        for i in xrange(3):
            for j in xrange(3):
                text += '%.7f, ' % ub_matrix[i][j]
            text += '\n'

        self.ui.plainTextEdit_ubInput.setPlainText(text)

        return

    def do_survey(self):
        """
        Purpose: survey for the strongest reflections
        :return:
        """
        # Get experiment number
        exp_number = int(self.ui.lineEdit_exp.text())
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_surveyStartPt,
                                                        self.ui.lineEdit_surveyEndPt])
        if status is False:
            err_msg = ret_obj
            self.pop_one_button_dialog(err_msg)
        start_scan = ret_obj[0]
        end_scan = ret_obj[1]

        max_number = int(self.ui.lineEdit_numSurveyOutput.text())

        # Get value
        status, ret_obj = self._myControl.survey(exp_number, start_scan, end_scan)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return
        scan_sum_list = ret_obj
        self.ui.tableWidget_surveyTable.set_survey_result(scan_sum_list)
        self.ui.tableWidget_surveyTable.show_reflections(max_number)

        return

    def do_switch_tab_peak_int(self):
        """ Switch to tab 'Peak Integration' to set up and learn how to do peak integration
        :return:
        """
        # switch tab
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['Peak Integration'])

        # set up value
        selected_scans = self.ui.tableWidget_mergeScans.get_selected_scans()
        if len(selected_scans) > 0:
            # set the first one.  remember that the return is a list of tuple
            self.ui.lineEdit_scanIntegratePeak.setText(str(selected_scans[0][0]))

        return

    def do_sync_ub(self):
        """ Purpose: synchronize UB matrix in use with UB matrix calculated.
        Requirements: One of these must be given
        1. a valid UB matrix from tableWidget_ubMatrix
        2. an ascii file that contains 9 float
        3. from text edit
        :return:
        """
        # get the radio button that is checked
        if self.ui.radioButton_ubFromTab1.isChecked():
            # get ub matrix from tab 'Calculate UB Matrix'
            ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix()

        elif self.ui.radioButton_loadUBmatrix.isChecked():
            # load ub matrix from a file
            # VZ-FUTURE: Implement this next!
            ub_matrix = set_ub_from_file()

        elif self.ui.radioButton_ubFromList.isChecked():
            # load ub matrix from text editor
            ub_matrix = self.set_ub_from_text()

        else:
            raise

        # set to in-use UB matrix and control
        self.ui.tableWidget_ubInUse.set_from_matrix(ub_matrix)

        exp_no = int(str(self.ui.lineEdit_exp.text()))
        self._myControl.set_ub_matrix(exp_number=exp_no, ub_matrix=ub_matrix)

        return

    def do_test_url(self):
        """ Test whether the root URL provided specified is good
        """
        url = str(self.ui.lineEdit_url.text())

        url_is_good, err_msg = hb3a.check_url(url)
        if url_is_good is True:
            self.pop_one_button_dialog("URL %s is valid." % url)
            self.ui.lineEdit_url.setStyleSheet("color: green;")
        else:
            self.pop_one_button_dialog(err_msg)
            self.ui.lineEdit_url.setStyleSheet("color: read;")

        return url_is_good

    def do_view_data_set_3d(self):
        """
        Launch the sub window to view merged data in 3D.
        :return:
        """
        # get a list of rows that are selected
        row_index_list = self.ui.tableWidget_peaksCalUB.get_selected_rows(True)
        assert len(row_index_list) > 0, 'There is no row that is selected to view.'

        # get the information from the selected row
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status, 'Experiment number is not a valid integer.'

        # create window
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)

        for i_row in row_index_list:
            exp_info = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
            scan_number = exp_info[0]

            # prepare data
            ret_obj = self._prepare_view_merged(exp_number, scan_number)
            assert len(ret_obj) == 5
            md_file_name, weight_peak_centers, weight_peak_intensities, avg_peak_centre, avg_peak_intensity = ret_obj

            # add the 3D window
            self._my3DWindow.initialize_group('%s' % scan_number)
            self._my3DWindow.add_plot_by_file(md_file_name)
            self._my3DWindow.add_plot_by_array(weight_peak_centers, weight_peak_intensities)
            self._my3DWindow.add_plot_by_array(avg_peak_centre, avg_peak_intensity)
            self._my3DWindow.close_session()

        # END-FOR

        # Show
        self._my3DWindow.show()

        return

    def do_view_data_3d(self):
        """
        View merged scan data in 3D after FindPeaks
        :return:
        """
        # get experiment and scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_scanNumber])
        if status:
            exp_number = ret_obj[0]
            scan_number = ret_obj[1]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Check
        if self._myControl.has_merged_data(exp_number, scan_number) is False:
            self.pop_one_button_dialog('No merged MD workspace found for %d, %d' % (exp_number, scan_number))
            return

        # Generate data by writing out temp file
        ret_obj = self._prepare_view_merged(exp_number, scan_number)
        assert len(ret_obj) == 5
        md_file_name, weight_peak_centers, weight_peak_intensities, avg_peak_centre, avg_peak_intensity = ret_obj

        print 'Write file to %s' % md_file_name
        for i_peak in xrange(len(weight_peak_centers)):
            peak_i = weight_peak_centers[i_peak]
            print '%f, %f, %f' % (peak_i[0], peak_i[1], peak_i[2])
        print
        print avg_peak_centre

        # Plot
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)

        self._my3DWindow.add_plot_by_file(md_file_name)
        self._my3DWindow.add_plot_by_array(weight_peak_centers, weight_peak_intensities)
        self._my3DWindow.add_plot_by_array(avg_peak_centre, avg_peak_intensity)

        # Show
        self._my3DWindow.show()

        return

    def _prepare_view_merged(self, exp_number, scan_number):
        """
        Prepare the data to view 3D for merged scan
        :return:
        """
        # check
        assert isinstance(exp_number, int) and isinstance(scan_number, int)

        # get file name for 3D data
        base_file_name = 'md_%d.dat' % random.randint(1, 1001)
        md_file_name = self._myControl.export_md_data(exp_number, scan_number, base_file_name)
        peak_info = self._myControl.get_peak_info(exp_number, scan_number)

        # peak center
        weight_peak_centers, weight_peak_intensities = peak_info.get_weighted_peak_centres()
        qx, qy, qz = peak_info.get_peak_centre()

        # convert from list to ndarray
        num_pt_peaks = len(weight_peak_centers)
        assert num_pt_peaks == len(weight_peak_intensities)

        pt_peak_centre_array = numpy.ndarray(shape=(num_pt_peaks, 3), dtype='float')
        pt_peak_intensity_array = numpy.ndarray(shape=(num_pt_peaks,), dtype='float')
        for i_pt in xrange(num_pt_peaks):
            for j in xrange(3):
                pt_peak_centre_array[i_pt][j] = weight_peak_centers[i_pt][j]
            pt_peak_intensity_array[i_pt] = weight_peak_intensities[i_pt]

        avg_peak_centre = numpy.ndarray(shape=(1, 3), dtype='float')
        avg_peak_intensity = numpy.ndarray(shape=(1,), dtype='float')
        avg_peak_centre[0][0] = qx
        avg_peak_centre[0][1] = qy
        avg_peak_centre[0][2] = qz
        # integrated peak intensity
        avg_peak_intensity[0] = sum(pt_peak_intensity_array)

        return md_file_name, pt_peak_centre_array, pt_peak_intensity_array, avg_peak_centre, avg_peak_intensity

    def do_view_merged_scans_3d(self):
        """ Get selected merged scan and plot them individually in 3D
        :return:
        """
        # collect the selected rows and thus workspace names
        row_index_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        exp_scan_list = list()
        for row_index in row_index_list:
            exp_number, scan_number = self.ui.tableWidget_mergeScans.get_exp_scan(row_index)
            pt_number_list = self._myControl.get_pt_numbers(exp_number, scan_number)
            md_data = self._myControl.get_merged_data(exp_number, scan_number, pt_number_list)
            exp_scan_list.append((scan_number, md_data))

        # initialize 3D plot
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)
        self._my3DWindow.set_merged_data_set(exp_scan_list)
        self._my3DWindow.show()

        return

    def do_view_ub(self):
        """ View UB matrix in tab 'UB matrix'
        :return:
        """
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['UB Matrix'])

        return

    def do_view_survey_peak(self):
        """ View selected peaks from survey table
        Requirements: one and only 1 run is selected
        Guarantees: the scan number and pt number that are selected will be set to
            tab 'View Raw' and the tab is switched.
        :return:
        """
        # get values
        try:
            scan_num, pt_num = self.ui.tableWidget_surveyTable.get_selected_run_surveyed()
        except RuntimeError, err:
            self.pop_one_button_dialog(str(err))
            return

        # clear selection
        self.ui.tableWidget_surveyTable.select_all_rows(False)

        # switch tab
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['View Raw Data'])
        self.ui.lineEdit_run.setText(str(scan_num))
        self.ui.lineEdit_rawDataPtNo.setText(str(pt_num))

        return

    def evt_apply_lorentz_correction_mt(self):
        """
        Apply Lorentz corrections to the integrated peak intensities of all the selected peaks.
        :return:
        """
        # get experiment number
        exp_number = int(self.ui.lineEdit_exp.text())

        # select rows
        selected_rows = self.ui.tableWidget_mergeScans.get_selected_rows(True)

        # apply for each row selected for Lorentz correction
        for row_number in selected_rows:
            # get scan number
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            # get peak information object
            peak_info_obj = self._myControl.get_peak_info(exp_number, scan_number)
            # get intensity
            peak_intensity = peak_info_obj.get_intensity()
            # get Q-vector of the peak center and calculate |Q| from it
            q = peak_info_obj.get_peak_centre_v3d().norm()
            # get wave length
            wavelength = self._myControl.get_wave_length(exp_number, [scan_number])
            self.ui.tableWidget_mergeScans.set_wave_length(row_number, wavelength)
            # get motor step (choose from omega, phi and chi)
            motor_move_tup = self._myControl.get_motor_step(exp_number, scan_number)
            self.ui.tableWidget_mergeScans.set_motor_info(row_number, motor_move_tup)
            motor_step = motor_move_tup[1]
            # apply the Lorentz correction to the intensity
            corrected = self._myControl.apply_lorentz_correction(peak_intensity, q, wavelength, motor_step)
            self.ui.tableWidget_mergeScans.set_peak_intensity(row_number, None, corrected, lorentz_corrected=True)
            self._myControl.set_peak_intensity(exp_number, scan_number, corrected)
        # END-FOR (row_number)

        return

    def pop_one_button_dialog(self, message):
        """ Pop up a one-button dialog
        :param message:
        :return:
        """
        assert isinstance(message, str)
        QtGui.QMessageBox.information(self, '4-circle Data Reduction', message)

        return

    def save_current_session(self, filename=None):
        """ Save current session/value setup to
        :return:
        """
        # Set up dictionary
        save_dict = dict()

        # Setup
        save_dict['lineEdit_localSpiceDir'] = str(self.ui.lineEdit_localSpiceDir.text())
        save_dict['lineEdit_url'] = str(self.ui.lineEdit_url.text())
        save_dict['lineEdit_workDir'] = str(self.ui.lineEdit_workDir.text())

        # Experiment
        save_dict['lineEdit_exp'] = str(self.ui.lineEdit_exp.text())
        save_dict['lineEdit_scanNumber'] = self.ui.lineEdit_scanNumber.text()

        # Lattice
        save_dict['lineEdit_a'] = str(self.ui.lineEdit_a.text())
        save_dict['lineEdit_b'] = str(self.ui.lineEdit_b.text())
        save_dict['lineEdit_c'] = str(self.ui.lineEdit_c.text())
        save_dict['lineEdit_alpha'] = str(self.ui.lineEdit_alpha.text())
        save_dict['lineEdit_beta'] = str(self.ui.lineEdit_beta.text())
        save_dict['lineEdit_gamma'] = str(self.ui.lineEdit_gamma.text())

        # Merge scan
        save_dict['lineEdit_listScansSliceView'] = str(self.ui.lineEdit_listScansSliceView.text())
        save_dict['lineEdit_baseMergeMDName'] = str(self.ui.lineEdit_baseMergeMDName.text())

        # Save to csv file
        if filename is None:
            # default
            save_dir = os.path.expanduser('~/.mantid/')
            if os.path.exists(save_dir) is False:
                os.mkdir(save_dir)
            filename = os.path.join(save_dir, 'session_backup.csv')
        # END-IF

        ofile = open(filename, 'w')
        writer = csv.writer(ofile)
        for key, value in save_dict.items():
            writer.writerow([key, value])
        ofile.close()

        return

    def load_session(self, filename=None):
        """
        To load a session, i.e., read it back:
        :param filename:
        :return:
        """
        if filename is None:
            filename = 'session_backup.csv'
            filename = os.path.join(os.path.expanduser('~/.mantid/'), filename)

        in_file = open(filename, 'r')
        reader = csv.reader(in_file)
        my_dict = dict(x for x in reader)

        # set the data from saved file
        for key, value in my_dict.items():
            if key.startswith('lineEdit') is True:
                self.ui.__getattribute__(key).setText(value)
            elif key.startswith('plainText') is True:
                self.ui.__getattribute__(key).setPlainText(value)
            elif key.startswith('comboBox') is True:
                self.ui.__getattribute__(key).setCurrentIndex(int(value))
            else:
                self.pop_one_button_dialog('Error! Widget name %s is not supported' % key)
        # END-FOR

        # set the experiment
        self._myControl.set_local_data_dir(str(self.ui.lineEdit_localSpiceDir.text()))
        self._myControl.set_working_directory(str(self.ui.lineEdit_workDir.text()))
        self._myControl.set_server_url(str(self.ui.lineEdit_url.text()))

        return

    def menu_quit(self):
        """

        :return:
        """
        self.save_settings()
        self.close()

    def show_scan_pt_list(self):
        """ Show the range of Pt. in a scan
        :return:
        """
        # Get parameters
        status, inp_list = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run])
        if status is False:
            self.pop_one_button_dialog(inp_list)
            return
        else:
            exp_no = inp_list[0]
            scan_no = inp_list[1]

        status, ret_obj = self._myControl.get_pt_numbers(exp_no, scan_no)

        # Form message
        if status is False:
            # Failed to get Pt. list
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
        else:
            # Form message
            pt_list = sorted(ret_obj)
            num_pts = len(pt_list)
            info = 'Exp %d Scan %d has %d Pt. ranging from %d to %d.\n' % (exp_no, scan_no, num_pts,
                                                                           pt_list[0], pt_list[-1])
            num_miss_pt = pt_list[-1] - pt_list[0] + 1 - num_pts
            if num_miss_pt > 0:
                info += 'There are %d Pt. skipped.\n' % num_miss_pt

            self.pop_one_button_dialog(info)

        return

    def set_ub_peak_table(self, peak_info):
        """
        Set up the table of peaks to calculate UB matrix
        Requirements: peak_info is a valid PeakInfo instance
        :param peak_info:
        :return:
        """
        # Check requirements
        assert isinstance(peak_info, r4c.PeakProcessHelper)

        # Get data
        exp_number, scan_number = peak_info.get_experiment_info()
        h, k, l = peak_info.get_spice_hkl()
        q_x, q_y, q_z = peak_info.get_peak_centre()
        m1 = self._myControl.get_sample_log_value(exp_number, scan_number, 1, '_m1')
        wave_length = hb3a.convert_to_wave_length(m1_position=m1)

        # Set to table
        status, err_msg = self.ui.tableWidget_peaksCalUB.append_row(
            [scan_number, -1, h, k, l, q_x, q_y, q_z, False, m1, wave_length, ''])
        if status is False:
            self.pop_one_button_dialog(err_msg)

        return

    def save_settings(self):
        """
        Save settings (parameter set) upon quiting
        :return:
        """
        settings = QtCore.QSettings()

        # directories
        local_spice_dir = str(self.ui.lineEdit_localSpiceDir.text())
        settings.setValue("local_spice_dir", local_spice_dir)
        work_dir = str(self.ui.lineEdit_workDir.text())
        settings.setValue('work_dir', work_dir)

        # experiment number
        exp_num = str(self.ui.lineEdit_exp.text())
        settings.setValue('exp_number', exp_num)

        # lattice parameters
        lattice_a = str(self.ui.lineEdit_a.text())
        settings.setValue('a', lattice_a)
        lattice_b = str(self.ui.lineEdit_b.text())
        settings.setValue('b', lattice_b)
        lattice_c = str(self.ui.lineEdit_c.text())
        settings.setValue('c', lattice_c)
        lattice_alpha = str(self.ui.lineEdit_alpha.text())
        settings.setValue('alpha', lattice_alpha)
        lattice_beta = str(self.ui.lineEdit_beta.text())
        settings.setValue('beta', lattice_beta)
        lattice_gamma = str(self.ui.lineEdit_gamma.text())
        settings.setValue('gamma', lattice_gamma)

        return

    def load_settings(self):
        """
        Load QSettings from previous saved file
        :return:
        """
        settings = QtCore.QSettings()

        # directories
        try:
            spice_dir = settings.value('local_spice_dir', '')
            self.ui.lineEdit_localSpiceDir.setText(str(spice_dir))
            work_dir = settings.value('work_dir')
            self.ui.lineEdit_workDir.setText(str(work_dir))

            # experiment number
            exp_num = settings.value('exp_number')
            self.ui.lineEdit_exp.setText(str(exp_num))

            # lattice parameters
            lattice_a = settings.value('a')
            self.ui.lineEdit_a.setText(str(lattice_a))
            lattice_b = settings.value('b')
            self.ui.lineEdit_b.setText(str(lattice_b))
            lattice_c = settings.value('c')
            self.ui.lineEdit_c.setText(str(lattice_c))
            lattice_alpha = settings.value('alpha')
            self.ui.lineEdit_alpha.setText(str(lattice_alpha))
            lattice_beta = settings.value('beta')
            self.ui.lineEdit_beta.setText(str(lattice_beta))
            lattice_gamma = settings.value('gamma')
            self.ui.lineEdit_gamma.setText(str(lattice_gamma))
        except TypeError as err:
            self.pop_one_button_dialog(str(err))
            return

        return

    def _get_lattice_parameters(self):
        """
        Get lattice parameters from GUI
        :return: (Boolean, Object).  True, 6-tuple as a, b, c, alpha, beta, gamm
                                     False: error message
        """
        status, ret_list = gutil.parse_float_editors([self.ui.lineEdit_a,
                                                      self.ui.lineEdit_b,
                                                      self.ui.lineEdit_c,
                                                      self.ui.lineEdit_alpha,
                                                      self.ui.lineEdit_beta,
                                                      self.ui.lineEdit_gamma])
        if status is False:
            err_msg = ret_list
            err_msg = 'Unable to parse unit cell due to %s' % err_msg
            return False, err_msg

        a, b, c, alpha, beta, gamma = ret_list

        return True, (a, b, c, alpha, beta, gamma)

    def _plot_raw_xml_2d(self, exp_no, scan_no, pt_no):
        """ Plot raw workspace from XML file for a measurement/pt.
        """
        # Check and load SPICE table file
        does_exist = self._myControl.does_spice_loaded(exp_no, scan_no)
        if does_exist is False:
            # Download data
            status, error_message = self._myControl.download_spice_file(exp_no, scan_no, over_write=False)
            if status is True:
                status, error_message = self._myControl.load_spice_scan_file(exp_no, scan_no)
                if status is False and self._allowDownload is False:
                    self.pop_one_button_dialog(error_message)
                    return
            else:
                self.pop_one_button_dialog(error_message)
                return
        # END-IF(does_exist)

        # Load Data for Pt's xml file
        does_exist = self._myControl.does_raw_loaded(exp_no, scan_no, pt_no)

        if does_exist is False:
            # Check whether needs to download
            status, error_message = self._myControl.download_spice_xml_file(scan_no, pt_no, exp_no=exp_no)
            if status is False:
                self.pop_one_button_dialog(error_message)
                return
            # Load SPICE xml file
            status, error_message = self._myControl.load_spice_xml_file(exp_no, scan_no, pt_no)
            if status is False:
                self.pop_one_button_dialog(error_message)
                return

        # Convert a list of vector to 2D numpy array for imshow()
        # Get data and plot
        raw_det_data = self._myControl.get_raw_detector_counts(exp_no, scan_no, pt_no)
        # raw_det_data = numpy.rot90(raw_det_data, 1)
        self.ui.graphicsView.clear_canvas()
        self.ui.graphicsView.add_plot_2d(raw_det_data, x_min=0, x_max=256, y_min=0, y_max=256,
                                         hold_prev_image=False)
        if self.ui.checkBox_keepRoi.isChecked():
            # get region of interest from control
            status, roi = self._myControl.get_region_of_interest(exp_no, scan_number=None)
            if status:
                self.ui.graphicsView.add_roi(roi[0], roi[1])
            else:
                error_msg = roi
                self.pop_one_button_dialog(error_msg)
        # END-IF

        # Information
        info = '%-10s: %d\n%-10s: %d\n%-10s: %d\n' % ('Exp', exp_no,
                                                      'Scan', scan_no,
                                                      'Pt', pt_no)
        self.ui.plainTextEdit_rawDataInformation.setPlainText(info)

        return

    def update_adding_peaks_status(self, exp_number, scan_number, progress):
        """
        Update the status for adding peak to UB matrix calculating
        :param exp_number:
        :param scan_number:
        :param progress:
        :return:
        """
        # show message to bar
        if scan_number < 0:
            message = 'Peak processing finished'
        else:
            message = 'Processing experiment %d scan %d starting from %s.' % (exp_number, scan_number,
                                                                              str(datetime.datetime.now()))
        self.ui.statusbar.showMessage(message)

        # update progress bar
        self.ui.progressBar_add_ub_peaks.setValue(progress)

        return

    def update_merge_status(self, exp_number, scan_number, sig_value, mode):
        """
        update the status of merging/integrating peaks
        :param exp_number:
        :param scan_number:
        :param sig_value:
        :param mode:
        :return:
        """
        if mode == 0:
            # start of processing one peak
            progress = int(sig_value - 0.5)
            if progress == 0:
                # run start
                self._startMeringScans = time.clock()
                self._errorMessageEnsemble = ''

            self.ui.progressBar_mergeScans.setValue(progress)

        elif mode == 1:
            # end of processing one peak
            intensity = sig_value

            # check intensity value
            is_error = False
            if intensity < 0:
                # set to status
                error_msg = 'Negative intensity: %.3f' % intensity
                self.ui.tableWidget_mergeScans.set_status(scan_no=scan_number, status=error_msg)
                # reset intensity to 0.
                intensity = 0.
                is_error = True

            # set the calculated peak intensity to _peakInfoDict
            status, error_msg = self._myControl.set_peak_intensity(exp_number, scan_number, intensity)
            if status:
                # set the value to table
                self.ui.tableWidget_mergeScans.set_peak_intensity(None, scan_number, intensity)
                if not is_error:
                    self.ui.tableWidget_mergeScans.set_status(scan_number, 'Done')
            else:
                self._errorMessageEnsemble += error_msg + '\n'
                self.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)

        elif mode == 2:
            # end of the whole run
            progress = int(sig_value+0.5)
            self.ui.progressBar_mergeScans.setValue(progress)

            merge_run_end = time.clock()

            elapsed = merge_run_end - self._startMeringScans
            message = 'Peak integration is over. Used %.2f seconds' % elapsed

            self.ui.statusbar.showMessage(message)

            # pop error message if there is any
            if len(self._errorMessageEnsemble) > 0:
                self.pop_one_button_dialog(self._errorMessageEnsemble)

            del self._myIntegratePeaksThread

        return

    def update_merge_error(self, exp_number, scan_number, error_msg):
        """
        Update the merge-scan table for error message
        :param exp_number:
        :param scan_number:
        :param error_msg:
        :return:
        """
        # check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(error_msg, str)

        # set intensity, state to table
        self.ui.tableWidget_mergeScans.set_peak_intensity(row_number=None, scan_number=scan_number,
                                                          peak_intensity=0., lorentz_corrected=False)
        self.ui.tableWidget_mergeScans.set_status(scan_no=scan_number, status=error_msg)

        # set peak value
        status, error_msg = self._myControl.set_peak_intensity(exp_number, scan_number, 0.)
        if not status:
            self.pop_one_button_dialog(error_msg)

        return

    def update_peak_added_info(self, int_msg, int_msg2):
        """
        Update the peak-being-added information
        :param int_msg:
        :param int_msg2:
        :return:
        """
        # get parameters passed
        exp_number = int_msg
        scan_number = int_msg2

        # get PeakInfo
        peak_info = self._myControl.get_peak_info(exp_number, scan_number)
        assert isinstance(peak_info, r4c.PeakProcessHelper)

        # retrieve and set HKL from spice table
        peak_info.retrieve_hkl_from_spice_table()

        # add to table
        self.set_ub_peak_table(peak_info)

        return
