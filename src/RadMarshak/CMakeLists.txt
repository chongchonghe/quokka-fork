
add_executable(test_radiation_marshak test_radiation_marshak.cpp ../fextract.cpp ../interpolate.cpp ${QuokkaObjSources})

if(AMReX_GPU_BACKEND MATCHES "CUDA")
    setup_target_for_cuda_compilation(test_radiation_marshak)
endif(AMReX_GPU_BACKEND MATCHES "CUDA")

add_test(NAME MarshakWave COMMAND test_radiation_marshak Marshak.in WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
