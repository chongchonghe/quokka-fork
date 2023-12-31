foreach(D IN LISTS AMReX_SPACEDIM)
    if (D EQUAL 1)
       continue()
    endif ()

    if (WIN32)
      continue()
    endif ()

    #
    # This test works for both 2D and 3D builds
    # There are two different tutorials here:
    #
    # 1) Single Vortex
    # 2) Uniform Velocity
    #


    #
    # Sources common to both cases
    #
    set(_sources Adv.cpp
                 AmrLevelAdv.cpp
                 AmrLevelAdv.H
                 bc_nullfill.cpp
                 Kernels.H
                 LevelBldAdv.cpp
                 main.cpp
                 Tagging_params.cpp)

    list(APPEND _sources  Src_K/slope_K.H
                          Src_K/flux_${D}d_K.H
                          Src_K/Adv_K.H
                          Src_K/tagging_K.H)

    list(TRANSFORM _sources PREPEND Source/)


    ###############################################################################
    #
    # Single Vortex tutorial ------------------------------------------------------
    #
    ###############################################################################
    set(_sv_exe_dir Exec/SingleVortex/)

    set(_sv_sources face_velocity_${D}d_K.H Prob_Parm.H Adv_prob.cpp Prob.cpp Prob.H)
    list(TRANSFORM _sv_sources PREPEND ${_sv_exe_dir})
    list(APPEND _sv_sources ${_sources})

    set(_input_files inputs-ci)
    list(TRANSFORM _input_files PREPEND ${_sv_exe_dir})

    setup_test(${D} _sv_sources _input_files
       BASE_NAME Advection_AmrLevel_SV
       RUNTIME_SUBDIR SingleVortex)

    unset(_sv_sources)
    unset(_sv_exe_dir)


    ###############################################################################
    #
    # Uniform Velocity tutorial ---------------------------------------------------
    #
    ###############################################################################
    set(_uv_exe_dir Exec/UniformVelocity/)

    set(_uv_sources face_velocity_${D}d_K.H Prob_Parm.H Adv_prob.cpp Prob.cpp Prob.H)
    list(TRANSFORM _uv_sources PREPEND ${_uv_exe_dir})
    list(APPEND _uv_sources ${_sources})

    set(_input_files inputs-ci)
    list(TRANSFORM _input_files PREPEND ${_uv_exe_dir})

    setup_test(${D} _uv_sources _input_files
       BASE_NAME Advection_AmrLevel_UV
       RUNTIME_SUBDIR UniformVelocity)

    unset(_uv_sources)
    unset(_uv_exe_dir)


    # Final clean up
    unset(_sources)
    unset(_input_files)
endforeach()
