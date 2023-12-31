if (NOT (3 IN_LIST AMReX_SPACEDIM) OR NOT CMAKE_Fortran_COMPILER_LOADED)
   return()
endif ()


set(_sources main.cpp CNS_advance.cpp CNSBld.cpp CNS.cpp CNS_F.H CNS.H CNS_init_eb2.cpp CNS_io.cpp
   CNS_setup.cpp )

set(_diffusion_sources cns_diff_mod.F90 cns_eb_diff_mod.F90 cns_eb_diff_wall.F90 diff_coef_mod.F90)
list(TRANSFORM _diffusion_sources PREPEND diffusion/)

set(_fortran_sources bc_fill_nd.F90 CNS_derive.F90 CNS_divop.F90 CNS_dudt.F90 CNS_f.F90 CNS_nd.F90
   CNS_physics.F90 CNS_tagging.F90 )
list(TRANSFORM _fortran_sources PREPEND fortran/)

set(_hydro_sources analriem3d.F90 cns_eb_hyp_wall.F90 Hyp_gamma_MOL_EB.F90 Hyp_gamma_MOL.F90
   slope_mol_3d_gamma_EB.F90 slope_mol_3d_gamma.F90)
list(TRANSFORM _hydro_sources PREPEND hydro/)

list(APPEND _sources ${_diffusion_sources} ${_fortran_sources} ${_hydro_sources})
list(TRANSFORM _sources PREPEND Source/)

unset(_diffusion_sources)
unset(_fortran_sources)
unset(_hydro_sources)


##########################################################################################
#
# Sod tutorial
#
##########################################################################################
set(_sod_sources Exec/Sod/cns_prob.F90 ${_sources})
set(_input_files Exec/Sod/inputs-ci)

setup_test(3 _sod_sources _input_files
   HAS_FORTRAN_MODULES
   BASE_NAME CNS_Sod
   RUNTIME_SUBDIR Sod)

unset(_sod_sources)


#
# Clean-up
#
unset(_sources)
unset(_input_files)
