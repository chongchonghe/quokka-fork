add_executable(test_radiation_streaming test_radiation_streaming.cpp ../fextract.cpp ${QuokkaObjSources})

if(AMReX_GPU_BACKEND MATCHES "CUDA")
    setup_target_for_cuda_compilation(test_radiation_streaming)
endif(AMReX_GPU_BACKEND MATCHES "CUDA")

add_test(NAME RadiationStreaming COMMAND test_radiation_streaming RadStreaming.in WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
