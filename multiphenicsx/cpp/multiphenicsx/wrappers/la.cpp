// Copyright (C) 2016-2021 by the multiphenicsx authors
//
// This file is part of multiphenicsx.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <array>
#include <caster_petsc.h>
#include <memory>
#include <multiphenicsx/la/PETScMatrix.h>
#include <multiphenicsx/la/PETScVector.h>
#include <petsc4py/petsc4py.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

namespace py = pybind11;

namespace
{
  template <class T>
  std::array<T, 2> convert_vector_to_array(const std::vector<T>& input)
  {
    // TODO remove this when pybind11#2123 is fixed.
    assert(input.size() == 2);
    std::array<T, 2> output {{input[0], input[1]}};
    return output;
  }
}

namespace multiphenicsx_wrappers
{

void la(py::module& m)
{
  // utils
  py::enum_<multiphenicsx::la::GhostBlockLayout>(m, "GhostBlockLayout")
      .value("intertwined", multiphenicsx::la::GhostBlockLayout::intertwined)
      .value("trailing", multiphenicsx::la::GhostBlockLayout::trailing);
  m.def("create_petsc_index_sets", &multiphenicsx::la::petsc::create_index_sets,
        py::arg("maps"), py::arg("is_bs"), py::arg("ghosted") = true,
        py::arg("ghost_block_layout") = multiphenicsx::la::GhostBlockLayout::intertwined,
        py::return_value_policy::take_ownership);

  // multiphenicsx::la::MatSubMatrixWrapper
  py::class_<multiphenicsx::la::MatSubMatrixWrapper,
             std::shared_ptr<multiphenicsx::la::MatSubMatrixWrapper>>(m,
                                                                      "MatSubMatrixWrapper")
      .def(py::init(
          [](Mat A,
             std::vector<IS> index_sets_) {
            // Due to pybind11#2123, the argument index_sets is of type
            //   std::vector<IS>
            // rather than
            //   std::array<IS, 2>
            // as in the C++ backend. Convert here the std::vector to a std::array.
            // TODO remove this when pybind11#2123 is fixed.
            auto index_sets = convert_vector_to_array(index_sets_);
            return std::make_unique<multiphenicsx::la::MatSubMatrixWrapper>(A, index_sets);
          }))
      .def(py::init(
          [](Mat A,
             std::vector<IS> unrestricted_index_sets_,
             std::vector<IS> restricted_index_sets_,
             std::array<std::map<std::int32_t, std::int32_t>, 2> unrestricted_to_restricted,
             std::array<int, 2> unrestricted_to_restricted_bs) {
            // Due to pybind11#2123, the arguments {restricted, unrestricted}_index_sets are of type
            //   std::vector<IS>
            // rather than
            //   std::array<IS, 2>
            // as in the C++ backend. Convert here the std::vector to a std::array.
            // TODO remove this when pybind11#2123 is fixed.
            auto unrestricted_index_sets = convert_vector_to_array(unrestricted_index_sets_);
            auto restricted_index_sets = convert_vector_to_array(restricted_index_sets_);
            return std::make_unique<multiphenicsx::la::MatSubMatrixWrapper>(A, unrestricted_index_sets,
                                                                      restricted_index_sets,
                                                                      unrestricted_to_restricted,
                                                                      unrestricted_to_restricted_bs);
          }))
      .def("restore", &multiphenicsx::la::MatSubMatrixWrapper::restore)
      .def("mat", &multiphenicsx::la::MatSubMatrixWrapper::mat);

  // multiphenicsx::la::VecSubVectorReadWrapper
  py::class_<multiphenicsx::la::VecSubVectorReadWrapper,
             std::shared_ptr<multiphenicsx::la::VecSubVectorReadWrapper>>(m,
                                                                          "VecSubVectorReadWrapper")
      .def(py::init<Vec, IS, bool>(),
           py::arg("x"), py::arg("index_set"), py::arg("ghosted") = true)
      .def(py::init<Vec, IS, IS, const std::map<std::int32_t, std::int32_t>&, int, bool>(),
           py::arg("x"), py::arg("unrestricted_index_set"), py::arg("restricted_index_set"),
           py::arg("unrestricted_to_restricted"), py::arg("unrestricted_to_restricted_bs"),
           py::arg("ghosted") = true)
      .def_property_readonly(
          "content",
          [](multiphenicsx::la::VecSubVectorReadWrapper& self) {
            std::vector<PetscScalar>& array = self.mutable_content();
            return py::array(array.size(), array.data(), py::none());
          },
          py::return_value_policy::reference_internal);

  // multiphenicsx::la::VecSubVectorWrapper
  py::class_<multiphenicsx::la::VecSubVectorWrapper, multiphenicsx::la::VecSubVectorReadWrapper,
             std::shared_ptr<multiphenicsx::la::VecSubVectorWrapper>>(m,
                                                                      "VecSubVectorWrapper")
      .def(py::init<Vec, IS, bool>(),
           py::arg("x"), py::arg("index_set"), py::arg("ghosted") = true)
      .def(py::init<Vec, IS, IS, const std::map<std::int32_t, std::int32_t>&, int, bool>(),
           py::arg("x"), py::arg("unrestricted_index_set"), py::arg("restricted_index_set"),
           py::arg("unrestricted_to_restricted"), py::arg("unrestricted_to_restricted_bs"),
           py::arg("ghosted") = true)
      .def("restore", &multiphenicsx::la::VecSubVectorWrapper::restore);
}
} // namespace multiphenics_wrappers
