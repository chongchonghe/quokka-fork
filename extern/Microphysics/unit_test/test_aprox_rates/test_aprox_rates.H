#ifndef TEST_APROX_RATES_H
#define TEST_APROX_RATES_H

#include <extern_parameters.H>
#include <variables.H>

void main_main();

void aprox_rates_test(const Box& bx,
                      const Real dlogrho, const Real dlogT, const Real dmetal,
                      const plot_t& vars,
                      Array4<Real> const sp);

void aprox_rates_extra_c12ag(const Box& bx,
                             const Real dlogrho, const Real dlogT, const Real dmetal,
                             const plot_t& vars,
                             Array4<Real> const sp);

#endif
