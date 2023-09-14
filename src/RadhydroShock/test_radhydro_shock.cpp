//==============================================================================
// TwoMomentRad - a radiation transport library for patch-based AMR codes
// Copyright 2020 Benjamin Wibking.
// Released under the MIT license. See LICENSE file included in the GitHub repo.
//==============================================================================
/// \file test_radhydro_shock.cpp
/// \brief Defines a test problem for a radiative shock.
///

#include <cmath>

#include "AMReX_BLassert.H"
#include "AMReX_ParallelDescriptor.H"
#include "RadhydroSimulation.hpp"
#include "fextract.hpp"
#include "hydro_system.hpp"
#include "test_radhydro_shock.hpp"

struct ShockProblem {
}; // dummy type to allow compile-type polymorphism via template specialization

constexpr double a_rad = 1.0e-4;  // equal to P_0 in dimensionless units
constexpr double sigma_a = 1.0e6; // absorption cross section
constexpr double Mach0 = 3.0;

constexpr double c_s0 = 1.0;		 // adiabatic sound speed
constexpr double c = 1732.0508075688772; // std::sqrt(3.0*sigma_a) * c_s0; //
					 // dimensionless speed of light
constexpr double k_B = (c_s0 * c_s0);	 // required to make temperature and sound speed consistent

const double kappa = sigma_a * (c_s0 / c); // opacity [cm^-1]
constexpr double gamma_gas = (5. / 3.);
constexpr double mu = gamma_gas;		       // mean molecular weight (required s.t. c_s0 == 1)
constexpr double c_v = k_B / (mu * (gamma_gas - 1.0)); // specific heat

constexpr double T0 = 1.0;
constexpr double rho0 = 1.0;
constexpr double v0 = (Mach0 * c_s0);

constexpr double T1 = 3.661912665809719;
constexpr double rho1 = 3.0021676971081166;
constexpr double v1 = (Mach0 * c_s0) * (rho0 / rho1);

constexpr double chat = 10.0 * (v0 + c_s0); // reduced speed of light

constexpr double Erad0 = a_rad * (T0 * T0 * T0 * T0);
constexpr double Egas0 = rho0 * c_v * T0;
constexpr double Erad1 = a_rad * (T1 * T1 * T1 * T1);
constexpr double Egas1 = rho1 * c_v * T1;

constexpr double shock_position = 0.0130; // 0.0132; // cm
					  // (shock position drifts to the right
					  // slightly during the simulation, so
// we initialize slightly to the left...)

template <> struct RadSystem_Traits<ShockProblem> {
	static constexpr double c_light = c;
	static constexpr double c_hat = chat;
	static constexpr double radiation_constant = a_rad;
	static constexpr double Erad_floor = 0.;
	static constexpr bool compute_v_over_c_terms = true;
};

template <> struct quokka::EOS_Traits<ShockProblem> {
	static constexpr double mean_molecular_weight = mu;
	static constexpr double boltzmann_constant = k_B;
	static constexpr double gamma = gamma_gas;
};

template <> struct Physics_Traits<ShockProblem> {
	// cell-centred
	static constexpr bool is_hydro_enabled = true;
	static constexpr bool is_chemistry_enabled = false;
	static constexpr int numMassScalars = 0;		     // number of mass scalars
	static constexpr int numPassiveScalars = numMassScalars + 0; // number of passive scalars
	static constexpr bool is_radiation_enabled = true;
	// face-centred
	static constexpr bool is_mhd_enabled = false;
};

template <> AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE auto RadSystem<ShockProblem>::ComputePlanckOpacity(const double rho, const double /*Tgas*/) -> double
{
	return (kappa / rho);
}

template <> AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE auto RadSystem<ShockProblem>::ComputeRosselandOpacity(const double rho, const double /*Tgas*/) -> double
{
	return (kappa / rho);
}

template <> AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE auto RadSystem<ShockProblem>::ComputeEddingtonFactor(double /*f*/) -> double
{
	return (1. / 3.); // Eddington approximation
}

template <>
AMREX_GPU_DEVICE AMREX_FORCE_INLINE void
AMRSimulation<ShockProblem>::setCustomBoundaryConditions(const amrex::IntVect &iv, amrex::Array4<amrex::Real> const &consVar, int /*dcomp*/, int /*numcomp*/,
							 amrex::GeometryData const &geom, const amrex::Real /*time*/, const amrex::BCRec *bcr, int /*bcomp*/,
							 int /*orig_comp*/)
{
	if (!((bcr->lo(0) == amrex::BCType::ext_dir) || (bcr->hi(0) == amrex::BCType::ext_dir))) {
		return;
	}

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

	for (int scalar_index = 0; scalar_index < HydroSystem<ShockProblem>::nscalars_; scalar_index++) {
		consVar(i, j, k, HydroSystem<ShockProblem>::scalar0_index + scalar_index) = 0;
	}

	if (i < lo[0]) {
		const double xmom_L = consVar(lo[0], j, k, RadSystem<ShockProblem>::x1GasMomentum_index);
		const double Egas_L = Egas0 + 0.5 * rho0 * (v0 * v0);
		const double px_L = (xmom_L < (rho0 * v0)) ? xmom_L : (rho0 * v0);

		// x1 left side boundary -- constant
		consVar(i, j, k, RadSystem<ShockProblem>::gasDensity_index) = rho0;
		consVar(i, j, k, RadSystem<ShockProblem>::gasInternalEnergy_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::x1GasMomentum_index) = px_L; // xmom_L;
		consVar(i, j, k, RadSystem<ShockProblem>::x2GasMomentum_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::x3GasMomentum_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::gasEnergy_index) = Egas_L;
		consVar(i, j, k, RadSystem<ShockProblem>::gasInternalEnergy_index) = Egas_L - (px_L * px_L) / (2 * rho0);
		consVar(i, j, k, RadSystem<ShockProblem>::radEnergy_index) = Erad0;
		consVar(i, j, k, RadSystem<ShockProblem>::x1RadFlux_index) = 0;
		consVar(i, j, k, RadSystem<ShockProblem>::x2RadFlux_index) = 0;
		consVar(i, j, k, RadSystem<ShockProblem>::x3RadFlux_index) = 0;
	} else if (i >= hi[0]) {
		const double xmom_R = consVar(hi[0], j, k, RadSystem<ShockProblem>::x1GasMomentum_index);
		const double Egas_R = Egas1 + 0.5 * rho1 * (v1 * v1);
		const double px_R = (xmom_R > (rho1 * v1)) ? xmom_R : (rho1 * v1);

		// x1 right-side boundary -- constant
		consVar(i, j, k, RadSystem<ShockProblem>::gasDensity_index) = rho1;
		consVar(i, j, k, RadSystem<ShockProblem>::gasInternalEnergy_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::x1GasMomentum_index) = px_R; // xmom_R;
		consVar(i, j, k, RadSystem<ShockProblem>::x2GasMomentum_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::x3GasMomentum_index) = 0.;
		consVar(i, j, k, RadSystem<ShockProblem>::gasEnergy_index) = Egas_R;
		consVar(i, j, k, RadSystem<ShockProblem>::gasInternalEnergy_index) = Egas_R - (px_R * px_R) / (2 * rho1);
		consVar(i, j, k, RadSystem<ShockProblem>::radEnergy_index) = Erad1;
		consVar(i, j, k, RadSystem<ShockProblem>::x1RadFlux_index) = 0;
		consVar(i, j, k, RadSystem<ShockProblem>::x2RadFlux_index) = 0;
		consVar(i, j, k, RadSystem<ShockProblem>::x3RadFlux_index) = 0;
	}
}

template <> void RadhydroSimulation<ShockProblem>::setInitialConditionsOnGrid(quokka::grid grid_elem)
{
	// extract variables required from the geom object
	amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx = grid_elem.dx_;
	amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> prob_lo = grid_elem.prob_lo_;
	const amrex::Box &indexRange = grid_elem.indexRange_;
	const amrex::Array4<double> &state_cc = grid_elem.array_;

	// loop over the grid and set the initial condition
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
		amrex::Real const x = prob_lo[0] + (i + amrex::Real(0.5)) * dx[0];

		amrex::Real radEnergy = NAN;
		amrex::Real x1RadFlux = NAN;
		amrex::Real energy = NAN;
		amrex::Real density = NAN;
		amrex::Real x1Momentum = NAN;

		if (x < shock_position) {
			radEnergy = Erad0;
			x1RadFlux = 0.0;
			energy = Egas0 + 0.5 * rho0 * (v0 * v0);
			density = rho0;
			x1Momentum = rho0 * v0;
		} else {
			radEnergy = Erad1;
			x1RadFlux = 0.0;
			energy = Egas1 + 0.5 * rho1 * (v1 * v1);
			density = rho1;
			x1Momentum = rho1 * v1;
		}

		// hydro variables
		state_cc(i, j, k, RadSystem<ShockProblem>::gasDensity_index) = density;
		state_cc(i, j, k, RadSystem<ShockProblem>::x1GasMomentum_index) = x1Momentum;
		state_cc(i, j, k, RadSystem<ShockProblem>::x2GasMomentum_index) = 0;
		state_cc(i, j, k, RadSystem<ShockProblem>::x3GasMomentum_index) = 0;
		state_cc(i, j, k, RadSystem<ShockProblem>::gasEnergy_index) = energy;
		state_cc(i, j, k, RadSystem<ShockProblem>::gasInternalEnergy_index) = energy - (x1Momentum * x1Momentum) / (2 * density);
		// passive scalars
		for (int scalar_index = 0; scalar_index < HydroSystem<ShockProblem>::nscalars_; scalar_index++) {
			state_cc(i, j, k, HydroSystem<ShockProblem>::scalar0_index + scalar_index) = 0;
		}
		// radiation variables
		state_cc(i, j, k, RadSystem<ShockProblem>::radEnergy_index) = radEnergy;
		state_cc(i, j, k, RadSystem<ShockProblem>::x1RadFlux_index) = x1RadFlux;
		state_cc(i, j, k, RadSystem<ShockProblem>::x2RadFlux_index) = 0;
		state_cc(i, j, k, RadSystem<ShockProblem>::x3RadFlux_index) = 0;
	});
}

auto problem_main() -> int
{
	// Problem parameters
	const int max_timesteps = 2e4;
	const double CFL_number = 0.4;
	const double Lx = 0.01578396467532876; // 9.112876254180604 * (c/c_s0) / sigma_a;
	// const int nx = 512;

	// const double initial_dtau = 1.0e-3;		// initial timestep
	// [dimensionless]
	const double max_dtau = 1.0e-3;		// maximum timestep [dimensionless]
	const double max_tau = 3.0 * (Lx / v0); // 3 shock crossing times [dimensionless]

	// const double initial_dt = initial_dtau / c_s0;
	const double max_dt = max_dtau / c_s0;
	const double max_time = max_tau / c_s0;

	constexpr int nvars = RadSystem<ShockProblem>::nvar_;
	amrex::Vector<amrex::BCRec> BCs_cc(nvars);
	for (int n = 0; n < nvars; ++n) {
		BCs_cc[n].setLo(0, amrex::BCType::ext_dir);	    // custom x1
		BCs_cc[n].setHi(0, amrex::BCType::ext_dir);	    // custom x1
		for (int i = 1; i < AMREX_SPACEDIM; ++i) {	    // x2- and x3- directions
			BCs_cc[n].setLo(i, amrex::BCType::int_dir); // periodic
			BCs_cc[n].setHi(i, amrex::BCType::int_dir);
		}
	}

	// Problem initialization
	RadhydroSimulation<ShockProblem> sim(BCs_cc);

	sim.cflNumber_ = CFL_number;
	sim.radiationCflNumber_ = CFL_number;
	sim.maxTimesteps_ = max_timesteps;
	// sim.initDt_ = initial_dt;
	sim.maxDt_ = max_dt;
	sim.stopTime_ = max_time;
	sim.plotfileInterval_ = -1;

	// run
	sim.setInitialConditions();
	sim.evolve();

	// read output variables
	auto [position, values] = fextract(sim.state_new_cc_[0], sim.Geom(0), 0, 0.0);
	int nx = static_cast<int>(position.size());
	int status = 0;

	if (amrex::ParallelDescriptor::IOProcessor()) {
		std::vector<double> xs(nx);
		std::vector<double> Trad(nx);
		std::vector<double> Tgas(nx);
		std::vector<double> Erad(nx);
		std::vector<double> Egas(nx);
		std::vector<double> gasDensity(nx);
		std::vector<double> gasVelocity(nx);

		for (int i = 0; i < nx; ++i) {
			const double x = Lx * ((i + 0.5) / static_cast<double>(nx));
			xs.at(i) = x; // cm

			const double Erad_t = values.at(RadSystem<ShockProblem>::radEnergy_index)[i];
			Erad.at(i) = Erad_t / a_rad;			     // scaled
			Trad.at(i) = std::pow(Erad_t / a_rad, 1. / 4.) / T0; // dimensionless

			const double Etot_t = values.at(RadSystem<ShockProblem>::gasEnergy_index)[i];
			const double rho = values.at(RadSystem<ShockProblem>::gasDensity_index)[i];
			const double x1GasMom = values.at(RadSystem<ShockProblem>::x1GasMomentum_index)[i];
			const double Ekin = (x1GasMom * x1GasMom) / (2.0 * rho);

			const double Egas_t = (Etot_t - Ekin);
			Egas.at(i) = Egas_t;
			Tgas.at(i) = quokka::EOS<ShockProblem>::ComputeTgasFromEint(rho, Egas_t) / T0; // dimensionless

			gasDensity.at(i) = rho;
			gasVelocity.at(i) = x1GasMom / rho;
		}

		// read in exact solution
		std::vector<double> xs_exact;
		std::vector<double> Trad_exact;
		std::vector<double> Tmat_exact;
		std::vector<double> Frad_over_c_exact;

		std::string filename = "../extern/LowrieEdwards/shock.txt";
		std::ifstream fstream(filename, std::ios::in);
		AMREX_ALWAYS_ASSERT(fstream.is_open());
		std::string header;
		std::getline(fstream, header);

		for (std::string line; std::getline(fstream, line);) {
			std::istringstream iss(line);
			std::vector<double> values;

			for (double value = NAN; iss >> value;) {
				values.push_back(value);
			}
			auto x_val = values.at(0);
			auto Tmat_val = values.at(3);
			auto Trad_val = values.at(4);
			auto Frad_over_c_val = values.at(5);

			if ((x_val > 0.0) && (x_val < Lx)) {
				xs_exact.push_back(x_val);
				Tmat_exact.push_back(Tmat_val);
				Trad_exact.push_back(Trad_val);
				Frad_over_c_exact.push_back(Frad_over_c_val);
			}
		}

		// compute error norm
		std::vector<double> Trad_interp(xs_exact.size());
		amrex::Print() << "xs min/max = " << xs[0] << ", " << xs[xs.size() - 1] << std::endl;
		amrex::Print() << "xs_exact min/max = " << xs_exact[0] << ", " << xs_exact[xs_exact.size() - 1] << std::endl;

		interpolate_arrays(xs_exact.data(), Trad_interp.data(), static_cast<int>(xs_exact.size()), xs.data(), Trad.data(), static_cast<int>(xs.size()));

		double err_norm = 0.;
		double sol_norm = 0.;
		for (size_t i = 0; i < xs_exact.size(); ++i) {
			err_norm += std::abs(Trad_interp[i] - Trad_exact[i]);
			sol_norm += std::abs(Trad_exact[i]);
		}

		const double error_tol = 0.01;
		double rel_error = NAN;
		rel_error = err_norm / sol_norm;
		amrex::Print() << "Error norm = " << err_norm << std::endl;
		amrex::Print() << "Solution norm = " << sol_norm << std::endl;
		amrex::Print() << "Relative L1 error norm = " << rel_error << std::endl;

		if ((rel_error > error_tol) || std::isnan(rel_error)) {
			status = 1;
		}

#ifdef HAVE_PYTHON
		// plot results

		// temperature
		std::map<std::string, std::string> Trad_args;
		Trad_args["label"] = "Trad";
		Trad_args["color"] = "black";
		matplotlibcpp::plot(xs, Trad, Trad_args);

		if (fstream.is_open()) {
			std::map<std::string, std::string> Trad_exact_args;
			Trad_exact_args["label"] = "Trad (diffusion ODE)";
			Trad_exact_args["color"] = "black";
			Trad_exact_args["linestyle"] = "dashed";
			matplotlibcpp::plot(xs_exact, Trad_exact, Trad_exact_args);
		}

		std::map<std::string, std::string> Tgas_args;
		Tgas_args["label"] = "Tmat";
		Tgas_args["color"] = "red";
		matplotlibcpp::plot(xs, Tgas, Tgas_args);

		if (fstream.is_open()) {
			std::map<std::string, std::string> Tgas_exact_args;
			Tgas_exact_args["label"] = "Tmat (diffusion ODE)";
			Tgas_exact_args["color"] = "red";
			Tgas_exact_args["linestyle"] = "dashed";
			matplotlibcpp::plot(xs_exact, Tmat_exact, Tgas_exact_args);
		}

		matplotlibcpp::xlabel("length x (dimensionless)");
		matplotlibcpp::ylabel("temperature (dimensionless)");
		matplotlibcpp::legend();
		matplotlibcpp::title(fmt::format("time t = {:.4g}", sim.tNew_[0]));
		matplotlibcpp::save("./radshock_temperature.pdf");

		// gas density
		std::map<std::string, std::string> gasdens_args;
		std::map<std::string, std::string> gasvx_args;
		gasdens_args["label"] = "gas density";
		gasdens_args["color"] = "black";
		gasvx_args["label"] = "gas velocity";
		gasvx_args["color"] = "blue";
		gasvx_args["linestyle"] = "dashed";

		matplotlibcpp::clf();
		matplotlibcpp::plot(xs, gasDensity, gasdens_args);
		matplotlibcpp::plot(xs, gasVelocity, gasvx_args);
		matplotlibcpp::xlabel("length x (dimensionless)");
		matplotlibcpp::ylabel("mass density (dimensionless)");
		matplotlibcpp::legend();
		matplotlibcpp::save("./radshock_gasdensity.pdf");
#endif
	}

	return status;
}
