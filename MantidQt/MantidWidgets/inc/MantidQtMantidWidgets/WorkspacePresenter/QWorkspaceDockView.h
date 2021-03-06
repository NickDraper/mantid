#ifndef MANTIDQT_MANTIDWIDGETS_QWORKSPACEDOCKVIEW_H
#define MANTIDQT_MANTIDWIDGETS_QWORKSPACEDOCKVIEW_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <MantidAPI/ExperimentInfo.h>
#include <MantidAPI/IAlgorithm_fwd.h>
#include <MantidAPI/IMDEventWorkspace_fwd.h>
#include <MantidAPI/IMDWorkspace.h>
#include <MantidAPI/IPeaksWorkspace_fwd.h>
#include <MantidAPI/ITableWorkspace_fwd.h>
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidAPI/WorkspaceGroup_fwd.h>

#include <MantidQtMantidWidgets/MantidSurfacePlotDialog.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h>
#include <QDockWidget>
#include <QMap>
#include <QMetaType>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

class QMainWindow;
class QLabel;
class QFileDialog;
class QLineEdit;
class QActionGroup;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

using TopLevelItems = std::map<std::string, Mantid::API::Workspace_sptr>;
Q_DECLARE_METATYPE(TopLevelItems)

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class MantidTreeWidgetItem;
class MantidTreeWidget;

/**
\class  QWorkspaceDockView
\author Lamar Moore
\date   24-08-2016
\version 1.0


Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS QWorkspaceDockView
    : public QDockWidget,
      public IWorkspaceDockView,
      public boost::enable_shared_from_this<QWorkspaceDockView> {
  Q_OBJECT
public:
  explicit QWorkspaceDockView(MantidQt::MantidWidgets::MantidDisplayBase *mui,
                              QMainWindow *parent);
  ~QWorkspaceDockView();
  void dropEvent(QDropEvent *de) override;
  void init() override;
  MantidQt::MantidWidgets::WorkspacePresenterWN_wptr
  getPresenterWeakPtr() override;

  MantidSurfacePlotDialog::UserInputSurface
  chooseContourPlotOptions(int nWorkspaces) const;
  MantidSurfacePlotDialog::UserInputSurface
  chooseSurfacePlotOptions(int nWorkspaces) const;

  SortDirection getSortDirection() const override;
  SortCriteria getSortCriteria() const override;
  void sortWorkspaces(SortCriteria criteria, SortDirection direction) override;

  MantidQt::MantidWidgets::StringList
  getSelectedWorkspaceNames() const override;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const override;

  bool askUserYesNo(const std::string &caption,
                    const std::string &message) const override;
  void showCriticalUserMessage(const std::string &caption,
                               const std::string &message) const override;

  void showLoadDialog() override;
  void showLiveDataDialog() override;
  void
  showRenameDialog(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void enableDeletePrompt(bool enable) override;
  bool isPromptDelete() const override;
  bool deleteConfirmation() const override;
  void
  deleteWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void clearView() override;
  std::string getFilterText() const override;
  SaveFileType getSaveFileType() const override;
  void saveWorkspace(SaveFileType type) override;
  void
  saveWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void filterWorkspaces(const std::string &filterText) override;
  void recordWorkspaceRename(const std::string &oldName,
                             const std::string &newName) override;
  void refreshWorkspaces() override;

  // Context Menu Handlers
  void popupContextMenu() override;
  void showWorkspaceData() override;
  void saveToProgram() override;
  void showInstrumentView() override;
  void plotSpectrum(bool showErrors) override;
  void showColourFillPlot() override;
  void showDetectorsTable() override;
  void showBoxDataTable() override;
  void showVatesGUI() override;
  void showMDPlot() override;
  void showListData() override;
  void showSpectrumViewer() override;
  void showSliceViewer() override;
  void showLogs() override;
  void showSampleMaterialWindow() override;
  void showAlgorithmHistory() override;
  void showTransposed() override;
  void convertToMatrixWorkspace() override;
  void convertMDHistoToMatrixWorkspace() override;
  void showSurfacePlot() override;
  void showContourPlot() override;

  bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                             const bool wait = true) override;

private:
  bool hasUBMatrix(const std::string &wsName);
  void addSaveMenuOption(QString algorithmString, QString menuEntryName = "");
  void setTreeUpdating(const bool state);
  inline bool isTreeUpdating() const { return m_treeUpdating; }
  void updateTree(const TopLevelItems &items) override;
  void populateTopLevel(const TopLevelItems &topLevelItems,
                        const QStringList &expanded);
  MantidTreeWidgetItem *
  addTreeEntry(const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
               QTreeWidgetItem *parent = NULL);
  bool shouldBeSelected(QString name) const;
  void createWorkspaceMenuActions();
  void createSortMenuActions();
  void setItemIcon(QTreeWidgetItem *item, const std::string &wsID);

  void addMatrixWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const;
  void addMDEventWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::IMDEventWorkspace_const_sptr &mdeventWS) const;
  void addMDHistoWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const;
  void addPeaksWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const;
  void addWorkspaceGroupMenuItems(
      QMenu *menu, const Mantid::API::WorkspaceGroup_const_sptr &groupWS) const;
  void addTableWorkspaceMenuItems(QMenu *menu) const;
  void addClearMenuItems(QMenu *menu, const QString &wsName);

  void excludeItemFromSort(MantidTreeWidgetItem *item);

  void setupWidgetLayout();
  void setupLoadButtonMenu();
  void setupConnections();

public slots:
  void clickedWorkspace(QTreeWidgetItem *, int);
  void saveWorkspaceCollection();
  void onClickDeleteWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem *item);
  void onClickSaveToProgram(const QString &name);
  void sortAscending();
  void sortDescending();
  void chooseByName();
  void chooseByLastModified();

protected slots:
  void popupMenu(const QPoint &pos);
  void workspaceSelected();

private slots:
  void handleShowSaveAlgorithm();
  void treeSelectionChanged();
  void onClickGroupButton();
  void onClickLoad();
  void onLoadAccept();
  void onClickLiveData();
  void onClickShowData();
  void onClickShowInstrument();
  void onClickShowBoxData();
  void onClickShowVates();
  void onClickShowMDPlot();
  void onClickShowListData();
  void onClickShowSpectrumViewer();
  void onClickShowSliceViewer();
  void onClickShowFileLog();
  void onClickSaveNexusWorkspace();
  void onClickShowTransposed();
  void onClickPlotSpectra();
  void onClickPlotSpectraErr();
  void onClickDrawColorFillPlot();
  void onClickShowDetectorTable();
  void onClickConvertToMatrixWorkspace();
  void onClickConvertMDHistoToMatrixWorkspace();
  void onClickShowAlgHistory();
  void onClickShowSampleMaterial();
  void onClickPlotSurface();
  void onClickPlotContour();
  void onClickClearUB();
  void incrementUpdateCount();
  void filterWorkspaceTree(const QString &text);

private:
  MantidQt::MantidWidgets::WorkspacePresenterVN_sptr m_presenter;

protected:
  MantidTreeWidget *m_tree;

private:
  QString selectedWsName;
  QPoint m_menuPosition;
  QString m_programName;
  MantidDisplayBase *const m_mantidUI;

  std::string m_filteredText;
  QPushButton *m_loadButton;
  QPushButton *m_saveButton;
  QMenu *m_loadMenu, *m_saveToProgram, *m_sortMenu, *m_saveMenu;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;
  QPushButton *m_sortButton;
  QLineEdit *m_workspaceFilter;
  QSignalMapper *m_programMapper;
  QActionGroup *m_sortChoiceGroup;
  QFileDialog *m_saveFolderDialog;

  // Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecErr,
      *m_showDetectors, *m_showBoxData, *m_showVatesGui, *m_showSpectrumViewer,
      *m_showSliceViewer, *m_colorFill, *m_showLogs, *m_showSampleMaterial,
      *m_showHist, *m_showMDPlot, *m_showListData, *m_saveNexus, *m_rename,
      *m_delete, *m_program, *m_ascendingSortAction, *m_descendingSortAction,
      *m_byNameChoice, *m_byLastModifiedChoice, *m_showTransposed,
      *m_convertToMatrixWorkspace, *m_convertMDHistoToMatrixWorkspace,
      *m_clearUB, *m_plotSurface, *m_plotContour;

  QAtomicInt m_updateCount;
  bool m_treeUpdating;
  bool m_promptDelete;
  QMainWindow *m_appParent;
  SaveFileType m_saveFileType;
  SortCriteria m_sortCriteria;
  SortDirection m_sortDirection;
  /// Temporarily keeps names of selected workspaces during tree update
  /// in order to restore selection after update
  QStringList m_selectedNames;
  /// Keep a map of renamed workspaces between updates
  QHash<QString, QString> m_renameMap;

private slots:
  void handleUpdateTree(const TopLevelItems &);
  void handleClearView();
signals:
  void signalClearView();
  void signalUpdateTree(const TopLevelItems &);
};
}
}
#endif // MANTIDQT_MANTIDWIDGETS_QWORKSPACEDOCKVIEW_H