if (AMReX_SPACEDIM GREATER_EQUAL 2)
    add_executable(test_advection2d test_advection2d.cpp ${QuokkaObjSources})
    if(AMReX_GPU_BACKEND MATCHES "CUDA")
        setup_target_for_cuda_compilation(test_advection2d)
    endif()
    
    add_test(NAME Advection2D COMMAND test_advection2d advection2d_amr.in WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
endif()
