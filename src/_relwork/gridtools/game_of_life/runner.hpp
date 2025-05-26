#ifndef GRIDTOOLS_GAME_OF_LIFE_RUNNER_HPP
#define GRIDTOOLS_GAME_OF_LIFE_RUNNER_HPP

#ifdef GRIDTOOLS_ENABLED

#include <gridtools/common/defs.hpp>
#include <gridtools/stencil/cartesian.hpp>
#include <gridtools/storage/builder.hpp>
#include <gridtools/storage/sid.hpp>
#ifdef GT_CUDACC  // If compiled with CUDA support
  #include <gridtools/stencil/gpu.hpp>
  #include <gridtools/storage/gpu.hpp>
  using backend_t       = gridtools::stencil::gpu<>;
  using storage_traits_t = gridtools::storage::gpu;
#else             // CPU fallback
  #include <gridtools/stencil/cpu_ifirst.hpp>
  #include <gridtools/storage/cpu_ifirst.hpp>
  using backend_t       = gridtools::stencil::cpu_ifirst<>;
  using storage_traits_t = gridtools::storage::cpu_ifirst;
#endif

namespace gridtools::game_of_life {

using namespace gridtools;
using namespace stencil;
using namespace cartesian;
using namespace expressions;

using axis_t = gridtools::stencil::axis<1>;
using exec_axis_t = gridtools::stencil::axis<1, gridtools::stencil::axis_config::offset<0, 0>>;

// Game of Life functor
struct life_functor {
    // Define accessors: one input (read-only) and one output (read-write)
    // The extent<-1,1,-1,1> indicates this functor will access neighbors 
    // 1 cell away in both i and j directions (8-neighbor stencil):contentReference[oaicite:2]{index=2}.
    using in  = in_accessor<0, extent<-1, 1, -1, 1>>; 
    using out = inout_accessor<1>;
    using param_list = make_param_list<in, out>;

    template <typename Eval>
    GT_FUNCTION static void apply(Eval && eval) {
        int center = eval(in());  // current cell state (0 or 1)
        // Sum all 8 neighbors around (i,j). We explicitly list offsets for Moore neighborhood.
        int neighbor_sum = eval(in(-1,-1)) + eval(in(-1,0)) + eval(in(-1, 1))
                         + eval(in( 0,-1))                  + eval(in( 0, 1))
                         + eval(in( 1,-1)) + eval(in( 1,0)) + eval(in( 1, 1));
        // Apply Game of Life rules:
        if (center == 1) {
            // Alive cell: survives if 2 or 3 neighbors, else dies
            eval(out()) = (neighbor_sum == 2 || neighbor_sum == 3) ? 1 : 0;
        } else {
            // Dead cell: becomes alive if exactly 3 neighbors
            eval(out()) = (neighbor_sum == 3) ? 1 : 0;
        }
    }
};

struct runner {
private:
    using StoragePtr = std::shared_ptr<storage::data_store_t<int, storage_traits_t>>; 
    StoragePtr current_state;  // GridTools storage for current grid state
    StoragePtr next_state;     // GridTools storage for next grid state
    gridtools::grid grid_obj;  // GridTools grid defining the iteration domain
    uint_t Nx, Ny;             // total grid dimensions (including borders)

public:
    void init(int* grid, const cellib::run::run_params& params) {
        Nx = params.x_size;
        Ny = params.y_size;
        int halo = 1;  // one-cell border (dead boundary)
        // Build storages with given dimensions and halo size
        auto builder = storage::builder<storage_traits_t>()
                         .type<int>()
                         .dimensions(Nx, Ny, 1)
                         .halos(halo, halo, 0);  // halo for i,j (needed for neighbor access):contentReference[oaicite:8]{index=8}
        current_state = builder.name("current").initializer(
            [&](int i, int j, int k) { return input_array[j * Nx + i]; }
        ).build();
        next_state = builder.name("next").value(0).build(); 
        // Initialize next_state with 0 (ensures border is zeroed out) 

        // Define iteration domain excluding the halo/border:
        halo_descriptor halo_i(halo, halo, halo, Nx - halo - 1, Nx);
        halo_descriptor halo_j(halo, halo, halo, Ny - halo - 1, Ny);
        grid_obj = make_grid(halo_i, halo_j, /*k_size=*/1);
        // This grid covers indices [1, Nx-2] in i and [1, Ny-2] in j, so the border is excluded:contentReference[oaicite:9]{index=9}.
    
    }

    void run(int steps) {
                for (int s = 0; s < steps; ++s) {
            // Execute one stencil pass (one Game-of-Life generation) on the GPU
            run_single_stage(life_functor(), backend_t(), grid_obj,
                             *current_state,    // accessor 0: input (read-only)
                             *next_state);      // accessor 1: output (written)
            // Swap current and next state for the next iteration (no costly copy)
            std::swap(current_state, next_state);
        }
        // If steps was odd, the final state resides in current_state (after last swap).
        // If steps was even, final state is in current_state as well (since an even number of swaps returns original).
        // So in all cases, current_state now holds the final grid.

        CUCH(cudaDeviceSynchronize());  // Ensure all GPU work is done before accessing data
    }

    std::vector<int> fetch_result() {
        std::vector<int> result(Nx * Ny, 0);  // Prepare output vector with zeros
        
        // Get a host view of the final state and copy it to result
        auto view = current_state->host_view();  // syncs device data to host if needed:contentReference[oaicite:10]{index=10}
        for (uint_t j = 0; j < Ny; ++j) {
            for (uint_t i = 0; i < Nx; ++i) {
                result[j * Nx + i] = view(i, j, 0);
            }
        }

        return result;
    }
};

}

#endif // GRIDTOOLS_ENABLED

#endif // GRIDTOOLS_GAME_OF_LIFE_RUNNER_HPP