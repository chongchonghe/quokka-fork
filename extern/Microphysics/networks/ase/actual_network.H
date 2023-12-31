#ifndef actual_network_H
#define actual_network_H

#include <AMReX_REAL.H>
#include <AMReX_Array.H>

#include <fundamental_constants.H>
#include <network_properties.H>

using namespace amrex;

void actual_network_init();

const std::string network_name = "pynucastro-cxx";

namespace network
{
    extern AMREX_GPU_MANAGED amrex::Array1D<amrex::Real, 1, NumSpec> bion;
    extern AMREX_GPU_MANAGED amrex::Array1D<amrex::Real, 1, NumSpec> mion;
}

namespace Rates
{

    enum NetworkRates
    {
        k_p_c12_to_n13 = 1,
        k_he4_c12_to_o16 = 2,
        k_he4_n14_to_f18 = 3,
        k_he4_o16_to_ne20 = 4,
        k_he4_f18_to_na22 = 5,
        k_n_ne20_to_ne21 = 6,
        k_he4_ne20_to_mg24 = 7,
        k_p_ne21_to_na22 = 8,
        k_n_na22_to_na23 = 9,
        k_p_na23_to_mg24 = 10,
        k_he4_mg24_to_si28 = 11,
        k_p_al27_to_si28 = 12,
        k_he4_si28_to_s32 = 13,
        k_p_p31_to_s32 = 14,
        k_c12_c12_to_p_na23 = 15,
        k_c12_c12_to_he4_ne20 = 16,
        k_he4_n13_to_p_o16 = 17,
        k_c12_o16_to_p_al27 = 18,
        k_c12_o16_to_he4_mg24 = 19,
        k_o16_o16_to_p_p31 = 20,
        k_o16_o16_to_he4_si28 = 21,
        k_he4_f18_to_p_ne21 = 22,
        k_he4_ne21_to_n_mg24 = 23,
        k_p_na23_to_he4_ne20 = 24,
        k_p_al27_to_he4_mg24 = 25,
        k_p_p31_to_he4_si28 = 26,
        k_he4_he4_he4_to_c12 = 27,
        k_c12_c12_to_mg24_modified = 28,
        k_o16_o16_to_s32_modified = 29,
        k_c12_o16_to_si28_modified = 30,
        k_he4_s32_to_ar36_removed = 31,
        k_p_cl35_to_ar36_removed = 32,
        k_p_cl35_to_he4_s32_removed = 33,
        k_he4_ar36_to_ca40_removed = 34,
        k_p_k39_to_ca40_removed = 35,
        k_p_k39_to_he4_ar36_removed = 36,
        k_he4_ca40_to_ti44_removed = 37,
        k_p_sc43_to_ti44_removed = 38,
        k_p_sc43_to_he4_ca40_removed = 39,
        k_he4_ti44_to_cr48_removed = 40,
        k_he4_ti44_to_p_v47_removed = 41,
        k_p_v47_to_cr48_removed = 42,
        k_he4_cr48_to_fe52_removed = 43,
        k_he4_cr48_to_p_mn51_removed = 44,
        k_p_mn51_to_fe52_removed = 45,
        k_he4_fe52_to_ni56_removed = 46,
        k_he4_fe52_to_p_co55_removed = 47,
        k_p_co55_to_ni56_removed = 48,
        k_s32_he4_to_ar36_approx = 49,
        k_ar36_to_s32_he4_approx = 50,
        k_ar36_he4_to_ca40_approx = 51,
        k_ca40_to_ar36_he4_approx = 52,
        k_ca40_he4_to_ti44_approx = 53,
        k_ti44_to_ca40_he4_approx = 54,
        k_ti44_he4_to_cr48_approx = 55,
        k_cr48_to_ti44_he4_approx = 56,
        k_cr48_he4_to_fe52_approx = 57,
        k_fe52_to_cr48_he4_approx = 58,
        k_fe52_he4_to_ni56_approx = 59,
        k_ni56_to_fe52_he4_approx = 60,
        k_n13_to_p_c12_derived = 61,
        k_o16_to_he4_c12_derived = 62,
        k_f18_to_he4_n14_derived = 63,
        k_ne20_to_he4_o16_derived = 64,
        k_ne21_to_n_ne20_derived = 65,
        k_na22_to_p_ne21_derived = 66,
        k_na22_to_he4_f18_derived = 67,
        k_na23_to_n_na22_derived = 68,
        k_mg24_to_p_na23_derived = 69,
        k_mg24_to_he4_ne20_derived = 70,
        k_si28_to_p_al27_derived = 71,
        k_si28_to_he4_mg24_derived = 72,
        k_s32_to_p_p31_derived = 73,
        k_s32_to_he4_si28_derived = 74,
        k_c12_to_he4_he4_he4_derived = 75,
        k_p_o16_to_he4_n13_derived = 76,
        k_he4_ne20_to_p_na23_derived = 77,
        k_p_ne21_to_he4_f18_derived = 78,
        k_n_mg24_to_he4_ne21_derived = 79,
        k_he4_mg24_to_p_al27_derived = 80,
        k_he4_si28_to_p_p31_derived = 81,
        k_he4_s32_to_p_cl35_derived_removed = 82,
        k_ar36_to_he4_s32_derived_removed = 83,
        k_ar36_to_p_cl35_derived_removed = 84,
        k_he4_ar36_to_p_k39_derived_removed = 85,
        k_ca40_to_he4_ar36_derived_removed = 86,
        k_ca40_to_p_k39_derived_removed = 87,
        k_he4_ca40_to_p_sc43_derived_removed = 88,
        k_ti44_to_he4_ca40_derived_removed = 89,
        k_ti44_to_p_sc43_derived_removed = 90,
        k_cr48_to_he4_ti44_derived_removed = 91,
        k_cr48_to_p_v47_derived_removed = 92,
        k_p_v47_to_he4_ti44_derived_removed = 93,
        k_fe52_to_he4_cr48_derived_removed = 94,
        k_fe52_to_p_mn51_derived_removed = 95,
        k_p_mn51_to_he4_cr48_derived_removed = 96,
        k_ni56_to_he4_fe52_derived_removed = 97,
        k_ni56_to_p_co55_derived_removed = 98,
        k_p_co55_to_he4_fe52_derived_removed = 99,
        NumRates = k_p_co55_to_he4_fe52_derived_removed
    };

    // number of reaclib rates

    const int NrateReaclib = 87;

    // number of tabular rates

    const int NrateTabular = 0;

    // rate names -- note: the rates are 1-based, not zero-based, so we pad
    // this vector with rate_names[0] = "" so the indices line up with the
    // NetworkRates enum

    static const std::vector<std::string> rate_names = {
        "",   // 0
        "p_c12_to_n13",  // 1,
        "he4_c12_to_o16",  // 2,
        "he4_n14_to_f18",  // 3,
        "he4_o16_to_ne20",  // 4,
        "he4_f18_to_na22",  // 5,
        "n_ne20_to_ne21",  // 6,
        "he4_ne20_to_mg24",  // 7,
        "p_ne21_to_na22",  // 8,
        "n_na22_to_na23",  // 9,
        "p_na23_to_mg24",  // 10,
        "he4_mg24_to_si28",  // 11,
        "p_al27_to_si28",  // 12,
        "he4_si28_to_s32",  // 13,
        "p_p31_to_s32",  // 14,
        "c12_c12_to_p_na23",  // 15,
        "c12_c12_to_he4_ne20",  // 16,
        "he4_n13_to_p_o16",  // 17,
        "c12_o16_to_p_al27",  // 18,
        "c12_o16_to_he4_mg24",  // 19,
        "o16_o16_to_p_p31",  // 20,
        "o16_o16_to_he4_si28",  // 21,
        "he4_f18_to_p_ne21",  // 22,
        "he4_ne21_to_n_mg24",  // 23,
        "p_na23_to_he4_ne20",  // 24,
        "p_al27_to_he4_mg24",  // 25,
        "p_p31_to_he4_si28",  // 26,
        "he4_he4_he4_to_c12",  // 27,
        "c12_c12_to_mg24_modified",  // 28,
        "o16_o16_to_s32_modified",  // 29,
        "c12_o16_to_si28_modified",  // 30,
        "he4_s32_to_ar36_removed",  // 31,
        "p_cl35_to_ar36_removed",  // 32,
        "p_cl35_to_he4_s32_removed",  // 33,
        "he4_ar36_to_ca40_removed",  // 34,
        "p_k39_to_ca40_removed",  // 35,
        "p_k39_to_he4_ar36_removed",  // 36,
        "he4_ca40_to_ti44_removed",  // 37,
        "p_sc43_to_ti44_removed",  // 38,
        "p_sc43_to_he4_ca40_removed",  // 39,
        "he4_ti44_to_cr48_removed",  // 40,
        "he4_ti44_to_p_v47_removed",  // 41,
        "p_v47_to_cr48_removed",  // 42,
        "he4_cr48_to_fe52_removed",  // 43,
        "he4_cr48_to_p_mn51_removed",  // 44,
        "p_mn51_to_fe52_removed",  // 45,
        "he4_fe52_to_ni56_removed",  // 46,
        "he4_fe52_to_p_co55_removed",  // 47,
        "p_co55_to_ni56_removed",  // 48,
        "s32_he4_to_ar36_approx",  // 49,
        "ar36_to_s32_he4_approx",  // 50,
        "ar36_he4_to_ca40_approx",  // 51,
        "ca40_to_ar36_he4_approx",  // 52,
        "ca40_he4_to_ti44_approx",  // 53,
        "ti44_to_ca40_he4_approx",  // 54,
        "ti44_he4_to_cr48_approx",  // 55,
        "cr48_to_ti44_he4_approx",  // 56,
        "cr48_he4_to_fe52_approx",  // 57,
        "fe52_to_cr48_he4_approx",  // 58,
        "fe52_he4_to_ni56_approx",  // 59,
        "ni56_to_fe52_he4_approx",  // 60,
        "n13_to_p_c12_derived",  // 61,
        "o16_to_he4_c12_derived",  // 62,
        "f18_to_he4_n14_derived",  // 63,
        "ne20_to_he4_o16_derived",  // 64,
        "ne21_to_n_ne20_derived",  // 65,
        "na22_to_p_ne21_derived",  // 66,
        "na22_to_he4_f18_derived",  // 67,
        "na23_to_n_na22_derived",  // 68,
        "mg24_to_p_na23_derived",  // 69,
        "mg24_to_he4_ne20_derived",  // 70,
        "si28_to_p_al27_derived",  // 71,
        "si28_to_he4_mg24_derived",  // 72,
        "s32_to_p_p31_derived",  // 73,
        "s32_to_he4_si28_derived",  // 74,
        "c12_to_he4_he4_he4_derived",  // 75,
        "p_o16_to_he4_n13_derived",  // 76,
        "he4_ne20_to_p_na23_derived",  // 77,
        "p_ne21_to_he4_f18_derived",  // 78,
        "n_mg24_to_he4_ne21_derived",  // 79,
        "he4_mg24_to_p_al27_derived",  // 80,
        "he4_si28_to_p_p31_derived",  // 81,
        "he4_s32_to_p_cl35_derived_removed",  // 82,
        "ar36_to_he4_s32_derived_removed",  // 83,
        "ar36_to_p_cl35_derived_removed",  // 84,
        "he4_ar36_to_p_k39_derived_removed",  // 85,
        "ca40_to_he4_ar36_derived_removed",  // 86,
        "ca40_to_p_k39_derived_removed",  // 87,
        "he4_ca40_to_p_sc43_derived_removed",  // 88,
        "ti44_to_he4_ca40_derived_removed",  // 89,
        "ti44_to_p_sc43_derived_removed",  // 90,
        "cr48_to_he4_ti44_derived_removed",  // 91,
        "cr48_to_p_v47_derived_removed",  // 92,
        "p_v47_to_he4_ti44_derived_removed",  // 93,
        "fe52_to_he4_cr48_derived_removed",  // 94,
        "fe52_to_p_mn51_derived_removed",  // 95,
        "p_mn51_to_he4_cr48_derived_removed",  // 96,
        "ni56_to_he4_fe52_derived_removed",  // 97,
        "ni56_to_p_co55_derived_removed",  // 98,
        "p_co55_to_he4_fe52_derived_removed"  // 99,
    };

}

#ifdef NSE_NET
namespace NSE_INDEX
{
    constexpr int h1_index = 1;
    constexpr int n_index = 0;
    constexpr int he4_index = 2;

    // Each row corresponds to the rate in NetworkRates enum
    // First 3 row indices for reactants, followed by 3 product indices
    // last index is the corresponding reverse rate index.

    extern AMREX_GPU_MANAGED amrex::Array2D<int, 1, Rates::NumRates, 1, 7, Order::C> rate_indices;
    extern AMREX_GPU_MANAGED bool initialized;
}
#endif

#endif
