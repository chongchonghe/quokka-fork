add_executable(test_ode test_ode.cpp ${QuokkaObjSources})

if(AMReX_GPU_BACKEND MATCHES "CUDA")
    setup_target_for_cuda_compilation(test_ode)
endif(AMReX_GPU_BACKEND MATCHES "CUDA")

add_test(NAME ODEIntegration COMMAND test_ode ODE.in WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
