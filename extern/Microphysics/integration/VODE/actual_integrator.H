#ifndef actual_integrator_H
#define actual_integrator_H

// Common variables and routines for burners
// that use VODE for their integration.

#include <iomanip>

#include <network.H>
#include <burn_type.H>
#include <eos_type.H>
#include <eos.H>
#include <extern_parameters.H>
#include <vode_type.H>
#include <vode_dvode.H>

using namespace integrator_rp;

template <typename BurnT>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void actual_integrator (BurnT& state, Real dt)
{
    constexpr int int_neqs = integrator_neqs<BurnT>();

    dvode_t<int_neqs> vode_state{};

    // Set the tolerances.

    vode_state.atol_spec = atol_spec; // mass fractions
    vode_state.atol_enuc = atol_enuc; // energy generated

    vode_state.rtol_spec = rtol_spec; // mass fractions
    vode_state.rtol_enuc = rtol_enuc; // energy generated

    // set the Jacobian type
    vode_state.jacobian_type = static_cast<short>(jacobian);

    // Start off by assuming a successful burn.

    state.success = true;

    // Initialize the integration time.

    vode_state.t = 0.0_rt;
    vode_state.tout = dt;

    // Initialize ydot to zero for Strang burn.

    for (int n = 0; n < SVAR; ++n) {
        state.ydot_a[n] = 0;
    }

    // Set the (inverse of the) timestep limiter.

    vode_state.HMXI = 1.0_rt / ode_max_dt;

    // We assume that (rho, T) coming in are valid, do an EOS call
    // to fill the rest of the thermodynamic variables.

    eos(eos_input_rt, state);

    // set the scaling for energy if we integrate it dimensionlessly
    state.e_scale = state.e;

    if (scale_system) {
        // the absolute tol for energy needs to reflect the scaled
        // energy the integrator sees
        vode_state.atol_enuc /= state.e_scale;
    }

    // Fill in the initial integration state.

    burn_to_integrator(state, vode_state);

    // Save the initial composition, temperature, and energy for our later diagnostics.

#ifndef AMREX_USE_GPU
    Real xn_in[NumSpec];
    for (int n = 0; n < NumSpec; ++n) {
        xn_in[n] = state.xn[n];
    }
    Real T_in = state.T;
#endif
    Real e_in = state.e;

    // Call the integration routine.

    int istate = dvode(state, vode_state);
    state.error_code = istate;

    // Copy the integration data back to the burn state.

    integrator_to_burn(vode_state, state);

#ifdef NSE
    // compute the temperature based on the energy release -- we need 
    // this in case we failed in our burn here because we entered NSE

#ifdef AUX_THERMO
    // need to sync the auxiliary data up with the new mass fractions
    set_aux_comp_from_X(state);
#endif

    eos(eos_input_re, state);

#endif

    // Subtract off the initial energy (the application codes expect
    // to get back only the generated energy during the burn).
    // Don't subtract it for primordial chem
    if (subtract_internal_energy) {
        state.e -= e_in;
    }

    // Normalize the final abundances.
    // Don't normalize for primordial chem
    if (!use_number_densities) {
        normalize_abundances_burn(state);
    }

    // Get the number of RHS and Jacobian evaluations.

    state.n_rhs = vode_state.NFE;
    state.n_jac = vode_state.NJE;
    state.n_step = vode_state.NST;

    // VODE does not always fail even though it can lead to unphysical states.
    // Add some checks that indicate a burn fail even if VODE thinks the
    // integration was successful.

    if (istate != IERR_SUCCESS) {
        state.success = false;
    }

    for (int n = 1; n <= NumSpec; ++n) {
        if (vode_state.y(n) < -species_failure_tolerance) {
            state.success = false;
        }

        // Don't enforce the condition below
        // for primordial chem
        if (!use_number_densities) {
            if (vode_state.y(n) > 1.0_rt + species_failure_tolerance) {
                state.success = false;
            }
        }
    }

#ifndef AMREX_USE_GPU
    if (burner_verbose) {
        // Print out some integration statistics, if desired.
        std::cout <<  "integration summary: " << std::endl;
        std::cout <<  "dens: " << state.rho << " temp: " << state.T << std::endl;
        std::cout << " energy released: " << state.e << std::endl;
        std::cout <<  "number of steps taken: " << vode_state.NST << std::endl;
        std::cout <<  "number of f evaluations: " << vode_state.NFE << std::endl;
    }
#endif

    // If we failed, print out the current state of the integration.

    if (!state.success) {
        if (istate != IERR_ENTERED_NSE) {
#ifndef AMREX_USE_GPU
            std::cout << Font::Bold << FGColor::Red << "[ERROR] integration failed in net" << ResetDisplay << std::endl;
            std::cout << "istate = " << istate << std::endl;
            if (istate == 2) {
                std::cout << "  VODE exited successfully, but a check on the data values failed" << std::endl;
            }
            std::cout << "zone = (" << state.i << ", " << state.j << ", " << state.k << ")" << std::endl;
            std::cout << "time = " << vode_state.t << std::endl;
            std::cout << "dt = " << std::setprecision(16) << dt << std::endl;
            std::cout << "temp start = " << std::setprecision(16) << T_in << std::endl;
            std::cout << "xn start = ";
            for (double x : xn_in) {
                std::cout << std::setprecision(16) << x << " ";
            }
            std::cout << std::endl;
            std::cout << "dens current = " << std::setprecision(16) << state.rho << std::endl;
            std::cout << "temp current = " << std::setprecision(16) << state.T << std::endl;
            std::cout << "xn current = ";
            for (double x : state.xn) {
                std::cout << std::setprecision(16) << x << " ";
            }
            std::cout << std::endl;
            std::cout << "energy generated = " << state.e << std::endl;
#endif
        } else {
#ifndef AMREX_USE_GPU
            std::cout << "burn entered NSE during integration (after " << vode_state.NST << " steps), zone = (" << state.i << ", " << state.j << ", " << state.k << ")" << std::endl;
#endif
        }
    }

}

#endif
