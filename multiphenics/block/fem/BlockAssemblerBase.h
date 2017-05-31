// Copyright (C) 2016-2017 by the block_ext authors
//
// This file is part of block_ext.
//
// block_ext is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// block_ext is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with block_ext. If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __BLOCK_ASSEMBLER_BASE_H
#define __BLOCK_ASSEMBLER_BASE_H

#include <block/fem/BlockFormBase.h>

namespace dolfin
{

  // Forward declarations
  class GenericTensor;

  /// Provide some common functions used in assembler classes.
  class BlockAssemblerBase
  {
  public:

    /// Constructor
    BlockAssemblerBase();

    /// add_values (bool)
    ///     Default value is false.
    ///     This controls whether values are added to the given
    ///     tensor or if it is zeroed prior to assembly.
    bool add_values;

    /// finalize_tensor (bool)
    ///     Default value is true.
    ///     This controls whether the assembler finalizes the
    ///     given tensor after assembly is completed by calling
    ///     A.apply().
    bool finalize_tensor;

    /// keep_diagonal (bool)
    ///     Default value is false.
    ///     This controls whether the assembler enures that a diagonal
    ///     entry exists in an assembled matrix. It may be removed
    ///     if the matrix is finalised.
    bool keep_diagonal;

    /// Initialize global tensor
    /// @param[out] A (GenericTensor&)
    ///  GenericTensor to assemble into
    /// @param[in] a (Form&)
    ///  Form to assemble from
    void init_global_tensor(GenericTensor& A, const BlockFormBase& a);

  };

}

#endif
