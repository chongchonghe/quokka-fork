# This tests requires particle support
if (NOT AMReX_PARTICLES)
   return()
endif ()

foreach(D IN LISTS AMReX_SPACEDIM)
    set(_sources     main.cpp)
    set(_input_files inputs  )

    setup_test(${D} _sources _input_files)

    unset(_sources)
    unset(_input_files)
endforeach()
