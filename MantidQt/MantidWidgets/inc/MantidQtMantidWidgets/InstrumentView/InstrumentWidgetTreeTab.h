#ifndef INSTRUMENTWIDGETTREETAB_H_
#define INSTRUMENTWIDGETTREETAB_H_

#include <MantidQtMantidWidgets/WidgetDllOption.h>
#include "InstrumentWidgetTab.h"

#include <QModelIndex>

namespace MantidQt {
namespace MantidWidgets {
class InstrumentTreeWidget;

/**
        * Implements the instrument tree tab in InstrumentWidget
        */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS InstrumentWidgetTreeTab
    : public InstrumentWidgetTab {
  Q_OBJECT
public:
  explicit InstrumentWidgetTreeTab(InstrumentWidget *instrWidget);
  void initSurface() override;
  /// Load settings for the tree widget tab from a project file
  virtual void loadFromProject(const std::string &lines) override;
  /// Save settings for the tree widget tab to a project file
  virtual std::string saveToProject() const override;
public slots:
  void selectComponentByName(const QString &name);

private:
  void showEvent(QShowEvent *) override;
  /// Widget to display instrument tree
  InstrumentTreeWidget *m_instrumentTree;
};
} // MantidWidgets
} // MantidQt

#endif /*INSTRUMENTWIDGETTREETAB_H_*/
