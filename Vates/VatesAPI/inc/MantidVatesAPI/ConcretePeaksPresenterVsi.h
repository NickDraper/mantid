#ifndef MANTID_VATES_CONCRETE_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_CONCRETE_PEAKS_PRESENTER_VSI_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <vector>

namespace Mantid {
namespace VATES {
class DLLExport ConcretePeaksPresenterVsi : public PeaksPresenterVsi {
public:
  ConcretePeaksPresenterVsi(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                            ViewFrustum_const_sptr frustum,
                            std::string wsFrame);
  ~ConcretePeaksPresenterVsi() override;
  Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const override;
  std::vector<bool> getViewablePeaks() const override;
  void updateViewFrustum(ViewFrustum_const_sptr frustum) override;
  std::string getFrame() const override;
  std::string getPeaksWorkspaceName() const override;
  void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row,
                    Mantid::Kernel::V3D &position, double &radius,
                    Mantid::Kernel::SpecialCoordinateSystem
                        specialCoordinateSystem) const override;
  void sortPeaksWorkspace(const std::string &byColumnName,
                          const bool ascending) override;

private:
  /// Get the max radius.
  double getMaxRadius(Mantid::Geometry::PeakShape_sptr shape) const;
  /// Viewable Peaks
  mutable std::vector<bool> m_viewablePeaks;
  /// The viewable region
  ViewFrustum_const_sptr m_viewableRegion;
  /// The peaks workspace
  Mantid::API::IPeaksWorkspace_sptr m_peaksWorkspace;
  /// The frame
  std::string m_frame;
};
}
}
#endif