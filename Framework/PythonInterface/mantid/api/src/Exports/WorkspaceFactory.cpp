#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/AsType.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;
using namespace Mantid::API;
using namespace Mantid::PythonInterface::Policies;

GET_POINTER_SPECIALIZATION(WorkspaceFactoryImpl)

namespace {
/**
 * Creates from workspace using an existing one as a template.
 * The C++ implementation accepts a boost::shared<const MatrixWorkspace> which
 * we cannot currently handle. This allows a boost::shared<MatrixWorkspace> to
 *be passed
 * in an converted. See MantidPythonInterface/kernel/Policies/RemoveConst for
 *the full
 * explanation of why this is necessary
 *
 *  @param  parent    A shared pointer to the parent workspace
 *  @param  NVectors  (Optional) The number of vectors/histograms/detectors in
 *the workspace
 *  @param  XLength   (Optional) The number of X data points/bin boundaries in
 *each vector (must all be the same)
 *  @param  YLength   (Optional) The number of data/error points in each vector
 *(must all be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 **/
Workspace_sptr createFromParentPtr(WorkspaceFactoryImpl &self,
                                   const MatrixWorkspace_sptr &parent,
                                   size_t NVectors = size_t(-1),
                                   size_t XLength = size_t(-1),
                                   size_t YLength = size_t(-1)) {
  return self.create(parent, NVectors, XLength, YLength);
}

/// Overload generator for create
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
BOOST_PYTHON_FUNCTION_OVERLOADS(createFromParent_Overload, createFromParentPtr,
                                2, 5)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createTable_Overload, createTable, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createPeaks_Overload, createPeaks, 0, 1)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

void export_WorkspaceFactory() {
  const char *createFromParentDoc = "Create a workspace based on the given "
                                    "one. The meta-data, instrument etc are "
                                    "copied from the input"
                                    "If the size parameters are passed then "
                                    "the workspace will be a different size.";

  const char *createFromScratchDoc =
      "Create a clean new worksapce of the given size.";
  typedef MatrixWorkspace_sptr (WorkspaceFactoryImpl::*createFromScratchPtr)(
      const std::string &, const size_t &, const size_t &, const size_t &)
      const;

  class_<WorkspaceFactoryImpl, boost::noncopyable>("WorkspaceFactoryImpl",
                                                   no_init)
      .def("create", &createFromParentPtr,
           createFromParent_Overload(createFromParentDoc,
                                     (arg("self"), arg("parent"),
                                      arg("NVectors") = -1, arg("XLength") = -1,
                                      arg("YLength") = -1)))

      .def("create", (createFromScratchPtr)&WorkspaceFactoryImpl::create,
           createFromScratchDoc, return_value_policy<AsType<Workspace_sptr>>(),
           (arg("self"), arg("className"), arg("NVectors"), arg("XLength"),
            arg("YLength")))

      .def("createTable", &WorkspaceFactoryImpl::createTable,
           createTable_Overload(
               "Creates an empty TableWorkspace",
               (arg("self"), arg("className") = "TableWorkspace"))
               [return_value_policy<AsType<Workspace_sptr>>()])

      .def("createPeaks", &WorkspaceFactoryImpl::createPeaks,
           createPeaks_Overload(
               "Creates an empty PeaksWorkspace",
               (arg("self"), arg("className") = "PeaksWorkspace"))
               [return_value_policy<AsType<Workspace_sptr>>()])

      .def("Instance", &WorkspaceFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns the single instance of this class.")
      .staticmethod("Instance");
}
