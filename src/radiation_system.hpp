#ifndef RADIATION_SYSTEM_HPP_ // NOLINT
#define RADIATION_SYSTEM_HPP_
//==============================================================================
// TwoMomentRad - a radiation transport library for patch-based AMR codes
// Copyright 2020 Benjamin Wibking.
// Released under the MIT license. See LICENSE file included in the GitHub repo.
//==============================================================================
/// \file radiation_system.hpp
/// \brief Defines a class for solving the (1d) radiation moment equations.
///

// c++ headers
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

// library headers
#include "AMReX.H"
#include "AMReX_Array.H"
#include "AMReX_BLassert.H"
#include "AMReX_GpuQualifiers.H"
#include "AMReX_REAL.H"

// internal headers
#include "ArrayView.hpp"
#include "EOS.hpp"
#include "hyperbolic_system.hpp"
#include "simulation.hpp"
#include "valarray.hpp"

// physical constants in CGS units
static constexpr double c_light_cgs_ = 2.99792458e10;	      // cgs
static constexpr double radiation_constant_cgs_ = 7.5646e-15; // cgs
static constexpr double pi = 3.14159265358979323846;

// this struct is specialized by the user application code
//
template <typename problem_t> struct RadSystem_Traits {
	static constexpr double c_light = c_light_cgs_;
	static constexpr double c_hat = c_light_cgs_;
	static constexpr double radiation_constant = radiation_constant_cgs_;
	static constexpr double Erad_floor = 0.;
	static constexpr bool compute_v_over_c_terms = true;
	static constexpr int nGroups = 1;
	static constexpr std::array<double, nGroups - 1> radEdges{}; // microns, size = nGroups - 1
};


/// Class for the radiation moment equations
///
template <typename problem_t> class RadSystem : public HyperbolicSystem<problem_t>
{
      public:
	[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE static auto MC(double a, double b) -> double
	{
		return 0.5 * (sgn(a) + sgn(b)) * std::min(0.5 * std::abs(a + b), std::min(2.0 * std::abs(a), 2.0 * std::abs(b)));
	}

	static constexpr int nmscalars_ = Physics_Traits<problem_t>::numMassScalars;
	static constexpr int numRadVars_ = Physics_NumVars::numRadVars; // CCH: number of radiation variables for each photon group
	static constexpr int nvarHyperbolic_ = Physics_NumVars::numRadVars * RadSystem_Traits<problem_t>::nGroups; // CCH: total number of radiation variables
	static constexpr int nstartHyperbolic_ = Physics_Indices<problem_t>::radFirstIndex;
	static constexpr int nvar_ = nstartHyperbolic_ + nvarHyperbolic_;

	enum gasVarIndex {
		gasDensity_index = Physics_Indices<problem_t>::hydroFirstIndex,
		x1GasMomentum_index,
		x2GasMomentum_index,
		x3GasMomentum_index,
		gasEnergy_index,
		gasInternalEnergy_index,
		scalar0_index
	};

	enum radVarIndex { radEnergy_index = nstartHyperbolic_, x1RadFlux_index, x2RadFlux_index, x3RadFlux_index };

	enum primVarIndex {
		primRadEnergy_index = 0,
		x1ReducedFlux_index,
		x2ReducedFlux_index,
		x3ReducedFlux_index,
	};

	// C++ standard does not allow constexpr to be uninitialized, even in a
	// templated class!
	static constexpr double c_light_ = RadSystem_Traits<problem_t>::c_light;
	static constexpr double c_hat_ = RadSystem_Traits<problem_t>::c_hat;
	static constexpr double radiation_constant_ = RadSystem_Traits<problem_t>::radiation_constant;

	static constexpr double Erad_floor_ = RadSystem_Traits<problem_t>::Erad_floor;
	static constexpr bool compute_v_over_c_terms_ = RadSystem_Traits<problem_t>::compute_v_over_c_terms;

	static constexpr int nGroups_ = RadSystem_Traits<problem_t>::nGroups;
	static constexpr std::array<double, nGroups_> radEdges_ = RadSystem_Traits<problem_t>::radEdges;

	static constexpr double mean_molecular_mass_ = quokka::EOS_Traits<problem_t>::mean_molecular_mass;
	static constexpr double boltzmann_constant_ = quokka::EOS_Traits<problem_t>::boltzmann_constant;
	static constexpr double gamma_ = quokka::EOS_Traits<problem_t>::gamma;

	// static functions

	static void ComputeMaxSignalSpeed(amrex::Array4<const amrex::Real> const &cons, array_t &maxSignal, amrex::Box const &indexRange);
	static void ConservedToPrimitive(amrex::Array4<const amrex::Real> const &cons, array_t &primVar, amrex::Box const &indexRange);

	static void PredictStep(arrayconst_t &consVarOld, array_t &consVarNew, amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxArray,
				amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxDiffusiveArray, double dt_in,
				amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx_in, amrex::Box const &indexRange, int nvars);

	static void AddFluxesRK2(array_t &U_new, arrayconst_t &U0, arrayconst_t &U1, amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxArray,
				 amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxDiffusiveArray, double dt_in,
				 amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx_in, amrex::Box const &indexRange, int nvars);

	template <FluxDir DIR>
	static void ComputeFluxes(array_t &x1Flux_in, array_t &x1FluxDiffusive_in, amrex::Array4<const amrex::Real> const &x1LeftState_in,
				  amrex::Array4<const amrex::Real> const &x1RightState_in, amrex::Box const &indexRange, arrayconst_t &consVar_in,
				  amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx);

	// CCH: compute radiation energy fractions for each bin, given nGroups, radEdges, and temperature
	// static void ComputeRadEnergyFractions(amrex::GpuArray<amrex::Real, nGroups_> &radEnergyFractions, amrex::Real const temperature);
	static void ComputeRadEnergyFractions(quokka::valarray<double, nGroups_> &radEnergyFractions, amrex::Real const temperature);

  // CCH: linear equation solver for matrix with non-zeros at the first row, first column, and diagonal only
  static void SolveLinearEqs(const double a00, const quokka::valarray<double, nGroups_> &a0i, const quokka::valarray<double, nGroups_> &ai0, const quokka::valarray<double, nGroups_> &aii, const double &y0, const quokka::valarray<double, nGroups_> &yi, double &x0, quokka::valarray<double, nGroups_> &xi);

	static void SetRadEnergySource(array_t &radEnergySource, amrex::Box const &indexRange, amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &dx,
				       amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &prob_lo, amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &prob_hi,
				       amrex::Real time);

	static void AddSourceTerms(array_t &consVar, arrayconst_t &radEnergySource, arrayconst_t &advectionFluxes, amrex::Box const &indexRange,
				   amrex::Real dt);
	static void ComputeSourceTermsExplicit(arrayconst_t &consPrev, arrayconst_t &radEnergySource, array_t &src, amrex::Box const &indexRange,
					       amrex::Real dt);

	// Use an additionalr template for ComputeMassScalars as the Array type is not always the same
	template <typename ArrayType>
	AMREX_GPU_DEVICE static auto ComputeMassScalars(ArrayType const &arr, int i, int j, int k) -> amrex::GpuArray<Real, nmscalars_>;

	template <FluxDir DIR>
	static void ReconstructStatesConstant(arrayconst_t &q, array_t &leftState, array_t &rightState, amrex::Box const &indexRange, int nvars);

	template <FluxDir DIR>
	static void ReconstructStatesPLM(arrayconst_t &q, array_t &leftState, array_t &rightState, amrex::Box const &indexRange, int nvars);

	template <FluxDir DIR>
	static void ReconstructStatesPPM(arrayconst_t &q_in, array_t &leftState_in, array_t &rightState_in, amrex::Box const &cellRange,
					 amrex::Box const &interfaceRange, int nvars, int iReadFrom = 0, int iWriteFrom = 0);

  // static auto ComputeOpacityAtBins(const double /*rho*/, const double /*Tgas*/) -> quokka::valarray<double, nGroups_>;
	AMREX_GPU_HOST_DEVICE static auto ComputeEddingtonFactor(double f) -> double;
	AMREX_GPU_HOST_DEVICE static auto ComputePlanckOpacity(double rho, double Tgas) -> double;
	AMREX_GPU_HOST_DEVICE static auto ComputeOpacityAtBins(double rho, double Tgas) -> quokka::valarray<double, nGroups_>;
	AMREX_GPU_HOST_DEVICE static auto ComputeFluxMeanOpacity(double rho, double Tgas) -> quokka::valarray<double, nGroups_>;
	AMREX_GPU_HOST_DEVICE static auto ComputePlanckOpacityTempDerivative(double rho, double Tgas) -> double;
	AMREX_GPU_HOST_DEVICE static auto ComputeOpacityTempDerivativeAtBins(double rho, double Tgas)->quokka::valarray<double, nGroups_>;
	AMREX_GPU_HOST_DEVICE static auto ComputeRosselandOpacity(double rho, double Tgas) -> double;
	AMREX_GPU_HOST_DEVICE static auto ComputeEintFromEgas(double density, double X1GasMom, double X2GasMom, double X3GasMom, double Etot) -> double;
	AMREX_GPU_HOST_DEVICE static auto ComputeEgasFromEint(double density, double X1GasMom, double X2GasMom, double X3GasMom, double Eint) -> double;

  template <FluxDir DIR>
  AMREX_GPU_DEVICE static auto ComputeCellOpticalDepth(const quokka::Array4View<const amrex::Real, DIR> &consVar,
						       amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx, int i, int j, int k) -> double;

  AMREX_GPU_DEVICE static auto isStateValid(std::array<amrex::Real, nvarHyperbolic_> &cons) -> bool;
};

// // CCH: compute radiation energy fractions for each bin, given the temperature. Note that nGroups and radEdges are defined at compile time.
// template <typename problem_t>
// void RadSystem<problem_t>::ComputeRadEnergyFractions(amrex::GpuArray<amrex::Real, nGroups_> &radEnergyFractions, amrex::Real /* temperature */)
// {
// 	const double tiny = 1e-10;
// 	radEnergyFractions[0] = 1.0 - tiny;
// 	for (int i = 1; i < nGroups_; ++i) {
// 		radEnergyFractions[i] = tiny;
// 	}
// }

// CCH: compute radiation energy fractions for each bin, given the temperature. Note that nGroups and radEdges are defined at compile time.
template <typename problem_t>
void RadSystem<problem_t>::ComputeRadEnergyFractions(quokka::valarray<double, nGroups_> &radEnergyFractions, amrex::Real const temperature)
{
  const double tiny = 1e-10;
  // fill in one bin
  fillin(radEnergyFractions, tiny);
  radEnergyFractions[0] = 1.0 - tiny * (nGroups_ - 1);
  // // evenly distribute across all bins
  // fillin(radEnergyFractions, 1.0 / RadSystem_Traits<StreamingProblem>::nGroups);
}

// CCH: linear equation solver for matrix with non-zeros at the first row, first column, and diagonal only
// solve the linear system
//   [a00 a0i] [x0] = [y0]
//   [ai0 aii] [xi]   [yi]
// for x0 and xi, where a0i = (a01, a02, a03, ...); ai0 = (a10, a20, a30, ...); aii = (a11, a22, a33, ...), xi = (x1, x2, x3, ...), yi = (y1, y2, y3, ...)
// and x0 and xi are unknown
template <typename problem_t>
void RadSystem<problem_t>::SolveLinearEqs(const double a00, const quokka::valarray<double, nGroups_> &a0i, const quokka::valarray<double, nGroups_> &ai0, const quokka::valarray<double, nGroups_> &aii, const double &y0, const quokka::valarray<double, nGroups_> &yi, double &x0, quokka::valarray<double, nGroups_> &xi)
{
  auto ratios = a0i / aii;
  x0 = (- sum(ratios * yi) + y0) / (- sum(ratios * ai0) + a00);
  xi = (yi - ai0 * x0) / aii;
}

template <typename problem_t>
void RadSystem<problem_t>::SetRadEnergySource(array_t &radEnergySource, amrex::Box const &indexRange, amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &dx,
					      amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &prob_lo,
					      amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const &prob_hi, amrex::Real time)
{
	// do nothing -- user implemented
}

template <typename problem_t>
void RadSystem<problem_t>::ConservedToPrimitive(amrex::Array4<const amrex::Real> const &cons, array_t &primVar, amrex::Box const &indexRange)
{
	// keep radiation energy density as-is
	// convert (Fx,Fy,Fz) into reduced flux components (fx,fy,fx):
	//   F_x -> F_x / (c*E_r)

	// cell-centered kernel
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
		// CCH: add reduced fluxes for each radiation group
		for (int g = 0; g < nGroups_; ++g) {
			const auto E_r = cons(i, j, k, radEnergy_index + Physics_NumVars::numRadVars * g);
			const auto Fx = cons(i, j, k, x1RadFlux_index + Physics_NumVars::numRadVars * g);
			const auto Fy = cons(i, j, k, x2RadFlux_index + Physics_NumVars::numRadVars * g);
			const auto Fz = cons(i, j, k, x3RadFlux_index + Physics_NumVars::numRadVars * g);

			// check admissibility of states
			AMREX_ASSERT(E_r > 0.0); // NOLINT

			primVar(i, j, k, primRadEnergy_index + Physics_NumVars::numRadVars * g) = E_r;
			primVar(i, j, k, x1ReducedFlux_index + Physics_NumVars::numRadVars * g) = Fx / (c_light_ * E_r);
			primVar(i, j, k, x2ReducedFlux_index + Physics_NumVars::numRadVars * g) = Fy / (c_light_ * E_r);
			primVar(i, j, k, x3ReducedFlux_index + Physics_NumVars::numRadVars * g) = Fz / (c_light_ * E_r);
		}
	});
}

template <typename problem_t>
void RadSystem<problem_t>::ComputeMaxSignalSpeed(amrex::Array4<const amrex::Real> const & /*cons*/, array_t &maxSignal, amrex::Box const &indexRange)
{
	// cell-centered kernel
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
		const double signal_max = c_hat_;
		maxSignal(i, j, k) = signal_max;
	});
}

template <typename problem_t> AMREX_GPU_DEVICE auto RadSystem<problem_t>::isStateValid(std::array<amrex::Real, nvarHyperbolic_> &cons) -> bool
{
	// check if the state variable 'cons' is a valid state
	bool isValid = true;
	for (int g = 0; g < nGroups_; ++g) {
		const auto E_r = cons[radEnergy_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_];
    const auto Fx = cons[x1RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_];
    const auto Fy = cons[x2RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_];
    const auto Fz = cons[x3RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_];

    const auto Fnorm = std::sqrt(Fx * Fx + Fy * Fy + Fz * Fz);
    const auto f = Fnorm / (c_light_ * E_r);

    bool isNonNegative = (E_r > 0.);
    bool isFluxCausal = (f <= 1.);
    isValid = (isValid && isNonNegative && isFluxCausal);
	}
	return isValid;
}

template <typename problem_t>
template <FluxDir DIR>
void RadSystem<problem_t>::ReconstructStatesConstant(arrayconst_t &q_in, array_t &leftState_in, array_t &rightState_in, amrex::Box const &indexRange,
						     const int nvars)
{
	// construct ArrayViews for permuted indices
	quokka::Array4View<amrex::Real const, DIR> q(q_in);
	quokka::Array4View<amrex::Real, DIR> leftState(leftState_in);
	quokka::Array4View<amrex::Real, DIR> rightState(rightState_in);

	// By convention, the interfaces are defined on the left edge of each zone, i.e. xleft_(i)
	// is the "left"-side of the interface at the left edge of zone i, and xright_(i) is the
	// "right"-side of the interface at the *left* edge of zone i. [Indexing note: There are (nx
	// + 1) interfaces for nx zones.]

	amrex::ParallelFor(indexRange, nvars, [=] AMREX_GPU_DEVICE(int i_in, int j_in, int k_in, int n) noexcept {
		// permute array indices according to dir
		auto [i, j, k] = quokka::reorderMultiIndex<DIR>(i_in, j_in, k_in);

		// Use piecewise-constant reconstruction (This converges at first
		// order in spatial resolution.)
		leftState(i, j, k, n) = q(i - 1, j, k, n);
		rightState(i, j, k, n) = q(i, j, k, n);
	});
}

template <typename problem_t>
template <FluxDir DIR>
void RadSystem<problem_t>::ReconstructStatesPLM(arrayconst_t &q_in, array_t &leftState_in, array_t &rightState_in, amrex::Box const &indexRange,
						const int nvars)
{
	// construct ArrayViews for permuted indices
	quokka::Array4View<amrex::Real const, DIR> q(q_in);
	quokka::Array4View<amrex::Real, DIR> leftState(leftState_in);
	quokka::Array4View<amrex::Real, DIR> rightState(rightState_in);

	// Unlike PPM, PLM with the MC limiter is TVD.
	// (There are no spurious oscillations, *except* in the slow-moving shock problem,
	// which can produce unphysical oscillations even when using upwind Godunov fluxes.)
	// However, most tests fail when using PLM reconstruction because
	// the accuracy tolerances are very strict, and the L1 error is significantly
	// worse compared to PPM for a fixed number of mesh elements.

	// By convention, the interfaces are defined on the left edge of each
	// zone, i.e. xleft_(i) is the "left"-side of the interface at
	// the left edge of zone i, and xright_(i) is the "right"-side of the
	// interface at the *left* edge of zone i.

	// Indexing note: There are (nx + 1) interfaces for nx zones.

	amrex::ParallelFor(indexRange, nvars, [=] AMREX_GPU_DEVICE(int i_in, int j_in, int k_in, int n) noexcept {
		// permute array indices according to dir
		auto [i, j, k] = quokka::reorderMultiIndex<DIR>(i_in, j_in, k_in);

		// Use piecewise-linear reconstruction
		// (This converges at second order in spatial resolution.)
		const auto lslope = MC(q(i, j, k, n) - q(i - 1, j, k, n), q(i - 1, j, k, n) - q(i - 2, j, k, n));
		const auto rslope = MC(q(i + 1, j, k, n) - q(i, j, k, n), q(i, j, k, n) - q(i - 1, j, k, n));

		leftState(i, j, k, n) = q(i - 1, j, k, n) + 0.25 * lslope; // NOLINT
		rightState(i, j, k, n) = q(i, j, k, n) - 0.25 * rslope;	   // NOLINT
	});
}

template <typename problem_t>
template <FluxDir DIR>
void RadSystem<problem_t>::ReconstructStatesPPM(arrayconst_t &q_in, array_t &leftState_in, array_t &rightState_in, amrex::Box const &cellRange,
						amrex::Box const & /*interfaceRange*/, const int nvars, const int iReadFrom, const int iWriteFrom)
{
	BL_PROFILE("HyperbolicSystem::ReconstructStatesPPM()");

	// construct ArrayViews for permuted indices
	quokka::Array4View<amrex::Real const, DIR> q(q_in);
	quokka::Array4View<amrex::Real, DIR> leftState(leftState_in);
	quokka::Array4View<amrex::Real, DIR> rightState(rightState_in);

	// By convention, the interfaces are defined on the left edge of each
	// zone, i.e. xleft_(i) is the "left"-side of the interface at the left
	// edge of zone i, and xright_(i) is the "right"-side of the interface
	// at the *left* edge of zone i.

	// Indexing note: There are (nx + 1) interfaces for nx zones.

	amrex::ParallelFor(cellRange, nvars, // cell-centered kernel
			   [=] AMREX_GPU_DEVICE(int i_in, int j_in, int k_in, int n) noexcept {
				   // permute array indices according to dir
				   auto [i, j, k] = quokka::reorderMultiIndex<DIR>(i_in, j_in, k_in);

		// (2.) Constrain interfaces to lie between surrounding cell-averaged
		// values (equivalent to step 2b in Athena++ [ppm_simple.cpp]).
		// [See Eq. B8 of Mignone+ 2005.]

#ifdef MULTIDIM_EXTREMA_CHECK
				   // N.B.: Checking all 27 nearest neighbors is *very* expensive on GPU
				   // (presumably due to lots of cache misses), so it is hard-coded disabled.
				   // Fortunately, almost all problems run stably without enabling this.
				   auto bounds = GetMinmaxSurroundingCell(q_in, i_in, j_in, k_in, n);
#else
			// compute bounds from neighboring cell-averaged values along axis
			const std::pair<double, double> bounds =
				std::minmax({q(i, j, k, iReadFrom+n), q(i - 1, j, k, iReadFrom+n), q(i + 1, j, k, iReadFrom+n)});
#endif

				   // get interfaces
				   // PPM reconstruction following Colella & Woodward (1984), with
				   // some modifications following Mignone (2014), as implemented in
				   // Athena++.

				   // (1.) Estimate the interface a_{i - 1/2}. Equivalent to step 1
				   // in Athena++ [ppm_simple.cpp].

				   // C&W Eq. (1.9) [parabola midpoint for the case of
				   // equally-spaced zones]: a_{j+1/2} = (7/12)(a_j + a_{j+1}) -
				   // (1/12)(a_{j+2} + a_{j-1}). Terms are grouped to preserve exact
				   // symmetry in floating-point arithmetic, following Athena++.
				   const double coef_1 = (7. / 12.);
				   const double coef_2 = (-1. / 12.);
				   const double a_minus = (coef_1 * q(i, j, k, iReadFrom + n) + coef_2 * q(i + 1, j, k, iReadFrom + n)) +
							  (coef_1 * q(i - 1, j, k, iReadFrom + n) + coef_2 * q(i - 2, j, k, iReadFrom + n));
				   const double a_plus = (coef_1 * q(i + 1, j, k, iReadFrom + n) + coef_2 * q(i + 2, j, k, iReadFrom + n)) +
							 (coef_1 * q(i, j, k, iReadFrom + n) + coef_2 * q(i - 1, j, k, iReadFrom + n));

				   // left side of zone i
				   double new_a_minus = clamp(a_minus, bounds.first, bounds.second);

				   // right side of zone i
				   double new_a_plus = clamp(a_plus, bounds.first, bounds.second);

				   // (3.) Monotonicity correction, using Eq. (1.10) in PPM paper. Equivalent
				   // to step 4b in Athena++ [ppm_simple.cpp].

				   const double a = q(i, j, k, iReadFrom + n); // a_i in C&W
				   const double dq_minus = (a - new_a_minus);
				   const double dq_plus = (new_a_plus - a);

				   const double qa = dq_plus * dq_minus; // interface extrema

				   if (qa <= 0.0) { // local extremum

					   // Causes subtle, but very weird, oscillations in the Shu-Osher test
					   // problem. However, it is necessary to get a reasonable solution
					   // for the sawtooth advection problem.
					   const double dq0 = MC(q(i + 1, j, k, iReadFrom + n) - q(i, j, k, iReadFrom + n),
								 q(i, j, k, iReadFrom + n) - q(i - 1, j, k, iReadFrom + n));

					   // use linear reconstruction, following Balsara (2017) [Living Rev
					   // Comput Astrophys (2017) 3:2]
					   new_a_minus = a - 0.5 * dq0;
					   new_a_plus = a + 0.5 * dq0;

					   // original C&W method for this case
					   // new_a_minus = a;
					   // new_a_plus = a;

				   } else { // no local extrema

					   // parabola overshoots near a_plus -> reset a_minus
					   if (std::abs(dq_minus) >= 2.0 * std::abs(dq_plus)) {
						   new_a_minus = a - 2.0 * dq_plus;
					   }

					   // parabola overshoots near a_minus -> reset a_plus
					   if (std::abs(dq_plus) >= 2.0 * std::abs(dq_minus)) {
						   new_a_plus = a + 2.0 * dq_minus;
					   }
				   }

				   rightState(i, j, k, iWriteFrom + n) = new_a_minus;
				   leftState(i + 1, j, k, iWriteFrom + n) = new_a_plus;
			   });
}

template <typename problem_t>
void RadSystem<problem_t>::PredictStep(arrayconst_t &consVarOld, array_t &consVarNew, amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxArray,
				       amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxDiffusiveArray, const double dt_in,
				       amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx_in, amrex::Box const &indexRange, const int /*nvars*/)
{
	// By convention, the fluxes are defined on the left edge of each zone,
	// i.e. flux_(i) is the flux *into* zone i through the interface on the
	// left of zone i, and -1.0*flux(i+1) is the flux *into* zone i through
	// the interface on the right of zone i.

	auto const dt = dt_in;
	const auto dx = dx_in[0];
	const auto x1Flux = fluxArray[0];
	const auto x1FluxDiffusive = fluxDiffusiveArray[0];
#if (AMREX_SPACEDIM >= 2)
	const auto dy = dx_in[1];
	const auto x2Flux = fluxArray[1];
	const auto x2FluxDiffusive = fluxDiffusiveArray[1];
#endif
#if (AMREX_SPACEDIM == 3)
	const auto dz = dx_in[2];
	const auto x3Flux = fluxArray[2];
	const auto x3FluxDiffusive = fluxDiffusiveArray[2];
#endif

	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) noexcept {
		std::array<amrex::Real, nvarHyperbolic_> cons{};

		for (int n = 0; n < nvarHyperbolic_; ++n) {
			cons[n] = consVarOld(i, j, k, nstartHyperbolic_ + n) + (AMREX_D_TERM((dt / dx) * (x1Flux(i, j, k, n) - x1Flux(i + 1, j, k, n)),
											     +(dt / dy) * (x2Flux(i, j, k, n) - x2Flux(i, j + 1, k, n)),
											     +(dt / dz) * (x3Flux(i, j, k, n) - x3Flux(i, j, k + 1, n))));
		}

		if (!isStateValid(cons)) {
			// use diffusive fluxes instead
			for (int n = 0; n < nvarHyperbolic_; ++n) {
				cons[n] = consVarOld(i, j, k, nstartHyperbolic_ + n) +
					  (AMREX_D_TERM((dt / dx) * (x1FluxDiffusive(i, j, k, n) - x1FluxDiffusive(i + 1, j, k, n)),
							+(dt / dy) * (x2FluxDiffusive(i, j, k, n) - x2FluxDiffusive(i, j + 1, k, n)),
							+(dt / dz) * (x3FluxDiffusive(i, j, k, n) - x3FluxDiffusive(i, j, k + 1, n))));

				// x1Flux(i, j, k, n) = x1FluxDiffusive(i, j, k, n);
				// x1Flux(i + 1, j, k, n) = x1FluxDiffusive(i + 1, j, k, n);
				// x1Flux(i, j + 1, k, n) = x1FluxDiffusive(i, j + 1, k, n);
				// x1Flux(i, j, k + 1, n) = x1FluxDiffusive(i, j, k + 1, n);
			}
		}

		for (int n = 0; n < nvarHyperbolic_; ++n) {
			consVarNew(i, j, k, nstartHyperbolic_ + n) = cons[n];
		}
	});
}

template <typename problem_t>
void RadSystem<problem_t>::AddFluxesRK2(array_t &U_new, arrayconst_t &U0, arrayconst_t &U1, amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxArray,
					amrex::GpuArray<arrayconst_t, AMREX_SPACEDIM> fluxDiffusiveArray, const double dt_in,
					amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx_in, amrex::Box const &indexRange, const int /*nvars*/)
{
	// By convention, the fluxes are defined on the left edge of each zone,
	// i.e. flux_(i) is the flux *into* zone i through the interface on the
	// left of zone i, and -1.0*flux(i+1) is the flux *into* zone i through
	// the interface on the right of zone i.

	auto const dt = dt_in;
	const auto dx = dx_in[0];
	const auto x1Flux = fluxArray[0];
	const auto x1FluxDiffusive = fluxDiffusiveArray[0];
#if (AMREX_SPACEDIM >= 2)
	const auto dy = dx_in[1];
	const auto x2Flux = fluxArray[1];
	const auto x2FluxDiffusive = fluxDiffusiveArray[1];
#endif
#if (AMREX_SPACEDIM == 3)
	const auto dz = dx_in[2];
	const auto x3Flux = fluxArray[2];
	const auto x3FluxDiffusive = fluxDiffusiveArray[2];
#endif

	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) noexcept {
		std::array<amrex::Real, nvarHyperbolic_> cons_new{};

		for (int n = 0; n < nvarHyperbolic_; ++n) {
			const double U_0 = U0(i, j, k, nstartHyperbolic_ + n);
			const double U_1 = U1(i, j, k, nstartHyperbolic_ + n);
			const double FxU_1 = (dt / dx) * (x1Flux(i, j, k, n) - x1Flux(i + 1, j, k, n));
#if (AMREX_SPACEDIM >= 2)
			const double FyU_1 = (dt / dy) * (x2Flux(i, j, k, n) - x2Flux(i, j + 1, k, n));
#endif
#if (AMREX_SPACEDIM == 3)
			const double FzU_1 = (dt / dz) * (x3Flux(i, j, k, n) - x3Flux(i, j, k + 1, n));
#endif
			// save results in cons_new
			cons_new[n] = (0.5 * U_0 + 0.5 * U_1) + (AMREX_D_TERM(0.5 * FxU_1, +0.5 * FyU_1, +0.5 * FzU_1));
		}

		if (!isStateValid(cons_new)) {
			// use diffusive fluxes instead
			for (int n = 0; n < nvarHyperbolic_; ++n) {
				const double U_0 = U0(i, j, k, nstartHyperbolic_ + n);
				const double U_1 = U1(i, j, k, nstartHyperbolic_ + n);
				const double FxU_1 = (dt / dx) * (x1FluxDiffusive(i, j, k, n) - x1FluxDiffusive(i + 1, j, k, n));
#if (AMREX_SPACEDIM >= 2)
				const double FyU_1 = (dt / dy) * (x2FluxDiffusive(i, j, k, n) - x2FluxDiffusive(i, j + 1, k, n));
#endif
#if (AMREX_SPACEDIM == 3)
				const double FzU_1 = (dt / dz) * (x3FluxDiffusive(i, j, k, n) - x3FluxDiffusive(i, j, k + 1, n));
#endif
				// save results in cons_new
				cons_new[n] = (0.5 * U_0 + 0.5 * U_1) + (AMREX_D_TERM(0.5 * FxU_1, +0.5 * FyU_1, +0.5 * FzU_1));
			}
		}

		for (int n = 0; n < nvarHyperbolic_; ++n) {
			U_new(i, j, k, nstartHyperbolic_ + n) = cons_new[n];
		}
	});
}

template <typename problem_t> AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeEddingtonFactor(double f_in) -> double
{
	// f is the reduced flux == |F|/cE.
	// compute Levermore (1984) closure [Eq. 25]
	// the is the M1 closure that is derived from Lorentz invariance
	const double f = clamp(f_in, 0., 1.); // restrict f to be within [0, 1]
	const double f_fac = std::sqrt(4.0 - 3.0 * (f * f));
	const double chi = (3.0 + 4.0 * (f * f)) / (5.0 + 2.0 * f_fac);

#if 0
	// compute Minerbo (1978) closure [piecewise approximation]
	// (For unknown reasons, this closure tends to work better
	// than the Levermore/Lorentz closure on the Su & Olson 1997 test.)
	const double chi = (f < 1. / 3.) ? (1. / 3.) : (0.5 - f + 1.5 * f*f);
#endif

	return chi;
}

template <typename problem_t>
template <typename ArrayType>
AMREX_GPU_DEVICE auto RadSystem<problem_t>::ComputeMassScalars(ArrayType const &arr, int i, int j, int k) -> amrex::GpuArray<Real, nmscalars_>
{
	amrex::GpuArray<Real, nmscalars_> massScalars;
	for (int n = 0; n < nmscalars_; ++n) {
		massScalars[n] = arr(i, j, k, scalar0_index + n);
	}
	return massScalars;
}

template <typename problem_t>
template <FluxDir DIR>
AMREX_GPU_DEVICE auto RadSystem<problem_t>::ComputeCellOpticalDepth(const quokka::Array4View<const amrex::Real, DIR> &consVar,
								    amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx, int i, int j, int k) -> double
{
	// compute interface-averaged cell optical depth

	// [By convention, the interfaces are defined on the left edge of each
	// zone, i.e. xleft_(i) is the "left"-side of the interface at
	// the left edge of zone i, and xright_(i) is the "right"-side of the
	// interface at the *left* edge of zone i.]

	// piecewise-constant reconstruction
	const double rho_L = consVar(i - 1, j, k, gasDensity_index);
	const double rho_R = consVar(i, j, k, gasDensity_index);

	const double x1GasMom_L = consVar(i - 1, j, k, x1GasMomentum_index);
	const double x1GasMom_R = consVar(i, j, k, x1GasMomentum_index);

	const double x2GasMom_L = consVar(i - 1, j, k, x2GasMomentum_index);
	const double x2GasMom_R = consVar(i, j, k, x2GasMomentum_index);

	const double x3GasMom_L = consVar(i - 1, j, k, x3GasMomentum_index);
	const double x3GasMom_R = consVar(i, j, k, x3GasMomentum_index);

	const double Egas_L = consVar(i - 1, j, k, gasEnergy_index);
	const double Egas_R = consVar(i, j, k, gasEnergy_index);

	auto massScalars_L = RadSystem<problem_t>::ComputeMassScalars(consVar, i - 1, j, k);
	auto massScalars_R = RadSystem<problem_t>::ComputeMassScalars(consVar, i, j, k);

	double Eint_L = NAN;
	double Eint_R = NAN;
	double Tgas_L = NAN;
	double Tgas_R = NAN;

	if constexpr (gamma_ != 1.0) {
		Eint_L = RadSystem<problem_t>::ComputeEintFromEgas(rho_L, x1GasMom_L, x2GasMom_L, x3GasMom_L, Egas_L);
		Eint_R = RadSystem<problem_t>::ComputeEintFromEgas(rho_R, x1GasMom_R, x2GasMom_R, x3GasMom_R, Egas_R);
		Tgas_L = quokka::EOS<problem_t>::ComputeTgasFromEint(rho_L, Eint_L, massScalars_L);
		Tgas_R = quokka::EOS<problem_t>::ComputeTgasFromEint(rho_R, Eint_R, massScalars_R);
	}

	double dl = NAN;
	if constexpr (DIR == FluxDir::X1) {
		dl = dx[0];
	} else if constexpr (DIR == FluxDir::X2) {
		dl = dx[1];
	} else if constexpr (DIR == FluxDir::X3) {
		dl = dx[2];
	}
	const double tau_L = dl * rho_L * RadSystem<problem_t>::ComputeRosselandOpacity(rho_L, Tgas_L);
	const double tau_R = dl * rho_R * RadSystem<problem_t>::ComputeRosselandOpacity(rho_R, Tgas_R);
	// TODO: add multigroup support for the Opacity

	return (2.0 * tau_L * tau_R) / (tau_L + tau_R); // harmonic mean
							// return 0.5*(tau_L + tau_R); // arithmetic mean
}

template <typename problem_t>
template <FluxDir DIR>
void RadSystem<problem_t>::ComputeFluxes(array_t &x1Flux_in, array_t &x1FluxDiffusive_in, amrex::Array4<const amrex::Real> const &x1LeftState_in,
					 amrex::Array4<const amrex::Real> const &x1RightState_in, amrex::Box const &indexRange, arrayconst_t &consVar_in,
					 amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx)
{
	quokka::Array4View<const amrex::Real, DIR> x1LeftState(x1LeftState_in);
	quokka::Array4View<const amrex::Real, DIR> x1RightState(x1RightState_in);
	quokka::Array4View<amrex::Real, DIR> x1Flux(x1Flux_in);
	quokka::Array4View<amrex::Real, DIR> x1FluxDiffusive(x1FluxDiffusive_in);
	quokka::Array4View<const amrex::Real, DIR> consVar(consVar_in);

	// By convention, the interfaces are defined on the left edge of each
	// zone, i.e. xinterface_(i) is the solution to the Riemann problem at
	// the left edge of zone i.

	// Indexing note: There are (nx + 1) interfaces for nx zones.

	// interface-centered kernel
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i_in, int j_in, int k_in) {
		auto [i, j, k] = quokka::reorderMultiIndex<DIR>(i_in, j_in, k_in);

		// HLL solver following Toro (1998) and Balsara (2017).
		// Radiation eigenvalues from Skinner & Ostriker (2013).

		// gather left- and right- state variables
		// CCH: add reduced fluxes for each radiation group
		for (int g = 0; g < nGroups_; ++g) {
			double erad_L = x1LeftState(i, j, k, primRadEnergy_index + Physics_NumVars::numRadVars * g);
			double erad_R = x1RightState(i, j, k, primRadEnergy_index + Physics_NumVars::numRadVars * g);

			double fx_L = x1LeftState(i, j, k, x1ReducedFlux_index + Physics_NumVars::numRadVars * g);
			double fx_R = x1RightState(i, j, k, x1ReducedFlux_index + Physics_NumVars::numRadVars * g);

			double fy_L = x1LeftState(i, j, k, x2ReducedFlux_index + Physics_NumVars::numRadVars * g);
			double fy_R = x1RightState(i, j, k, x2ReducedFlux_index + Physics_NumVars::numRadVars * g);

			double fz_L = x1LeftState(i, j, k, x3ReducedFlux_index + Physics_NumVars::numRadVars * g);
			double fz_R = x1RightState(i, j, k, x3ReducedFlux_index + Physics_NumVars::numRadVars * g);

			// compute scalar reduced flux f
			double f_L = std::sqrt(fx_L * fx_L + fy_L * fy_L + fz_L * fz_L);
			double f_R = std::sqrt(fx_R * fx_R + fy_R * fy_R + fz_R * fz_R);

			// Compute "un-reduced" Fx, Fy, Fz
			double Fx_L = fx_L * (c_light_ * erad_L);
			double Fx_R = fx_R * (c_light_ * erad_R);

			double Fy_L = fy_L * (c_light_ * erad_L);
			double Fy_R = fy_R * (c_light_ * erad_R);

			double Fz_L = fz_L * (c_light_ * erad_L);
			double Fz_R = fz_R * (c_light_ * erad_R);

			// check that states are physically admissible; if not, use first-order
			// reconstruction
			if (!((erad_L > 0.) && (erad_R > 0.) && (f_L < 1.) && (f_R < 1.))) {
				erad_L = consVar(i - 1, j, k, radEnergy_index + Physics_NumVars::numRadVars * g);
				erad_R = consVar(i, j, k, radEnergy_index + Physics_NumVars::numRadVars * g);

				Fx_L = consVar(i - 1, j, k, x1RadFlux_index + Physics_NumVars::numRadVars * g);
				Fx_R = consVar(i, j, k, x1RadFlux_index + Physics_NumVars::numRadVars * g);

				Fy_L = consVar(i - 1, j, k, x2RadFlux_index + Physics_NumVars::numRadVars * g);
				Fy_R = consVar(i, j, k, x2RadFlux_index + Physics_NumVars::numRadVars * g);

				Fz_L = consVar(i - 1, j, k, x3RadFlux_index + Physics_NumVars::numRadVars * g);
				Fz_R = consVar(i, j, k, x3RadFlux_index + Physics_NumVars::numRadVars * g);

				// compute primitive variables
				fx_L = Fx_L / (c_light_ * erad_L);
				fx_R = Fx_R / (c_light_ * erad_R);

				fy_L = Fy_L / (c_light_ * erad_L);
				fy_R = Fy_R / (c_light_ * erad_R);

				fz_L = Fz_L / (c_light_ * erad_L);
				fz_R = Fz_R / (c_light_ * erad_R);

				f_L = std::sqrt(fx_L * fx_L + fy_L * fy_L + fz_L * fz_L);
				f_R = std::sqrt(fx_R * fx_R + fy_R * fy_R + fz_R * fz_R);
			}

			// check that states are physically admissible
			AMREX_ASSERT(erad_L > 0.0);
			AMREX_ASSERT(erad_R > 0.0);
			// AMREX_ASSERT(f_L < 1.0); // there is sometimes a small (<1%) flux
			// limiting violation when using P1 AMREX_ASSERT(f_R < 1.0);

			std::array<amrex::Real, 3> fvec_L = {fx_L, fy_L, fz_L};
			std::array<amrex::Real, 3> fvec_R = {fx_R, fy_R, fz_R};

			// angle between interface and radiation flux \hat{n}
			// If direction is undefined, just drop direction-dependent
			// terms.
			std::array<amrex::Real, 3> n_L{};
			std::array<amrex::Real, 3> n_R{};

			for (int i = 0; i < 3; ++i) {
				n_L[i] = (f_L > 0.) ? (fvec_L[i] / f_L) : 0.;
				n_R[i] = (f_R > 0.) ? (fvec_R[i] / f_R) : 0.;
			}

			// compute radiation pressure tensors
			const double chi_L = RadSystem<problem_t>::ComputeEddingtonFactor(f_L);
			const double chi_R = RadSystem<problem_t>::ComputeEddingtonFactor(f_R);

			AMREX_ASSERT((chi_L >= 1. / 3.) && (chi_L <= 1.0)); // NOLINT
			AMREX_ASSERT((chi_R >= 1. / 3.) && (chi_R <= 1.0)); // NOLINT

			// diagonal term of Eddington tensor
			const double Tdiag_L = (1.0 - chi_L) / 2.0;
			const double Tdiag_R = (1.0 - chi_R) / 2.0;

			// anisotropic term of Eddington tensor (in the direction of the
			// rad. flux)
			const double Tf_L = (3.0 * chi_L - 1.0) / 2.0;
			const double Tf_R = (3.0 * chi_R - 1.0) / 2.0;

			// assemble Eddington tensor
			double T_L[3][3];
			double T_R[3][3];
			double P_L[3][3];
			double P_R[3][3];

			for (int i = 0; i < 3; ++i) {
				for (int j = 0; j < 3; ++j) {
					const double delta_ij = (i == j) ? 1 : 0;
					T_L[i][j] = Tdiag_L * delta_ij + Tf_L * (n_L[i] * n_L[j]);
					T_R[i][j] = Tdiag_R * delta_ij + Tf_R * (n_R[i] * n_R[j]);
					// compute the elements of the total radiation pressure
					// tensor
					P_L[i][j] = T_L[i][j] * erad_L;
					P_R[i][j] = T_R[i][j] * erad_R;
				}
			}

			// frozen Eddington tensor approximation, following Balsara
			// (1999) [JQSRT Vol. 61, No. 5, pp. 617–627, 1999], Eq. 46.
			double Tnormal_L = NAN;
			double Tnormal_R = NAN;
			if constexpr (DIR == FluxDir::X1) {
				Tnormal_L = T_L[0][0];
				Tnormal_R = T_R[0][0];
			} else if constexpr (DIR == FluxDir::X2) {
				Tnormal_L = T_L[1][1];
				Tnormal_R = T_R[1][1];
			} else if constexpr (DIR == FluxDir::X3) {
				Tnormal_L = T_L[2][2];
				Tnormal_R = T_R[2][2];
			}

			// compute fluxes F_L, F_R
			// P_nx, P_ny, P_nz indicate components where 'n' is the direction of the
			// face normal F_n is the radiation flux component in the direction of the
			// face normal
			double Fn_L = NAN;
			double Fn_R = NAN;
			double Pnx_L = NAN;
			double Pnx_R = NAN;
			double Pny_L = NAN;
			double Pny_R = NAN;
			double Pnz_L = NAN;
			double Pnz_R = NAN;

			if constexpr (DIR == FluxDir::X1) {
				Fn_L = Fx_L;
				Fn_R = Fx_R;

				Pnx_L = P_L[0][0];
				Pny_L = P_L[0][1];
				Pnz_L = P_L[0][2];

				Pnx_R = P_R[0][0];
				Pny_R = P_R[0][1];
				Pnz_R = P_R[0][2];
			} else if constexpr (DIR == FluxDir::X2) {
				Fn_L = Fy_L;
				Fn_R = Fy_R;

				Pnx_L = P_L[1][0];
				Pny_L = P_L[1][1];
				Pnz_L = P_L[1][2];

				Pnx_R = P_R[1][0];
				Pny_R = P_R[1][1];
				Pnz_R = P_R[1][2];
			} else if constexpr (DIR == FluxDir::X3) {
				Fn_L = Fz_L;
				Fn_R = Fz_R;

				Pnx_L = P_L[2][0];
				Pny_L = P_L[2][1];
				Pnz_L = P_L[2][2];

				Pnx_R = P_R[2][0];
				Pny_R = P_R[2][1];
				Pnz_R = P_R[2][2];
			}

			AMREX_ASSERT(Fn_L != NAN);
			AMREX_ASSERT(Fn_R != NAN);
			AMREX_ASSERT(Pnx_L != NAN);
			AMREX_ASSERT(Pnx_R != NAN);
			AMREX_ASSERT(Pny_L != NAN);
			AMREX_ASSERT(Pny_R != NAN);
			AMREX_ASSERT(Pnz_L != NAN);
			AMREX_ASSERT(Pnz_R != NAN);

			// CCH: changed nvarHyperbolic_ to numRadVars_ (= 4)
			const quokka::valarray<double, numRadVars_> F_L = {(c_hat_ / c_light_) * Fn_L, (c_hat_ * c_light_) * Pnx_L, (c_hat_ * c_light_) * Pny_L,
									   (c_hat_ * c_light_) * Pnz_L};

			const quokka::valarray<double, numRadVars_> F_R = {(c_hat_ / c_light_) * Fn_R, (c_hat_ * c_light_) * Pnx_R, (c_hat_ * c_light_) * Pny_R,
									   (c_hat_ * c_light_) * Pnz_R};

			const quokka::valarray<double, numRadVars_> U_L = {erad_L, Fx_L, Fy_L, Fz_L};
			const quokka::valarray<double, numRadVars_> U_R = {erad_R, Fx_R, Fy_R, Fz_R};

			// asymptotic-preserving flux correction
			// [Similar to Skinner et al. (2019), but tau^-2 instead of tau^-1, which
			// does not appear to be asymptotic-preserving with PLM+SDC2.]
			const double tau_cell = ComputeCellOpticalDepth<DIR>(consVar, dx, i, j, k);

			// ensures that signal speed -> c \sqrt{f_xx} / tau_cell in the diffusion
			// limit [see Appendix of Jiang et al. ApJ 767:148 (2013)]
			// const double S_corr = std::sqrt(1.0 - std::exp(-tau_cell * tau_cell)) /
			//		      tau_cell; // Jiang et al. (2013)
			const double S_corr = std::min(1.0, 1.0 / tau_cell); // Skinner et al.

			// adjust the wavespeeds
			// (this factor cancels out except for the last term in the HLL flux)
			// const quokka::valarray<double, nvarHyperbolic_> epsilon = {
			//    S_corr, 1.0, 1.0, 1.0}; // Skinner et al. (2019)
			// const quokka::valarray<double, nvarHyperbolic_> epsilon = {S_corr,
			// S_corr,
			//    S_corr, S_corr}; // Jiang et al. (2013)
			const quokka::valarray<double, numRadVars_> epsilon = {S_corr * S_corr, S_corr, S_corr, S_corr}; // this code

			// compute the norm of the wavespeed vector
			const double S_L = std::min(-0.1 * c_hat_, -c_hat_ * std::sqrt(Tnormal_L));
			const double S_R = std::max(0.1 * c_hat_, c_hat_ * std::sqrt(Tnormal_R));

			AMREX_ASSERT(std::abs(S_L) <= c_hat_); // NOLINT
			AMREX_ASSERT(std::abs(S_R) <= c_hat_); // NOLINT

			// in the frozen Eddington tensor approximation, we are always
			// in the star region, so F = F_star
			const quokka::valarray<double, numRadVars_> F =
			    (S_R / (S_R - S_L)) * F_L - (S_L / (S_R - S_L)) * F_R + epsilon * (S_R * S_L / (S_R - S_L)) * (U_R - U_L);

			// check states are valid
			AMREX_ASSERT(!std::isnan(F[0])); // NOLINT
			AMREX_ASSERT(!std::isnan(F[1])); // NOLINT
			AMREX_ASSERT(!std::isnan(F[2])); // NOLINT
			AMREX_ASSERT(!std::isnan(F[3])); // NOLINT

			x1Flux(i, j, k, radEnergy_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = F[0];
			x1Flux(i, j, k, x1RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = F[1];
			x1Flux(i, j, k, x2RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = F[2];
			x1Flux(i, j, k, x3RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = F[3];

			const quokka::valarray<double, numRadVars_> diffusiveF =
			    (S_R / (S_R - S_L)) * F_L - (S_L / (S_R - S_L)) * F_R + (S_R * S_L / (S_R - S_L)) * (U_R - U_L);

			x1FluxDiffusive(i, j, k, radEnergy_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = diffusiveF[0];
			x1FluxDiffusive(i, j, k, x1RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = diffusiveF[1];
			x1FluxDiffusive(i, j, k, x2RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = diffusiveF[2];
			x1FluxDiffusive(i, j, k, x3RadFlux_index + Physics_NumVars::numRadVars * g - nstartHyperbolic_) = diffusiveF[3];
		} // end loop over radiation groups
	});
}

template <typename problem_t> AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputePlanckOpacity(const double /*rho*/, const double /*Tgas*/) -> double
{
	return NAN;
}

template <typename problem_t> AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeOpacityAtBins(const double /*rho*/, const double /*Tgas*/) -> quokka::valarray<double, nGroups_>
{
	quokka::valarray<double, nGroups_> kappaVec{};
	// fillin(kappaVec, NAN);
	return kappaVec;
}

template <typename problem_t> AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeFluxMeanOpacity(const double /*rho*/, const double /*Tgas*/) -> quokka::valarray<double, nGroups_>
{
	quokka::valarray<double, nGroups_> kappaVec{};
	// fillin(kappaVec, NAN);
	return kappaVec;
}

template <typename problem_t>
AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputePlanckOpacityTempDerivative(const double /*rho*/, const double /*Tgas*/) -> double
{
	return 0.0;
}

template <typename problem_t>
AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeOpacityTempDerivativeAtBins(const double rho, const double Tgas) -> quokka::valarray<double, nGroups_>
{
  quokka::valarray<double, nGroups_> kappa{};
	fillin(kappa, 0.0);
	return kappa;
}

template <typename problem_t> AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeRosselandOpacity(const double /*rho*/, const double /*Tgas*/) -> double
{
	return NAN;
}

template <typename problem_t>
AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeEintFromEgas(const double density, const double X1GasMom, const double X2GasMom, const double X3GasMom,
								     const double Etot) -> double
{
	const double p_sq = X1GasMom * X1GasMom + X2GasMom * X2GasMom + X3GasMom * X3GasMom;
	const double Ekin = p_sq / (2.0 * density);
	const double Eint = Etot - Ekin;
	AMREX_ASSERT_WITH_MESSAGE(Eint > 0., "Gas internal energy is not positive!");
	return Eint;
}

template <typename problem_t>
AMREX_GPU_HOST_DEVICE auto RadSystem<problem_t>::ComputeEgasFromEint(const double density, const double X1GasMom, const double X2GasMom, const double X3GasMom,
								     const double Eint) -> double
{
	const double p_sq = X1GasMom * X1GasMom + X2GasMom * X2GasMom + X3GasMom * X3GasMom;
	const double Ekin = p_sq / (2.0 * density);
	const double Etot = Eint + Ekin;
	return Etot;
}

template <typename problem_t>
void RadSystem<problem_t>::AddSourceTerms(array_t &consVar, arrayconst_t &radEnergySource, arrayconst_t &advectionFluxes, amrex::Box const &indexRange,
					  amrex::Real dt)
{
	arrayconst_t &consPrev = consVar; // make read-only
	array_t &consNew = consVar;

	// CCH
	const auto numRadVars_ = Physics_NumVars::numRadVars;

	// Add source terms

	// 1. Compute gas energy and radiation energy update following Howell &
	// Greenough [Journal of Computational Physics 184 (2003) 53–78].

	// cell-centered kernel
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
		const double c = c_light_;
		const double chat = c_hat_;

		// load fluid properties
		const double rho = consPrev(i, j, k, gasDensity_index);
		const double x1GasMom0 = consPrev(i, j, k, x1GasMomentum_index);
		const double x2GasMom0 = consPrev(i, j, k, x2GasMomentum_index);
		const double x3GasMom0 = consPrev(i, j, k, x3GasMomentum_index);
		const double Egastot0 = consPrev(i, j, k, gasEnergy_index);
		auto massScalars = RadSystem<problem_t>::ComputeMassScalars(consPrev, i, j, k);





		// ------  CCH: Multigroup ---------
		// load radiation energy
		quokka::valarray<double, nGroups_> Erad0Vec; // TODO: make this const
		for (int g = 0; g < nGroups_; ++g) {
			Erad0Vec[g] = consPrev(i, j, k, radEnergy_index + numRadVars_ * g);
		}
		AMREX_ASSERT(min(Erad0Vec) >= 0.0);
		const double Erad0 = sum(Erad0Vec);

		// load radiation energy source term
		// plus advection source term (for well-balanced/SDC integrators)
		quokka::valarray<double, nGroups_> Src;
		for (int g = 0; g < nGroups_; ++g) {
			Src[g] = dt * (chat * radEnergySource(i, j, k, g) + advectionFluxes(i, j, k, g));
		}

		double Egas0 = NAN;
		double Ekin0 = NAN;
		double Egas_guess = NAN;
		double T_gas = NAN;
		double Rtot;
		double dFG_dEgas = NAN;
		double deltaEgas = NAN;
		double dRtot_dEgas;
		quokka::valarray<double, nGroups_> EradVec_guess;
		quokka::valarray<double, nGroups_> kappaVec;
		quokka::valarray<double, nGroups_> Rvec;
		quokka::valarray<double, nGroups_> dRtot_dErad;
		quokka::valarray<double, nGroups_> dRvec_dEgas;
		quokka::valarray<double, nGroups_> dFG_dErad;
		quokka::valarray<double, nGroups_> dFR_dEgas;
		quokka::valarray<double, nGroups_> dFR_i_dErad_i;
		quokka::valarray<double, nGroups_> deltaErad;
    for (int g = 0; g < nGroups_; ++g) {
			EradVec_guess[g] = NAN;
			deltaErad[g] = NAN;
    }

		if constexpr (gamma_ != 1.0) {
			Egas0 = ComputeEintFromEgas(rho, x1GasMom0, x2GasMom0, x3GasMom0, Egastot0);
			Ekin0 = Egastot0 - Egas0;

			AMREX_ASSERT(min(Src) >= 0.0);
			AMREX_ASSERT(Egas0 > 0.0);

			const double Etot0 = Egas0 + (c / chat) * (Erad0 + sum(Src));

			// BEGIN NEWTON-RAPHSON LOOP
			Egas_guess = Egas0;
			EradVec_guess = Erad0Vec;
			double kappa, fourPiB;

			const double a_rad = radiation_constant_;
			double F_G = NAN;
			quokka::valarray<double, nGroups_> F_R;
      // fillin(F_R, NAN);
      for (int g = 0; g < nGroups_; ++g) {
        F_R[g] = NAN;
      }

			const double resid_tol = 1.0e-10; // 1.0e-15;
			const int maxIter = 400;
      int n;

			for (n = 0; n < maxIter; ++n) {
				// compute material temperature
				T_gas = quokka::EOS<problem_t>::ComputeTgasFromEint(rho, Egas_guess, massScalars);
        // TOOD: problem, Egas_guess could be negative
				AMREX_ASSERT(T_gas >= 0.);

				// compute opacity, emissivity
				kappa = RadSystem<problem_t>::ComputePlanckOpacity(rho, T_gas);
				AMREX_ASSERT(kappa >= 0.);
				fourPiB = chat * a_rad * std::pow(T_gas, 4);
				auto B = fourPiB / (4.0 * pi);
				kappaVec = RadSystem<problem_t>::ComputeOpacityAtBins(rho, T_gas);

				// compute derivatives w/r/t T_gas
				const double dB_dTgas = (4.0 * B) / T_gas;
				const double dkappa_dTgas = RadSystem<problem_t>::ComputePlanckOpacityTempDerivative(rho, T_gas);

				// compute residuals
				quokka::valarray<double, nGroups_> radEnergyFractions;
        RadSystem<problem_t>::ComputeRadEnergyFractions(radEnergyFractions, T_gas);
        AMREX_ASSERT(!isnan(radEnergyFractions));
				Rvec = dt * rho * (kappaVec * fourPiB * radEnergyFractions - chat * kappaVec * EradVec_guess);
				Rtot = sum(Rvec);
				F_G = Egas_guess - Egas0 + (c / chat) * Rtot;
				F_R = EradVec_guess - Erad0Vec - (Rvec + Src);

				// check if converged
				if ((std::abs(F_G / Etot0) < resid_tol) && ((c / chat) * max(abs(F_R)) / Etot0 < resid_tol)) {
					break;
				}

		    quokka::valarray<double, nGroups_> dkappaVec_dTgas = RadSystem<problem_t>::ComputeOpacityTempDerivativeAtBins(rho, T_gas);
				AMREX_ASSERT(!std::isnan(sum(dkappaVec_dTgas)));

				// prepare to compute Jacobian elements
				const double c_v = quokka::EOS<problem_t>::ComputeEintTempDerivative(rho, T_gas, massScalars);
				dRtot_dEgas = dt * rho / c_v * (4 * pi * kappa * dB_dTgas + dkappa_dTgas * fourPiB - chat * sum(dkappaVec_dTgas * EradVec_guess));
        AMREX_ASSERT(dRtot_dEgas > 0.0);
        // // force dRtot_dEgas to non-negative
        // dRtot_dEgas = dRtot_dEgas > 0.0 ? dRtot_dEgas : 0.0;
				dRtot_dErad = -dt * rho * kappaVec * chat;
        // TODO: fix this: dB_dTgas * radEnergyFractions is inaccurate
				dRvec_dEgas = dt * rho / c_v * (4 * pi * kappaVec * dB_dTgas * radEnergyFractions + dkappaVec_dTgas * fourPiB * radEnergyFractions - chat * dkappaVec_dTgas * EradVec_guess);
        // force dRvec_dEgas to non-negative
        // for (int g = 0; g < nGroups_; ++g) {
        //   dRvec_dEgas[g] = dRvec_dEgas[g] > 0.0 ? dRvec_dEgas[g] : 0.0;
        // }

				// compute Jacobian elements
				dFG_dEgas = 1.0 + (c / chat) * dRtot_dEgas;
        dFG_dErad = dRtot_dErad * (c / chat);
        dFR_dEgas = -1.0 * dRvec_dEgas;
        // // force dFR_dEgas to non-negative
        // // dFR_dEgas[dFR_dEgas < 0.0] = 0.0;
        // for (int g = 0; g < nGroups_; ++g) {
        //   dFR_dEgas[g] = dFR_dEgas[g] > 0.0 ? dFR_dEgas[g] : 0.0;
        // }
        AMREX_ASSERT(max(dFR_dEgas) <= 0.0);
        dFR_i_dErad_i = -1.0 * dRtot_dErad + 1.0;
				AMREX_ASSERT(!std::isnan(dFG_dEgas));
				AMREX_ASSERT(!std::isnan(sum(dFG_dErad)));
				AMREX_ASSERT(!isnan(dFR_dEgas));
				AMREX_ASSERT(!std::isnan(sum(dFR_i_dErad_i)));

        // update variables
        RadSystem<problem_t>::SolveLinearEqs(dFG_dEgas, dFG_dErad, dFR_dEgas, dFR_i_dErad_i, -F_G, -1. * F_R, deltaEgas, deltaErad);
				AMREX_ASSERT(!std::isnan(deltaEgas));
				AMREX_ASSERT(!std::isnan(sum(deltaErad)));
				EradVec_guess += deltaErad;
				Egas_guess += deltaEgas;
				AMREX_ASSERT(min(EradVec_guess) >= 0.);
				AMREX_ASSERT(Egas_guess > 0.);
			} // END NEWTON-RAPHSON LOOP

      AMREX_ALWAYS_ASSERT_WITH_MESSAGE(n < maxIter, "Newton-Raphson iteration failed to converge!");
			AMREX_ALWAYS_ASSERT(Egas_guess > 0.0);
			AMREX_ALWAYS_ASSERT(min(EradVec_guess) > 0.0);
		} // endif gamma != 1.0

		// Erad_guess is the new radiation energy (excluding work term)
		// Egas_guess is the new gas internal energy

		// 2. Compute radiation flux update
		amrex::GpuArray<amrex::Real, 3 * nGroups_> Frad_t0{};
		for (int g = 0; g < nGroups_; ++g) {
      Frad_t0[0 + 3 * g] = consPrev(i, j, k, x1RadFlux_index + numRadVars_ * g);
      Frad_t0[1 + 3 * g] = consPrev(i, j, k, x2RadFlux_index + numRadVars_ * g);
      Frad_t0[2 + 3 * g] = consPrev(i, j, k, x3RadFlux_index + numRadVars_ * g);
    }

    quokka::valarray<double, nGroups_> const kappaFluxMean = ComputeFluxMeanOpacity(rho, T_gas);
		amrex::GpuArray<amrex::Real, 3> dMomentum{0.0, 0.0, 0.0};
    amrex::GpuArray<amrex::Real, 3> const consNewCopy{consNew(i, j, k, x1GasMomentum_index), consNew(i, j, k, x2GasMomentum_index), consNew(i, j, k, x3GasMomentum_index)};

    amrex::Real x1GasMom1;
    amrex::Real x2GasMom1;
    amrex::Real x3GasMom1;

    // Method 1: apportion energy of photon groups by updating gas momentum one group at a time
		for (int g = 0; g < nGroups_; ++g) {
      // update F_{r, i}
      consNew(i, j, k, x1RadFlux_index + numRadVars_ * g) = (Frad_t0[0 + 3 * g] + (dt * advectionFluxes(i, j, k, 0))) / (1.0 + rho * kappaFluxMean[g] * chat * dt);
      consNew(i, j, k, x2RadFlux_index + numRadVars_ * g) = (Frad_t0[1 + 3 * g] + (dt * advectionFluxes(i, j, k, 1))) / (1.0 + rho * kappaFluxMean[g] * chat * dt);
      consNew(i, j, k, x3RadFlux_index + numRadVars_ * g) = (Frad_t0[2 + 3 * g] + (dt * advectionFluxes(i, j, k, 2))) / (1.0 + rho * kappaFluxMean[g] * chat * dt);

		  // Compute conservative gas momentum update
      dMomentum[0] = -(consNew(i, j, k, x1RadFlux_index + numRadVars_ * g) - Frad_t0[0 + 3 * g]) / (c * chat);
      dMomentum[1] = -(consNew(i, j, k, x2RadFlux_index + numRadVars_ * g) - Frad_t0[1 + 3 * g]) / (c * chat);
      dMomentum[2] = -(consNew(i, j, k, x3RadFlux_index + numRadVars_ * g) - Frad_t0[2 + 3 * g]) / (c * chat);

      // update gas momentum from radiation momentum of the current group
      // compute the new, imaginary gas momentum
      x1GasMom1 = consNewCopy[0] + dMomentum[0];
      x2GasMom1 = consNewCopy[1] + dMomentum[1];
      x3GasMom1 = consNewCopy[2] + dMomentum[2];

      // compute the new, real gas momentum after add the current group's momentum
      consNew(i, j, k, x1GasMomentum_index) += dMomentum[0];
      consNew(i, j, k, x2GasMomentum_index) += dMomentum[1];
      consNew(i, j, k, x3GasMomentum_index) += dMomentum[2];

      if constexpr (gamma_ != 1.0) {

        // 4a. Compute radiation work terms
        amrex::Real Egastot1 = ComputeEgasFromEint(rho, x1GasMom1, x2GasMom1, x3GasMom1, Egas_guess);
        amrex::Real dErad_work = 0.;
        if constexpr (compute_v_over_c_terms_) {
          // compute difference in gas kinetic energy before and after momentum update
          amrex::Real const Ekin1 = Egastot1 - Egas_guess;
          amrex::Real const dEkin_work = Ekin1 - Ekin0;
          // compute loss of radiation energy to gas kinetic energy
          dErad_work = -(c_hat_ / c_light_) * dEkin_work;
        } 
        // else, do not subtract radiation work in new radiation energy

        // 4b. Store new radiation energy, gas energy
        consNew(i, j, k, radEnergy_index + nGroups_ * g) = EradVec_guess[g] + dErad_work;
      } else {
        amrex::ignore_unused(EradVec_guess);
        amrex::ignore_unused(Egas_guess);
      } // endif gamma != 1.0
    } // end for g

    // Method 2: apportion energy of photon groups by the fractional work they do

    // compute difference between new and old internal energy
    amrex::Real const dEint = Egas_guess - Egas0;

    // Store new gas energy
    x1GasMom1 = consNew(i, j, k, x1GasMomentum_index);
    x2GasMom1 = consNew(i, j, k, x2GasMomentum_index);
    x3GasMom1 = consNew(i, j, k, x3GasMomentum_index);
    consNew(i, j, k, gasEnergy_index) = ComputeEgasFromEint(rho, x1GasMom1, x2GasMom1, x3GasMom1, Egas_guess);
    consNew(i, j, k, gasInternalEnergy_index) += dEint; // must compute difference

	});
}

// TODO: CCH: this function ComputeSourceTermsExplicit is never used. 
template <typename problem_t>
void RadSystem<problem_t>::ComputeSourceTermsExplicit(arrayconst_t &consPrev, arrayconst_t & /*radEnergySource*/, array_t &src, amrex::Box const &indexRange,
						      amrex::Real dt)
{
	const double chat = c_hat_;
	const double a_rad = radiation_constant_;

	// cell-centered kernel
	amrex::ParallelFor(indexRange, [=] AMREX_GPU_DEVICE(int i, int j, int k) {
		// load gas energy
		const auto rho = consPrev(i, j, k, gasDensity_index);
		const auto Egastot0 = consPrev(i, j, k, gasEnergy_index);
		const auto x1GasMom0 = consPrev(i, j, k, x1GasMomentum_index);
		const double x2GasMom0 = consPrev(i, j, k, x2GasMomentum_index);
		const double x3GasMom0 = consPrev(i, j, k, x3GasMomentum_index);
		const auto Egas0 = ComputeEintFromEgas(rho, x1GasMom0, x2GasMom0, x3GasMom0, Egastot0);
		auto massScalars = RadSystem<problem_t>::ComputeMassScalars(consPrev, i, j, k);

		// load radiation energy, momentum
		const auto Erad0 = consPrev(i, j, k, radEnergy_index);
		const auto Frad0_x = consPrev(i, j, k, x1RadFlux_index);

		// compute material temperature
		const auto T_gas = quokka::EOS<problem_t>::ComputeTgasFromEint(rho, Egas0, massScalars);

		// compute opacity, emissivity
		const auto kappa = RadSystem<problem_t>::ComputeOpacity(rho, T_gas);
		const auto fourPiB = chat * a_rad * std::pow(T_gas, 4);

		// compute reaction term
		const auto rhs = dt * (rho * kappa) * (fourPiB - chat * Erad0);
		const auto Fx_rhs = -dt * chat * (rho * kappa) * Frad0_x;

		src(radEnergy_index, i) = rhs;
		src(x1RadFlux_index, i) = Fx_rhs;
	});
}

#endif // RADIATION_SYSTEM_HPP_
