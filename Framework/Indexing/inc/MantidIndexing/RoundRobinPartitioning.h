#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONING_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONING_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/Partitioning.h"

#include <stdexcept>

namespace Mantid {
namespace Indexing {

/** RoundRobinPartitioning : TODO: DESCRIPTION

  @author Simon Heybrock
  @date 2016

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_INDEXING_DLL RoundRobinPartitioning : public Partitioning {
public:
  explicit RoundRobinPartitioning(int numberOfPartitions)
      : Partitioning(numberOfPartitions, PartitionIndex(0),
                     Partitioning::MonitorStrategy::CloneOnEachPartition,
                     std::vector<SpectrumNumber>{}) {}

private:
  PartitionIndex
  doIndexOf(const SpectrumNumber spectrumNumber) const override {
    return PartitionIndex(static_cast<int>(
        static_cast<int32_t>(spectrumNumber) % numberOfNonMonitorPartitions()));
  }
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONING_H_ */
