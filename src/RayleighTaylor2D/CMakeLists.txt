if (AMReX_SPACEDIM GREATER_EQUAL 2)
    add_executable(test_hydro2d_rt test_hydro2d_rt.cpp ${QuokkaObjSources})
    if(AMReX_GPU_BACKEND MATCHES "CUDA")
        setup_target_for_cuda_compilation(test_hydro2d_rt)
    endif()
endif()
