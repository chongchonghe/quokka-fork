#ifndef EOS_HPP_
#define EOS_HPP_
// TwoMomentRad - a radiation transport library for patch-based AMR codes
// Copyright 2020 Benjamin Wibking.
// Released under the MIT license. See LICENSE file included in the GitHub repo.
//==============================================================================
/// \file EOS.hpp
/// \brief A class for equation of state calculations.

#include <cmath>
#include <optional>
#include <tuple>

#include "AMReX_Array.H"
#include "AMReX_GpuQualifiers.H"
#include "AMReX_REAL.H"
#include "physics_info.hpp"
#include <AMReX_Print.H>

#include "burn_type.H"
#include "eos.H"
#include "extern_parameters.H"

#ifdef PRIMORDIAL_CHEM
#include "actual_eos_data.H"
#endif

namespace quokka
{

// specify default values for ideal gamma-law EOS
//
template <typename problem_t> struct EOS_Traits {
	static constexpr double gamma = 5. / 3.;     // default value
	static constexpr double cs_isothermal = NAN; // only used when gamma = 1
	static constexpr double mean_molecular_weight = NAN;
	static constexpr double boltzmann_constant = C::k_B;
};

template <typename problem_t> class EOS
{

      public:
	static constexpr int nmscalars_ = Physics_Traits<problem_t>::numMassScalars;
	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeTgasFromEint(amrex::Real rho, amrex::Real Eint, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {}) -> amrex::Real;

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeEintFromTgas(amrex::Real rho, amrex::Real Tgas, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {}) -> amrex::Real;

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeEintFromPres(amrex::Real rho, amrex::Real Pressure, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {})
	    -> amrex::Real;

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeEintTempDerivative(amrex::Real rho, amrex::Real Tgas, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {})
	    -> amrex::Real;

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeOtherDerivatives(amrex::Real rho, amrex::Real P, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {});

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputePressure(amrex::Real rho, amrex::Real Eint, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {}) -> amrex::Real;

	[[nodiscard]] AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE static auto
	ComputeSoundSpeed(amrex::Real rho, amrex::Real Pressure, const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars = {}) -> amrex::Real;

      private:
	static constexpr amrex::Real gamma_ = EOS_Traits<problem_t>::gamma;
	static constexpr amrex::Real boltzmann_constant_ = EOS_Traits<problem_t>::boltzmann_constant;
	static constexpr amrex::Real mean_molecular_weight_ = EOS_Traits<problem_t>::mean_molecular_weight;
};

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputeTgasFromEint(amrex::Real rho, amrex::Real Eint,
										  const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
    -> amrex::Real
{
	// return temperature for an ideal gas given density and internal energy
	amrex::Real Tgas = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	chemstate.e = Eint / rho;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_re, chemstate);
	Tgas = chemstate.T;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.e = Eint / rho;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_re, estate);
		// scale returned temperature in case boltzmann constant is dimensionless
		Tgas = estate.T * C::k_B / boltzmann_constant_;
	}
#endif
	return Tgas;
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputeEintFromTgas(amrex::Real rho, amrex::Real Tgas,
										  const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
    -> amrex::Real
{
	// return internal energy density given density and temperature
	amrex::Real Eint = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	// Define and initialize Tgas here
	amrex::Real const Tgas_value = Tgas;
	chemstate.T = Tgas_value;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_rt, chemstate);
	Eint = chemstate.e * chemstate.rho;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.T = Tgas;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_rt, estate);
		Eint = estate.e * rho * boltzmann_constant_ / C::k_B;
	}
#endif
	return Eint;
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputeEintFromPres(amrex::Real rho, amrex::Real Pressure,
										  const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
    -> amrex::Real
{
	// return internal energy density given density and pressure
	amrex::Real Eint = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	chemstate.p = Pressure;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_rp, chemstate);
	Eint = chemstate.e * chemstate.rho;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.p = Pressure;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_rp, estate);
		Eint = estate.e * rho;
	}
#endif
	return Eint;
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto
EOS<problem_t>::ComputeEintTempDerivative(const amrex::Real rho, const amrex::Real Tgas,
					  const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars) -> amrex::Real
{
	// compute derivative of internal energy w/r/t temperature, given density and temperature
	amrex::Real dEint_dT = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	// we don't need Tgas to find chemstate.dedT, but we still need to initialize chemstate.T because we are using the 'rt' EOS mode
	chemstate.T = NAN;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_rt, chemstate);
	dEint_dT = chemstate.dedT * chemstate.rho;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.T = Tgas;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_rt, estate);
		dEint_dT = estate.dedT * rho * boltzmann_constant_ / C::k_B;
	}
#endif
	return dEint_dT;
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputeOtherDerivatives(const amrex::Real rho, const amrex::Real P,
										      const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
{
	// compute derivative of specific internal energy w/r/t density, given density and pressure
	amrex::Real deint_dRho = NAN;
	// compute derivative of specific internal energy w/r/t density, given density and pressure
	amrex::Real deint_dP = NAN;
	// compute derivative of density w/r/t pressure, given density and pressure
	amrex::Real dRho_dP = NAN;
	// compute derivative of pressure w/r/t density at constant entropy, given density and pressure (needed for the fundamental derivative G)
	amrex::Real dP_dRho_s = NAN;
	// fundamental derivative
	amrex::Real G = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	chemstate.p = P;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_rp, chemstate);
	deint_dRho = chemstate.dedr;
	deint_dP = 1.0 / chemstate.dpde;
	dRho_dP = 1.0 / (chemstate.dpdr * C::k_B / boltzmann_constant_);
	dP_dRho_s = chemstate.cs * chemstate.cs;
	G = chemstate.G;

#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.p = P;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_rp, estate);
		deint_dRho = estate.dedr;
		deint_dP = 1.0 / estate.dpde;
		dRho_dP = 1.0 / (estate.dpdr * C::k_B / boltzmann_constant_);
		dP_dRho_s = estate.cs * estate.cs;
		G = estate.G;
	}
#endif
	return std::make_tuple(deint_dRho, deint_dP, dRho_dP, dP_dRho_s, G);
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputePressure(amrex::Real rho, amrex::Real Eint,
									      const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
    -> amrex::Real
{
	// return pressure for an ideal gas
	amrex::Real P = NAN;
#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	chemstate.e = Eint / rho;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_re, chemstate);
	P = chemstate.p;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		// if rho is 0, pass 0 to state.e
		if (rho == 0.0) {
			estate.e = 0;
		} else {
			estate.e = Eint / rho;
		}
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_re, estate);
		P = estate.p;
	}
#endif
	return P;
}

template <typename problem_t>
AMREX_FORCE_INLINE AMREX_GPU_HOST_DEVICE auto EOS<problem_t>::ComputeSoundSpeed(amrex::Real rho, amrex::Real Pressure,
										const std::optional<amrex::GpuArray<amrex::Real, nmscalars_>> massScalars)
    -> amrex::Real
{
	// return sound speed for an ideal gas
	amrex::Real cs = NAN;

#ifdef PRIMORDIAL_CHEM
	eos_t chemstate;
	chemstate.rho = rho;
	chemstate.p = Pressure;
	// initialize array of number densities
	for (int ii = 0; ii < NumSpec; ++ii) {
		chemstate.xn[ii] = -1.0;
	}

	if (massScalars) {
		const auto &massArray = *massScalars;
		for (int nn = 0; nn < nmscalars_; ++nn) {
			chemstate.xn[nn] = massArray[nn] / spmasses[nn]; // massScalars are partial densities (massFractions * rho)
		}
	}

	eos(eos_input_rp, chemstate);
	cs = chemstate.cs;
#else
	if constexpr (gamma_ != 1.0) {
		chem_eos_t estate;
		estate.rho = rho;
		estate.p = Pressure;
		estate.mu = mean_molecular_weight_ / C::m_u;
		eos(eos_input_rp, estate);
		cs = estate.cs;
	}
#endif
	return cs;
}

} // namespace quokka

#endif // EOS_HPP_
