#include "globals.h"
#include "ApplicationWindow.h"
#include "Graph3D.h"
#include "Matrix.h"
#include "Note.h"
#include "ProjectSerialiser.h"
#include "ScriptingWindow.h"
#include "TableStatistics.h"
#include "WindowFactory.h"

#include "Mantid/InstrumentWidget/InstrumentWindow.h"
#include "Mantid/MantidMatrixFunction.h"
#include "Mantid/MantidUI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtAPI/PlotAxis.h"
#include "MantidQtAPI/VatesViewerInterface.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidQtSpectrumViewer/SpectrumView.h"

#include <QTextCodec>
#include <QTextStream>

using namespace Mantid::API;
using namespace MantidQt::API;

// This C function is defined in the third party C lib minigzip.c
extern "C" {
void file_compress(const char *file, const char *mode);
}

ProjectSerialiser::ProjectSerialiser(ApplicationWindow *window)
    : window(window), m_windowCount(0) {}

/**
 * Save the current state of the application as a Mantid project file
 *
 * @param folder :: the folder instance to save
 * @param projectName :: the name of the project to write to
 * @param compress :: whether to compress the project (default false)
 */
void ProjectSerialiser::save(Folder *folder, const QString &projectName,
                             bool compress) {
  m_windowCount = 0;
  QFile fileHandle(projectName);

  // attempt to backup project files and check we can write
  if (!canBackupProjectFiles(&fileHandle, projectName) ||
      !canWriteToProject(&fileHandle, projectName)) {
    return;
  }

  QString text = serialiseProjectState(folder);
  saveProjectFile(&fileHandle, projectName, text, compress);
}

/**
 * Load the state of Mantid from a collection of lines read from a project file
 *
 * @param lines :: string of characters from a project file
 * @param fileVersion :: project file version used
 * @param isTopLevel :: whether this function is being called on a top level
 * 		folder. (Default True)
 */
void ProjectSerialiser::load(std::string lines, const int fileVersion,
                             const bool isTopLevel) {
  // If we're not the top level folder, read the folder settings and create the
  // folder
  // This is a legacy edgecase because folders are written
  // <folder>\tsettings\tgo\there
  if (!isTopLevel && lines.size() > 0) {
    std::vector<std::string> lineVec;
    boost::split(lineVec, lines, boost::is_any_of("\n"));

    std::string firstLine = lineVec.front();

    std::vector<std::string> values;
    boost::split(values, firstLine, boost::is_any_of("\t"));

    auto newFolder =
        new Folder(window->currentFolder(), QString::fromStdString(values[1]));
    newFolder->setBirthDate(QString::fromStdString(values[2]));
    newFolder->setModificationDate(QString::fromStdString(values[3]));

    if (values.size() > 4 && values[4] == "current")
      window->d_loaded_current = newFolder;

    auto fli = new FolderListItem(window->currentFolder()->folderListItem(),
                                  newFolder);
    newFolder->setFolderListItem(fli);

    window->d_current_folder = newFolder;

    // Remove the first line (i.e. the folder's settings line)
    lineVec.erase(lineVec.begin());
    lines = boost::algorithm::join(lineVec, "\n");
  }

  loadProjectSections(lines, fileVersion, isTopLevel);

  // We're returning to our parent folder, so set d_current_folder to our parent
  auto parent = dynamic_cast<Folder *>(window->currentFolder()->parent());
  if (!parent)
    window->d_current_folder = window->projectFolder();
  else
    window->d_current_folder = parent;
}

/**
 * Load sections of the project file back into Mantid
 *
 * This function looks at individual sections of the TSV project file
 * and loads the relevant windows etc.
 *
 * @param lines :: string of characters from a Mantid project file.
 * @param fileVersion :: version of the project file loaded
 * @param isTopLevel :: whether this is being called on a top level folder.
 */
void ProjectSerialiser::loadProjectSections(const std::string &lines,
                                            const int fileVersion,
                                            const bool isTopLevel) {
  // This now ought to be the regular contents of a folder. Parse as normal.
  TSVSerialiser tsv(lines);

  // If this is the top level folder of the project, we'll need to load the
  // workspaces before anything else.
  if (isTopLevel) {
    loadWorkspaces(tsv);
  }

  loadCurrentFolder(tsv);
  loadWindows(tsv, fileVersion);
  loadLogData(tsv);
  loadScriptWindow(tsv, fileVersion);
  loadAdditionalWindows(lines, fileVersion);

  // Deal with subfolders last.
  loadSubFolders(tsv, fileVersion);
}

/**
 * Load workspaces listed in the project file.
 *
 * This function should only be called once.
 *
 * @param tsv :: the TSVserialiser object for the project file
 */
void ProjectSerialiser::loadWorkspaces(const TSVSerialiser &tsv) {
  if (tsv.hasSection("mantidworkspaces")) {
    // There should only be one of these, so we only read the first.
    std::string workspaces = tsv.sections("mantidworkspaces").front();
    populateMantidTreeWidget(QString::fromStdString(workspaces));
  }
}

/** Load open project windows from the project file
 *
 * This uses the dynamic window factory to create the various different types
 * of windows.
 *
 * @param lines :: string of characters from a Mantid project file
 * @param fileVersion :: version of the project file loaded
 */
void ProjectSerialiser::loadWindows(const TSVSerialiser &tsv,
                                    const int fileVersion) {
  auto keys = WindowFactory::Instance().getKeys();
  // Work around for graph-table dependance. Graph3D's currently rely on
  // looking up tables. These must be loaded before the graphs, so work around
  // by loading in reverse alphabetical order.
  std::reverse(keys.begin(), keys.end());
  for (auto &classname : keys) {
    if (tsv.hasSection(classname)) {
      for (auto &section : tsv.sections(classname)) {
        WindowFactory::Instance().loadFromProject(classname, section, window,
                                                  fileVersion);
      }
    }
  }
}

/**
 * Load subfolders from the project file.
 *
 * @param tsv :: the TSVserialiser object for the project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::loadSubFolders(const TSVSerialiser &tsv,
                                       const int fileVersion) {
  if (tsv.hasSection("folder")) {
    auto folders = tsv.sections("folder");
    for (auto &it : folders) {
      load(it, fileVersion, false);
    }
  }
}

/**
 * Load the script window from the project file.
 *
 * @param tsv :: the TSVserialiser object for the project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::loadScriptWindow(const TSVSerialiser &tsv,
                                         const int fileVersion) {
  if (tsv.hasSection("scriptwindow")) {
    auto scriptSections = tsv.sections("scriptwindow");
    for (auto &it : scriptSections) {
      openScriptWindow(it, fileVersion);
    }
  }
}

/**
 * Load any log entries from the project file.
 *
 * @param tsv :: the TSVserialiser object for the project file
 */
void ProjectSerialiser::loadLogData(const TSVSerialiser &tsv) {
  if (tsv.hasSection("log")) {
    auto logSections = tsv.sections("log");
    for (auto &it : logSections) {
      window->currentFolder()->appendLogInfo(QString::fromStdString(it));
    }
  }
}

/**
 * Load data about the current folder from the project file.
 *
 * @param tsv :: the TSVserialiser object for the project file
 */
void ProjectSerialiser::loadCurrentFolder(const TSVSerialiser &tsv) {
  if (tsv.hasSection("open")) {
    std::string openStr = tsv.sections("open").front();
    int openValue = 0;
    std::stringstream(openStr) >> openValue;
    window->currentFolder()->folderListItem()->setExpanded(openValue);
  }
}

/**
 * Check if the file can we written to
 * @param f :: the file handle
 * @param projectName :: the name of the project
 * @return true if the file handle is writable
 */
bool ProjectSerialiser::canWriteToProject(QFile *fileHandle,
                                          const QString &projectName) {
  // check if we can write
  if (!fileHandle->open(QIODevice::WriteOnly)) {
    QMessageBox::about(window, window->tr("MantidPlot - File save error"),
                       window->tr("The file: <br><b>%1</b> is opened in "
                                  "read-only mode").arg(projectName)); // Mantid
    return false;
  }
  return true;
}

/**
 * Serialise the state of Mantid
 *
 * This will go through all the parts that need to be serialised
 * and create a string for them which will be written to the .mantid file.
 *
 * This will also save workspaces etc. to the project location.
 *
 * @param folder :: the folder to write out
 * @return a string representation of the current project state
 */
QString ProjectSerialiser::serialiseProjectState(Folder *folder) {
  QString text;

  // Save the list of workspaces
  if (window->mantidUI) {
    text += saveWorkspaces();
  }

  // Save the scripting window
  ScriptingWindow *scriptingWindow = window->getScriptWindowHandle();
  if (scriptingWindow) {
    std::string scriptString = scriptingWindow->saveToProject(window);
    text += QString::fromStdString(scriptString);
  }

  text += saveAdditionalWindows();

  // Finally, recursively save folders
  if (folder) {
    text += saveFolderState(folder, true);
  }

  return text;
}

/**
 * Save the folder structure to a Mantid project file.
 *
 * @param app :: the current application window instance
 * @return string represnetation of the folder's data
 */
QString ProjectSerialiser::saveFolderState(Folder *folder,
                                           const bool isTopLevel) {
  QString text;
  bool isCurrentFolder = window->currentFolder() == folder;

  if (!isTopLevel) {
    text += saveFolderHeader(folder, isCurrentFolder);
  }

  text += saveFolderSubWindows(folder);

  if (!isTopLevel) {
    text += saveFolderFooter();
  }

  return text;
}

/**
 * Generate the opening tags and meta information about
 * a folder record for the Mantid project file.
 *
 * @param isCurrentFolder :: whether this folder is the current one.
 * @return string representation of the folder's header data
 */
QString ProjectSerialiser::saveFolderHeader(Folder *folder,
                                            bool isCurrentFolder) {
  QString text;

  // Write the folder opening tag
  text += "<folder>\t" + QString(folder->objectName()) + "\t" +
          folder->birthDate() + "\t" + folder->modificationDate();

  // label it as current if necessary
  if (isCurrentFolder) {
    text += "\tcurrent";
  }

  text += "\n";
  text += "<open>" + QString::number(folder->folderListItem()->isExpanded()) +
          "</open>\n";
  return text;
}

/**
 * Generate the subfolder and subwindow records for the current folder.
 * This method will recursively convert subfolders to their text representation
 *
 * @param app :: the current application window instance
 * @param folder :: the folder to generate the text for.
 * @param windowCount :: count of the number of windows
 * @return string representation of the folder's subfolders
 */
QString ProjectSerialiser::saveFolderSubWindows(Folder *folder) {
  QString text;

  // Write windows
  QList<MdiSubWindow *> windows = folder->windowsList();
  for (auto &w : windows) {
    MantidQt::API::IProjectSerialisable *ips =
        dynamic_cast<MantidQt::API::IProjectSerialisable *>(w);

    if (ips) {
      text += QString::fromUtf8(ips->saveToProject(window).c_str());
    }
  }

  m_windowCount += windows.size();

  // Write subfolders
  QList<Folder *> subfolders = folder->folders();
  foreach (Folder *f, subfolders) { text += saveFolderState(f); }

  // Write log info
  if (!folder->logInfo().isEmpty()) {
    text += "<log>\n" + folder->logInfo() + "</log>\n";
  }

  return text;
}

/**
 * Generate the closing folder data and end tag.
 * @return footer string for this folder
 */
QString ProjectSerialiser::saveFolderFooter() { return "</folder>\n"; }

/** This method saves the currently loaded workspaces in
 * the project.
 *
 * Saves the names of all the workspaces loaded into mantid workspace tree.
 * Creates a string and calls save nexus on each workspace to save the data
 * to a nexus file.
 *
 * @return workspace names formatted as a Mantid project file string
 */
QString ProjectSerialiser::saveWorkspaces() {
  using namespace Mantid::API;
  std::string workingDir = window->workingDir.toStdString();
  QString wsNames;
  wsNames = "<mantidworkspaces>\n";
  wsNames += "WorkspaceNames";

  auto workspaceItems = AnalysisDataService::Instance().getObjectNames();
  for (auto &itemIter : workspaceItems) {
    QString wsName = QString::fromStdString(itemIter);

    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
        wsName.toStdString());
    auto group = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);

    // We don't split up multiperiod workspaces for performance reasons.
    // There's significant optimisations we can perform on load if they're a
    // single file.
    if (ws->id() == "WorkspaceGroup" && group && !group->isMultiperiod()) {
      wsNames += "\t";
      wsNames += wsName;
      std::vector<std::string> secondLevelItems = group->getNames();
      for (size_t j = 0; j < secondLevelItems.size(); j++) {
        wsNames += ",";
        wsNames += QString::fromStdString(secondLevelItems[j]);
        std::string fileName(workingDir + "//" + secondLevelItems[j] + ".nxs");
        window->mantidUI->savedatainNexusFormat(fileName, secondLevelItems[j]);
      }
    } else {
      wsNames += "\t";
      wsNames += wsName;

      std::string fileName(workingDir + "//" + wsName.toStdString() + ".nxs");
      window->mantidUI->savedatainNexusFormat(fileName, wsName.toStdString());
    }
  }
  wsNames += "\n</mantidworkspaces>\n";
  return wsNames;
}

/**
 * Save additional windows that are not MdiSubWindows
 *
 * This includes windows such as the slice viewer, VSI, and the spectrum viewer
 *
 * @return a string representing the sections of the
 */
QString ProjectSerialiser::saveAdditionalWindows() {
  QString output;
  for (auto win : window->getSerialisableWindows()) {
    auto serialisableWindow = dynamic_cast<IProjectSerialisable *>(win);
    if (!serialisableWindow)
      continue;

    auto lines = serialisableWindow->saveToProject(window);
    output += QString::fromStdString(lines);
  }

  return output;
}

/**
 * Check if the project can be backed up.
 *
 * If files cannot be backed up then the user will be queried
 * for permission to skip. If they do not want to skip for any
 * file the function will return false.
 *
 * @param fileHandle :: the file handle
 * @param projectName :: the name of the project
 * @return true if the project can be backed up or the user does not care
 */
bool ProjectSerialiser::canBackupProjectFiles(QFile *fileHandle,
                                              const QString &projectName) {

  if (window->d_backup_files &&
      fileHandle->exists()) { // make byte-copy of current file so that
                              // there's always a copy of the data on
                              // disk
    while (!fileHandle->open(QIODevice::ReadOnly)) {
      if (fileHandle->isOpen())
        fileHandle->close();
      int choice = QMessageBox::warning(
          window, window->tr("MantidPlot - File backup error"), // Mantid
          window->tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If "
                     "you ignore "
                     "this, you run the risk of <b>data loss</b>.")
              .arg(projectName)
              .arg(projectName + "~"),
          QMessageBox::Retry | QMessageBox::Default,
          QMessageBox::Abort | QMessageBox::Escape, QMessageBox::Ignore);
      if (choice == QMessageBox::Abort)
        return false;
      if (choice == QMessageBox::Ignore)
        break;
    }

    if (fileHandle->isOpen()) {
      QFile::copy(projectName, projectName + "~");
      fileHandle->close();
    }
  }

  return true;
}

/**
 * Save the project file to disk
 *
 * @param fileHandle :: the file handle
 * @param projectName :: the name of the project
 * @param text :: the string representation of the current state
 * @param compress :: whether to compress the project
 */
void ProjectSerialiser::saveProjectFile(QFile *fileHandle,
                                        const QString &projectName,
                                        QString &text, bool compress) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // add number of MdiSubWindows saved to file
  text.prepend("<windows>\t" + QString::number(m_windowCount) + "\n");

  // add some header content to the file
  QString lang = QString(window->scriptingEnv()->objectName());
  text.prepend("<scripting-lang>\t" + lang + "\n");

  // construct MantidPlot version number
  QString version;
  version += QString::number(maj_version) + ".";
  version += QString::number(min_version) + ".";
  version += QString::number(patch_version);

  text.prepend("MantidPlot " + version + " project file\n");

  // write out the saved project state
  QTextStream t(fileHandle);
  t.setCodec(QTextCodec::codecForName("UTF-8"));
  t << text;
  fileHandle->close();

  // compress the project if needed
  if (compress) {
    file_compress(projectName.toLatin1().constData(), "w9");
  }

  QApplication::restoreOverrideCursor();
}

/**
 * Open a new script window
 * @param lines :: the string of characters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openScriptWindow(const std::string &lines,
                                         const int fileVersion) {
  window->showScriptWindow();
  auto scriptingWindow = window->getScriptWindowHandle();

  if (!scriptingWindow)
    return;

  scriptingWindow->loadFromProject(lines, window, fileVersion);
}

/**
 * Open a new script window
 * @param files :: list of strings representing file names for python scripts
 */
void ProjectSerialiser::openScriptWindow(const QStringList &files) {
  window->showScriptWindow();
  auto scriptingWindow = window->getScriptWindowHandle();

  if (!scriptingWindow)
    return;

  scriptingWindow->setWindowTitle(
      "MantidPlot: " + window->scriptingEnv()->languageName() + " Window");

  scriptingWindow->loadFromFileList(files);
}

/**
 * Populate Mantid and the ADS with workspaces loaded from the project file
 *
 * @param s :: the string of characters loaded from a Mantid project file
 */
void ProjectSerialiser::populateMantidTreeWidget(const QString &lines) {
  QStringList list = lines.split("\t");
  QStringList::const_iterator line = list.begin();
  for (++line; line != list.end(); ++line) {
    if ((*line)
            .contains(',')) // ...it is a group and more work needs to be done
    {
      // Format of string is "GroupName, Workspace, Workspace, Workspace, ....
      // and so on "
      QStringList groupWorkspaces = (*line).split(',');
      std::string groupName = groupWorkspaces[0].toStdString();
      std::vector<std::string> inputWsVec;
      // Work through workspaces, load into Mantid and then push into
      // vectorgroup (ignore group name, start at 1)
      for (int i = 1; i < groupWorkspaces.size(); i++) {
        std::string wsName = groupWorkspaces[i].toStdString();
        loadWsToMantidTree(wsName);
        inputWsVec.push_back(wsName);
      }

      try {
        bool smallGroup(inputWsVec.size() < 2);
        if (smallGroup) // if the group contains less than two items...
        {
          // ...create a new workspace and then delete it later on (group
          // workspace requires two workspaces in order to run the alg)
          Mantid::API::IAlgorithm_sptr alg =
              Mantid::API::AlgorithmManager::Instance().create(
                  "CreateWorkspace", 1);
          alg->setProperty("OutputWorkspace", "boevsMoreBoevs");
          alg->setProperty<std::vector<double>>("DataX",
                                                std::vector<double>(2, 0.0));
          alg->setProperty<std::vector<double>>("DataY",
                                                std::vector<double>(2, 0.0));
          // execute the algorithm
          alg->execute();
          // name picked because random and won't ever be used.
          inputWsVec.emplace_back("boevsMoreBoevs");
        }

        // Group the workspaces as they were when the project was saved
        std::string algName("GroupWorkspaces");
        Mantid::API::IAlgorithm_sptr groupingAlg =
            Mantid::API::AlgorithmManager::Instance().create(algName, 1);
        groupingAlg->initialize();
        groupingAlg->setProperty("InputWorkspaces", inputWsVec);
        groupingAlg->setPropertyValue("OutputWorkspace", groupName);
        // execute the algorithm
        groupingAlg->execute();

        if (smallGroup) {
          // Delete the temporary workspace used to create a group of 1 or less
          // (currently can't have group of 0)
          Mantid::API::AnalysisDataService::Instance().remove("boevsMoreBoevs");
        }
      }
      // Error catching for algorithms
      catch (std::invalid_argument &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (std::runtime_error &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (std::exception &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      }
    } else // ...not a group so just load the workspace
    {
      loadWsToMantidTree((*line).toStdString());
    }
  }
}

/**
   * Load a workspace into Mantid from a project directory
   *
   * @param wsName :: the name of the workspace to load
   */
void ProjectSerialiser::loadWsToMantidTree(const std::string &wsName) {
  if (wsName.empty()) {
    throw std::runtime_error("Workspace Name not found in project file ");
  }
  std::string fileName(window->workingDir.toStdString() + "/" + wsName);
  fileName.append(".nxs");
  window->mantidUI->loadWSFromFile(wsName, fileName);
}

/**
 * Load additional windows which are not MdiSubWindows
 *
 * This will load other windows in Mantid such as the slice viewer, VSI, and
 * the spectrum viewer
 *
 * @param tsv :: the TSVSerialiser object for the project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::loadAdditionalWindows(const std::string &lines,
                                              const int fileVersion) {
  TSVSerialiser tsv(lines);

  if (tsv.hasSection("SliceViewer")) {
    for (auto &section : tsv.sections("SliceViewer")) {
      auto win = SliceViewer::SliceViewerWindow::loadFromProject(
          section, window, fileVersion);
      window->addSerialisableWindow(dynamic_cast<QObject *>(win));
    }
  }

  if (tsv.hasSection("spectrumviewer")) {
    for (const auto &section : tsv.sections("spectrumviewer")) {
      auto win = SpectrumView::SpectrumView::loadFromProject(section, window,
                                                             fileVersion);
      window->addSerialisableWindow(dynamic_cast<QObject *>(win));
    }
  }

  if (tsv.selectSection("vsi")) {
    std::string vatesLines;
    tsv >> vatesLines;

    auto win = dynamic_cast<VatesViewerInterface *>(
        VatesViewerInterface::loadFromProject(vatesLines, window, fileVersion));
    auto subWindow = setupQMdiSubWindow();
    subWindow->setWidget(win);

    window->connect(window, SIGNAL(shutting_down()), win, SLOT(shutdown()));
    win->connect(win, SIGNAL(requestClose()), subWindow, SLOT(close()));
    win->setParent(subWindow);

    QRect geometry;
    TSVSerialiser tsv2(vatesLines);
    tsv2.selectLine("geometry");
    tsv2 >> geometry;
    subWindow->setGeometry(geometry);
    subWindow->widget()->show();

    window->mantidUI->setVatesSubWindow(subWindow);
    window->addSerialisableWindow(dynamic_cast<QObject *>(win));
  }
}

/**
 * Create a new QMdiSubWindow which will become the parent of the Vates window.
 *
 * @return  a new handle to a QMdiSubWindow instance
 */
QMdiSubWindow *ProjectSerialiser::setupQMdiSubWindow() const {
  auto subWindow = new QMdiSubWindow();

  QIcon icon;
  auto iconName =
      QString::fromUtf8(":/VatesSimpleGuiViewWidgets/icons/pvIcon.png");
  icon.addFile(iconName, QSize(), QIcon::Normal, QIcon::Off);

  subWindow->setAttribute(Qt::WA_DeleteOnClose, false);
  subWindow->setWindowIcon(icon);
  subWindow->setWindowTitle("Vates Simple Interface");
  window->connect(window, SIGNAL(shutting_down()), subWindow, SLOT(close()));
  return subWindow;
}
