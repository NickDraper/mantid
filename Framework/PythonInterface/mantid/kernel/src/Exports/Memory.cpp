#include "MantidKernel/Memory.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::MemoryStats;
using namespace boost::python;

void export_MemoryStats() {

  class_<MemoryStats>("MemoryStats",
                      init<>(arg("self"), "Construct MemoryStats object."))
      .def("update", &MemoryStats::update, arg("self"))
      .def("totalMem", &MemoryStats::totalMem, arg("self"))
      .def("availMem", &MemoryStats::availMem, arg("self"))
      .def("residentMem", &MemoryStats::residentMem, arg("self"))
      .def("virtualMem", &MemoryStats::virtualMem, arg("self"))
      .def("reservedMem", &MemoryStats::reservedMem, arg("self"))
      .def("getFreeRatio", &MemoryStats::getFreeRatio, arg("self"))
      .def("getCurrentRSS", &MemoryStats::getCurrentRSS, arg("self"))
      .def("getPeakRSS", &MemoryStats::getPeakRSS, arg("self"));
}
