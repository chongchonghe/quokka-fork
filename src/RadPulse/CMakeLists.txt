add_executable(test_radiation_pulse test_radiation_pulse.cpp ../fextract.cpp ${QuokkaObjSources})

if(AMReX_GPU_BACKEND MATCHES "CUDA")
    setup_target_for_cuda_compilation(test_radiation_pulse)
endif(AMReX_GPU_BACKEND MATCHES "CUDA")

add_test(NAME RadiationPulse COMMAND test_radiation_pulse RadPulse.in WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
