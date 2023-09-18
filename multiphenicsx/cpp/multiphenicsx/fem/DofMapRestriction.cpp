// Copyright (C) 2016-2023 by the multiphenicsx authors
//
// This file is part of multiphenicsx.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <dolfinx/fem/DofMap.h>
#include <dolfinx/common/IndexMap.h>
#include <multiphenicsx/fem/DofMapRestriction.h>

using namespace dolfinx;
using dolfinx::fem::DofMap;
using multiphenicsx::fem::DofMapRestriction;

//---------------------------------------------------------------------------
template <typename T>
graph::AdjacencyList<T> all_to_all(MPI_Comm comm,
                                   const graph::AdjacencyList<T>& send_data)
{
  const std::vector<std::int32_t>& send_offsets = send_data.offsets();
  const std::vector<T>& values_in = send_data.array();

  const int comm_size = dolfinx::MPI::size(comm);
  assert(send_data.num_nodes() == comm_size);

  // Data size per destination rank
  std::vector<int> send_size(comm_size);
  std::adjacent_difference(std::next(send_offsets.begin()), send_offsets.end(),
                           send_size.begin());

  // Get received data sizes from each rank
  std::vector<int> recv_size(comm_size);
  MPI_Alltoall(send_size.data(), 1, dolfinx::MPI::mpi_type<int>(), recv_size.data(), 1,
               dolfinx::MPI::mpi_type<int>(), comm);

  // Compute receive offset
  std::vector<std::int32_t> recv_offset(comm_size + 1, 0);
  std::partial_sum(recv_size.begin(), recv_size.end(),
                   std::next(recv_offset.begin()));

  // Send/receive data
  std::vector<T> recv_values(recv_offset.back());
  MPI_Alltoallv(values_in.data(), send_size.data(), send_offsets.data(),
                dolfinx::MPI::mpi_type<T>(), recv_values.data(), recv_size.data(),
                recv_offset.data(), dolfinx::MPI::mpi_type<T>(), comm);

  return graph::AdjacencyList<T>(std::move(recv_values),
                                 std::move(recv_offset));
}
//-----------------------------------------------------------------------------
DofMapRestriction::DofMapRestriction(std::shared_ptr<const DofMap> dofmap,
                                     const std::vector<std::int32_t>& restriction)
    : _dofmap(dofmap)
{
  // Associate each owned and ghost dof that is in the restriction, i.e. a subset of dofs contained by dofmap,
  // to a numbering with respect to the list of active degrees of freedom (restriction)
  _map_owned_dofs(dofmap, restriction);
  _map_ghost_dofs(dofmap, restriction);

  // Compute cell dofs arrays
  _compute_cell_dofs(dofmap);
}
//-----------------------------------------------------------------------------
void DofMapRestriction::_map_owned_dofs(std::shared_ptr<const DofMap> dofmap,
                                        const std::vector<std::int32_t>& restriction)
{
  // Compute local (restricted) indices associated to owned (unrestricted) dofs
  std::int32_t restricted_owned_size = 0;
  const auto unrestricted_owned_size = dofmap->index_map->size_local();
  for (std::size_t d = 0; d < restriction.size(); ++d)
  {
    auto unrestricted_dof = restriction[d];
    if (unrestricted_dof < unrestricted_owned_size)
    {
      _unrestricted_to_restricted[unrestricted_dof] = restricted_owned_size;
      _restricted_to_unrestricted[restricted_owned_size] = unrestricted_dof;
      restricted_owned_size++;
    }
  }

  // Prepare temporary index map, neglecting ghosts.
  MPI_Comm comm = dofmap->index_map->comm();
  std::vector<std::int64_t> empty_ghosts;
  std::vector<int> empty_ranks;
  index_map.reset(new common::IndexMap(comm, restricted_owned_size, empty_ghosts, empty_ranks));
}
//-----------------------------------------------------------------------------
void DofMapRestriction::_map_ghost_dofs(std::shared_ptr<const DofMap> dofmap,
                                        const std::vector<std::int32_t>& restriction)
{
  // Compute local (restricted) indices associated to ghost (unrestricted) dofs
  std::int32_t restricted_ghost_size = 0;
  const auto unrestricted_owned_size = dofmap->index_map->size_local();
  const auto unrestricted_local_range_0 = dofmap->index_map->local_range()[0];
  for (std::size_t d = 0; d < restriction.size(); ++d)
  {
    auto unrestricted_local_dof = restriction[d];
    if (unrestricted_local_dof >= unrestricted_owned_size)
    {
      _unrestricted_to_restricted[unrestricted_local_dof] = index_map->size_local() + restricted_ghost_size;
      _restricted_to_unrestricted[index_map->size_local() + restricted_ghost_size] = unrestricted_local_dof;
      restricted_ghost_size++;
    }
  }

  // Fill in local to global map of ghost dofs
  std::vector<std::int64_t> restricted_global_indices = index_map->global_indices();
  std::vector<std::int64_t> unrestricted_global_indices = dofmap->index_map->global_indices();
  std::vector<int> unrestricted_ghost_owners = dofmap->index_map->owners();
  MPI_Comm comm = dofmap->index_map->comm();
  const std::uint32_t mpi_rank = dolfinx::MPI::rank(comm);
  const std::uint32_t mpi_size = dolfinx::MPI::size(comm);
  std::vector<std::vector<std::int64_t>> send_buffer(mpi_size);
  std::vector<std::int64_t> local_to_global_ghost(restricted_ghost_size);
  std::vector<int> src_ranks_ghost(restricted_ghost_size);

  // In order to fill in the local to global map of ghost *restricted* dofs,
  // we need to proceed as follows:
  // 1. we know the *unrestricted* *global* dof. Find the owner of the *unrestricted* *global* dof.
  //    Then, send this *unrestricted* *global* dof to its owner.
  // 2. on the owning processor, get the *unrestricted* *local* dof. Once this is done,
  //    we can obtain the *restricted* *local* dof thanks to the _unrestricted_to_restricted
  //    map that we store as private attribute, and finally obtain the *restricted* *global*
  //    dof thank to the index map. Then, send this *restricted* *global* dof back to the neigbhoring
  //    processor from which we received it.
  // 3. back on the neigbhoring processor, exploit the _unrestricted_to_restricted map to obatin
  //    the *restricted* *local* dof corresponding to the received *unrestricted* *global* dof,
  //    and store this in the local_to_global_ghost and src_ranks_ghost temporary variables


  // Step 1 - cleanup sending buffer
  for (auto& send_buffer_r: send_buffer)
    send_buffer_r.clear();

  // Step 1 - compute
  for (std::size_t d = 0; d < restriction.size(); ++d)
  {
    auto unrestricted_local_dof = restriction[d];
    if (unrestricted_local_dof >= unrestricted_owned_size)
    {
      const auto unrestricted_global_dof = unrestricted_global_indices[
        unrestricted_local_dof];
      const std::uint32_t index_owner = unrestricted_ghost_owners[
        unrestricted_local_dof - unrestricted_owned_size];
      assert(index_owner != mpi_rank);
      send_buffer[index_owner].push_back(unrestricted_local_dof);
      send_buffer[index_owner].push_back(unrestricted_global_dof);
      send_buffer[index_owner].push_back(mpi_rank);
    }
  }

  // Step 1 - communicate
  const graph::AdjacencyList<std::int64_t> received_buffer_1
      = all_to_all(comm, graph::AdjacencyList<std::int64_t>(send_buffer));

  // Step 2 - cleanup sending buffer
  for (auto& send_buffer_r: send_buffer)
    send_buffer_r.clear();

  // Step 2 - compute
  for (std::uint32_t r = 0; r < mpi_size; ++r)
  {
    auto data_r = received_buffer_1.links(r);
    for (std::uint32_t q = 0; q < data_r.size(); q += 3)
    {
      const auto unrestricted_local_dof_on_sender = data_r[q];
      const auto unrestricted_global_dof = data_r[q + 1];
      const auto sender_rank = data_r[q + 2];

      const auto unrestricted_local_dof = unrestricted_global_dof - unrestricted_local_range_0;
      const auto restricted_local_dof = _unrestricted_to_restricted.at(unrestricted_local_dof);
      send_buffer[sender_rank].push_back(restricted_global_indices[restricted_local_dof]);
      send_buffer[sender_rank].push_back(unrestricted_local_dof_on_sender);
    }
  }

  // Step 2 - communicate
  const graph::AdjacencyList<std::int64_t> received_buffer_2
      = all_to_all(comm, graph::AdjacencyList<std::int64_t>(send_buffer));

  // Step 3 - cleanup sending buffer
  for (auto& send_buffer_r: send_buffer)
    send_buffer_r.clear();

  // Step 3 - compute
  for (std::uint32_t r = 0; r < mpi_size; ++r)
  {
    auto data_r = received_buffer_2.links(r);
    for (std::uint32_t q = 0; q < data_r.size(); q += 2)
    {
      const auto restricted_global_dof = data_r[q];
      const auto unrestricted_local_dof = data_r[q + 1];

      const std::uint32_t index_owner = unrestricted_ghost_owners[
        unrestricted_local_dof - unrestricted_owned_size];
      const auto restricted_local_dof = _unrestricted_to_restricted.at(unrestricted_local_dof);
      const auto restricted_local_dof_ghost = restricted_local_dof - index_map->size_local();
      local_to_global_ghost[restricted_local_dof_ghost] = restricted_global_dof;
      src_ranks_ghost[restricted_local_dof_ghost] = index_owner;
    }
  }

  // Replace temporary index map with a new one, which now includes ghost local_to_global map
  index_map.reset(new common::IndexMap(
    comm, index_map->size_local(), local_to_global_ghost, src_ranks_ghost));
}
//-----------------------------------------------------------------------------
void DofMapRestriction::_compute_cell_dofs(std::shared_ptr<const DofMap> dofmap)
{
  // Fill in cell dofs first into a temporary std::map
  std::map<int, std::vector<std::int32_t>> restricted_cell_dofs;
  std::size_t restricted_cell_dofs_total_size = 0;
  auto unrestricted_cell_dofs = dofmap->map();
  const int num_cells = unrestricted_cell_dofs.extent(0);
  for (int c = 0; c < num_cells; ++c)
  {
    const auto unrestricted_cell_dofs_c = std::experimental::submdspan(
      unrestricted_cell_dofs, c, MDSPAN_IMPL_STANDARD_NAMESPACE::full_extent);
    std::vector<std::int32_t> restricted_cell_dofs_c;
    restricted_cell_dofs_c.reserve(unrestricted_cell_dofs_c.size()); // conservative allocation
    for (std::uint32_t d = 0; d < unrestricted_cell_dofs_c.size(); ++d)
    {
      const auto unrestricted_dof = unrestricted_cell_dofs_c[d];
      if (_unrestricted_to_restricted.count(unrestricted_dof) > 0)
      {
        restricted_cell_dofs_c.push_back(_unrestricted_to_restricted[unrestricted_dof]);
      }
    }
    if (restricted_cell_dofs_c.size() > 0)
    {
      restricted_cell_dofs[c].insert(restricted_cell_dofs[c].end(),
                                     restricted_cell_dofs_c.begin(), restricted_cell_dofs_c.end());
      restricted_cell_dofs_total_size += restricted_cell_dofs_c.size();
    }
  }

  // Flatten std::map into the std::vector dof_array, and store start/end indices associated
  // to each cell in cell_bounds
  _dof_array.reserve(restricted_cell_dofs_total_size);
  _cell_bounds.reserve(num_cells + 1);
  std::size_t current_cell_bound = 0;
  _cell_bounds.push_back(current_cell_bound);
  for (int c = 0; c < num_cells; ++c)
  {
    if (restricted_cell_dofs.count(c) > 0)
    {
      const auto restricted_cell_dofs_c = restricted_cell_dofs.at(c);
      assert(current_cell_bound + restricted_cell_dofs_c.size() <= restricted_cell_dofs_total_size);
      _dof_array.insert(_dof_array.end(), restricted_cell_dofs_c.begin(), restricted_cell_dofs_c.end());
      current_cell_bound += restricted_cell_dofs_c.size();
    }
    _cell_bounds.push_back(current_cell_bound);
  }
}
//-----------------------------------------------------------------------------
