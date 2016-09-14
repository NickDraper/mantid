#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid {
namespace Indexing {

// TODO: What form of validation should we do? Duplicates? Invalids?
IndexInfo extract(const IndexInfo &source, const std::vector<size_t> &indices) {
  std::vector<specnum_t> specNums;
  std::vector<std::vector<detid_t>> detIDs;
  for (const auto &i : indices) {
    specNums.emplace_back(source.spectrumNumber(i));
    detIDs.emplace_back(source.detectorIDs(i));
  }
  return {std::move(specNums), std::move(detIDs)};
}

IndexInfo extract(const IndexInfo &source, const size_t minIndex, const size_t maxIndex) {
  std::vector<specnum_t> specNums;
  std::vector<std::vector<detid_t>> detIDs;
  for (size_t i = minIndex; i <= maxIndex; ++i) {
    specNums.emplace_back(source.spectrumNumber(i));
    detIDs.emplace_back(source.detectorIDs(i));
  }
  return {std::move(specNums), std::move(detIDs)};
}

} // namespace Indexing
} // namespace Mantid