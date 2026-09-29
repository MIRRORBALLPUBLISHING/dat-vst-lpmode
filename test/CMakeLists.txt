# plain executable tests, no framework. DatNonlinear has no JUCE dependency.
add_executable(test_datnonlinear
    test_datnonlinear.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/DatNonlinear.cpp
)
target_include_directories(test_datnonlinear PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_datnonlinear PRIVATE cxx_std_17)
add_test(NAME datnonlinear COMMAND test_datnonlinear)
