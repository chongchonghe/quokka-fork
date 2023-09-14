//==============================================================================
// TwoMomentRad - a radiation transport library for patch-based AMR codes
// Copyright 2020 Benjamin Wibking.
// Released under the MIT license. See LICENSE file included in the GitHub repo.
//==============================================================================
/// \file test_radiation_streaming.cpp
/// \brief Defines a test problem for radiation in the free-streaming regime.
///

#include "test_radiation_streaming.hpp"
#include "AMReX.H"
#include "RadhydroSimulation.hpp"
#include "fextract.hpp"
#include "valarray.hpp"

struct StreamingProblem {
};

constexpr double initial_Erad = 1.0e-5;
constexpr double initial_Egas = 1.0e-5;
constexpr double c = 1.0;	   // speed of light
constexpr double chat = 0.2;	   // reduced speed of light
constexpr double kappa0 = 1.0e-10; // opacity
constexpr double rho = 1.0;

template <> struct quokka::EOS_Traits<StreamingProblem> {
	static constexpr double mean_molecular_weight = 1.0;
	static constexpr double boltzmann_constant = 1.0;
	static constexpr double gamma = 5. / 3.;
};

template <> struct RadSystem_Traits<StreamingProblem> {
	static constexpr double c_light = c;
	static constexpr double c_hat = chat;
	static constexpr double radiation_constant = 1.0;
	static constexpr double Erad_floor = initial_Erad;
	static constexpr bool compute_v_over_c_terms = false;
  static constexpr int nGroups = 2;
  static constexpr int pickGroupToPlot = 1;
  static constexpr std::array<double, nGroups - 1> radEdges {0.1};  // microns, size = nGroups - 1
  // static constexpr int nGroups = 1;
  // static constexpr std::array<double, nGroups - 1> radEdges {};  // microns, size = nGroups - 1
};

template <> struct Physics_Traits<StreamingProblem> {
	// cell-centred
	static constexpr bool is_hydro_enabled = false;
	static constexpr bool is_chemistry_enabled = false;
	static constexpr int numMassScalars = 0;		     // number of mass scalars
	static constexpr int numPassiveScalars = numMassScalars + 0; // number of passive scalars
	static constexpr bool is_radiation_enabled = true;
	// face-centred
	static constexpr bool is_mhd_enabled = false;
  // CCH: a hack to get nvarTotal_cc_radhydro right in physics_info.hpp
  static constexpr int nGroups = RadSystem_Traits<StreamingProblem>::nGroups;
};

template <> AMREX_GPU_HOST_DEVICE void RadSystem<StreamingProblem>::ComputeRadEnergyFractions(quokka::valarray<double, RadSystem_Traits<StreamingProblem>::nGroups> &radEnergyFractions, amrex::Real const temperature)
{
  const double tiny = 1e-10;
  // // fill in one bin
  // fillin(radEnergyFractions, tiny);
  // radEnergyFractions[1] = 1.0 - tiny;
  // evenly distribute across all bins
  fillin(radEnergyFractions, 1.0 / RadSystem_Traits<StreamingProblem>::nGroups);
}

template <> AMREX_GPU_HOST_DEVICE auto RadSystem<StreamingProblem>::ComputePlanckOpacity(const double /*rho*/, const double /*Tgas*/) -> double
{
	return kappa0;
}

template <> AMREX_GPU_HOST_DEVICE auto RadSystem<StreamingProblem>::ComputeOpacityAtBins(const double /*rho*/, const double /*Tgas*/) -> quokka::valarray<double, nGroups_>
{
	quokka::valarray<double, nGroups_> kappaVec;
  for (int g = 0; g < nGroups_; ++g) {
    kappaVec[g] = kappa0;
  }
	return kappaVec;
}

template <> AMREX_GPU_HOST_DEVICE auto RadSystem<StreamingProblem>::ComputeFluxMeanOpacity(const double /*rho*/, const double /*Tgas*/) -> quokka::valarray<double, nGroups_>
{
	quokka::valarray<double, nGroups_> kappaVec;
  for (int g = 0; g < nGroups_; ++g) {
    kappaVec[g] = kappa0;
  }
	return kappaVec;
}

template <> AMREX_GPU_HOST_DEVICE auto RadSystem<StreamingProblem>::ComputeRosselandOpacity(const double /*rho*/, const double /*Tgas*/) -> double
{
	return kappa0;
}

// CCH: compute radiation energy fractions for each bin, given the temperature. Note that nGroups and radEdges are defined at compile time.

template <> void RadhydroSimulation<StreamingProblem>::setInitialConditionsOnGrid(quokka::grid grid_elem)
{
	const amrex::Box &indexRange = grid_elem.indexRange_;
	const amrex::Array4<double> &state_cc = grid_elem.array_;

	const auto Erad0 = initial_Erad;
	const auto Egas0 = initial_Egas;

  // CCH: calculate radEnergyFractions 
	// amrex::GpuArray<double, RadSystem_Traits<StreamingProblem>::nGroups> radEnergyFractions;
	quokka::valarray<double, RadSystem_Traits<StreamingProblem>::nGroups> radEnergyFractions;
  amrex::Real const temperature = 0.0;
	RadSystem<StreamingProblem>::ComputeRadEnergyFractions(radEnergyFractions, temperature);

	// loop over the grid and set the initial condition
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
    // CCH: multigroup radiation
    for (int g = 0; g < RadSystem_Traits<StreamingProblem>::nGroups; ++g) {
      state_cc(i, j, k, RadSystem<StreamingProblem>::radEnergy_index + Physics_NumVars::numRadVars * g) = Erad0 * radEnergyFractions[g];
      state_cc(i, j, k, RadSystem<StreamingProblem>::x1RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
      state_cc(i, j, k, RadSystem<StreamingProblem>::x2RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
      state_cc(i, j, k, RadSystem<StreamingProblem>::x3RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
    }
		state_cc(i, j, k, RadSystem<StreamingProblem>::gasEnergy_index) = Egas0;
		state_cc(i, j, k, RadSystem<StreamingProblem>::gasDensity_index) = rho;
		state_cc(i, j, k, RadSystem<StreamingProblem>::gasInternalEnergy_index) = Egas0;
		state_cc(i, j, k, RadSystem<StreamingProblem>::x1GasMomentum_index) = 0.;
		state_cc(i, j, k, RadSystem<StreamingProblem>::x2GasMomentum_index) = 0.;
		state_cc(i, j, k, RadSystem<StreamingProblem>::x3GasMomentum_index) = 0.;
	});
}

template <>
AMREX_GPU_DEVICE AMREX_FORCE_INLINE void
AMRSimulation<StreamingProblem>::setCustomBoundaryConditions(const amrex::IntVect &iv, amrex::Array4<amrex::Real> const &consVar, int /*dcomp*/,
							     int /*numcomp*/, amrex::GeometryData const &geom, const amrex::Real /*time*/,
							     const amrex::BCRec * /*bcr*/, int /*bcomp*/, int /*orig_comp*/)
{
#if (AMREX_SPACEDIM == 1)
	auto i = iv.toArray()[0];
	int j = 0;
	int k = 0;
#endif
#if (AMREX_SPACEDIM == 2)
	auto [i, j] = iv.toArray();
	int k = 0;
#endif
#if (AMREX_SPACEDIM == 3)
	auto [i, j, k] = iv.toArray();
#endif

	amrex::Box const &box = geom.Domain();
	amrex::GpuArray<int, 3> lo = box.loVect3d();
	amrex::GpuArray<int, 3> hi = box.hiVect3d();

  // CCH: calculate radEnergyFractions 
	// amrex::GpuArray<double, RadSystem_Traits<StreamingProblem>::nGroups> radEnergyFractions;
	quokka::valarray<double, RadSystem_Traits<StreamingProblem>::nGroups> radEnergyFractions;
  const double Erad = 1.0;
  amrex::Real const temperature = 0.0;
  RadSystem<StreamingProblem>::ComputeRadEnergyFractions(radEnergyFractions, temperature);
  // amrex::Real const temperature = quokka::EOS<SuOlsonProblemCgs>::ComputeTgasFromEint(rho, Egas_t);

	if (i < lo[0]) {
		// // streaming inflow boundary
    // const double Erad = 1.0;
		// const double Frad = c * Erad;

		// // x1 left side boundary (Marshak)
		// consVar(i, j, k, RadSystem<StreamingProblem>::radEnergy_index) = Erad;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x1RadFlux_index) = Frad;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x2RadFlux_index) = 0.;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x3RadFlux_index) = 0.;

    // CCH: multigroup radiation
		for (int g = 0; g < RadSystem_Traits<StreamingProblem>::nGroups; ++g) {
      auto const Erad_g = Erad * radEnergyFractions[g];
      consVar(i, j, k, RadSystem<StreamingProblem>::radEnergy_index + Physics_NumVars::numRadVars * g) = Erad_g;
      consVar(i, j, k, RadSystem<StreamingProblem>::x1RadFlux_index + Physics_NumVars::numRadVars * g) = c * Erad_g;
      consVar(i, j, k, RadSystem<StreamingProblem>::x2RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
      consVar(i, j, k, RadSystem<StreamingProblem>::x3RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
    }
	} else if (i >= hi[0]) {
		// // right-side boundary -- constant
		// const double Erad = initial_Erad;
		// consVar(i, j, k, RadSystem<StreamingProblem>::radEnergy_index) = Erad;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x1RadFlux_index) = 0;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x2RadFlux_index) = 0;
		// consVar(i, j, k, RadSystem<StreamingProblem>::x3RadFlux_index) = 0;
    for (int g = 0; g < RadSystem_Traits<StreamingProblem>::nGroups; ++g) {
      auto const Erad_g = initial_Erad * radEnergyFractions[g];
      consVar(i, j, k, RadSystem<StreamingProblem>::radEnergy_index + Physics_NumVars::numRadVars * g) = Erad_g;
      consVar(i, j, k, RadSystem<StreamingProblem>::x1RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
      consVar(i, j, k, RadSystem<StreamingProblem>::x2RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
      consVar(i, j, k, RadSystem<StreamingProblem>::x3RadFlux_index + Physics_NumVars::numRadVars * g) = 0;
    }
	}

	// gas boundary conditions are the same everywhere
	const double Egas = initial_Egas;
	consVar(i, j, k, RadSystem<StreamingProblem>::gasEnergy_index) = Egas;
	consVar(i, j, k, RadSystem<StreamingProblem>::gasDensity_index) = rho;
	consVar(i, j, k, RadSystem<StreamingProblem>::gasInternalEnergy_index) = Egas;
	consVar(i, j, k, RadSystem<StreamingProblem>::x1GasMomentum_index) = 0.;
	consVar(i, j, k, RadSystem<StreamingProblem>::x2GasMomentum_index) = 0.;
	consVar(i, j, k, RadSystem<StreamingProblem>::x3GasMomentum_index) = 0.;
}

auto problem_main() -> int
{
	// Problem parameters
	// const int nx = 1000;
	// const double Lx = 1.0;
	const double CFL_number = 0.8;
	const double dt_max = 1e-2;
	const double tmax = 1.0;
	// const double tmax = 0.03;
	const int max_timesteps = 5000;

	// Boundary conditions
	constexpr int nvars = RadSystem<StreamingProblem>::nvar_;
	amrex::Vector<amrex::BCRec> BCs_cc(nvars);
	for (int n = 0; n < nvars; ++n) {
		BCs_cc[n].setLo(0, amrex::BCType::ext_dir);  // Dirichlet x1
		BCs_cc[n].setHi(0, amrex::BCType::foextrap); // extrapolate x1
		for (int i = 1; i < AMREX_SPACEDIM; ++i) {
			BCs_cc[n].setLo(i, amrex::BCType::int_dir); // periodic
			BCs_cc[n].setHi(i, amrex::BCType::int_dir);
		}
	}

	// Problem initialization
	RadhydroSimulation<StreamingProblem> sim(BCs_cc);

	sim.radiationReconstructionOrder_ = 3; // PPM
	sim.stopTime_ = tmax;
	sim.radiationCflNumber_ = CFL_number;
	sim.maxDt_ = dt_max;
	sim.maxTimesteps_ = max_timesteps;
	sim.plotfileInterval_ = -1;

	// initialize
	sim.setInitialConditions();

	// evolve
	sim.evolve();

	// read output variables
	auto [position, values] = fextract(sim.state_new_cc_[0], sim.Geom(0), 0, 0.0);
	const int nx = static_cast<int>(position.size());

	// compute error norm
	std::vector<double> erad(nx);
	std::vector<double> erad_exact(nx);
	std::vector<double> xs(nx);
	for (int i = 0; i < nx; ++i) {
		amrex::Real const x = position[i];
		xs.at(i) = x;
		erad_exact.at(i) = (x <= chat * tmax) ? 1.0 : 0.0;
		erad.at(i) = values.at(RadSystem<StreamingProblem>::radEnergy_index + Physics_NumVars::numRadVars * RadSystem_Traits<StreamingProblem>::pickGroupToPlot)[i];
	}

	double err_norm = 0.;
	double sol_norm = 0.;
	for (int i = 0; i < nx; ++i) {
		err_norm += std::abs(erad[i] - erad_exact[i]);
		sol_norm += std::abs(erad_exact[i]);
	}

	const double rel_err_norm = err_norm / sol_norm;
	const double rel_err_tol = 0.01;
	int status = 1;
	if (rel_err_norm < rel_err_tol) {
		status = 0;
	}
	amrex::Print() << "Relative L1 norm = " << rel_err_norm << std::endl;

#ifdef HAVE_PYTHON
	// Plot results
	matplotlibcpp::clf();
	matplotlibcpp::ylim(0.0, 1.1);

	std::map<std::string, std::string> erad_args;
	std::map<std::string, std::string> erad_exact_args;
	erad_args["label"] = "numerical solution";
	erad_exact_args["label"] = "exact solution";
	erad_exact_args["linestyle"] = "--";
	matplotlibcpp::plot(xs, erad, erad_args);
	matplotlibcpp::plot(xs, erad_exact, erad_exact_args);

	matplotlibcpp::legend();
	matplotlibcpp::title(fmt::format("t = {:.4f}", sim.tNew_[0]));
	matplotlibcpp::save("./radiation_streaming.pdf");
#endif // HAVE_PYTHON

	// Cleanup and exit
	amrex::Print() << "Finished." << std::endl;
	return status;
}
