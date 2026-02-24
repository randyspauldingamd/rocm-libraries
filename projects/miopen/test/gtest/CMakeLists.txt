find_package(GTest REQUIRED)

include(GoogleTest)

if(MIOPEN_USE_HIPBLASLT)
  find_package(hipblaslt REQUIRED PATHS /opt/rocm $ENV{HIP_PATH})
endif()

if(MIOPEN_USE_ROCBLAS)
  find_package(rocblas REQUIRED PATHS /opt/rocm $ENV{HIP_PATH})
endif()

set(SOURCES
    gtest_common.cpp
    log.cpp
    platform.cpp
    conv_common.cpp
    unit_conv_solver.cpp
    )

# We use a special entry point for HIP backend testing so we can detect and flag any HIP errors that tests don't
# clean up themselves
if(MIOPEN_BACKEND_HIP)
  list(APPEND SOURCES main_hip.cpp)
endif()

if(MIOPEN_BACKEND_OPENCL)
  list(APPEND SKIP_TESTS dumpTensorTest.cpp)
endif()

if(MIOPEN_CK_BUILDER_EXPERIMENTAL)
    add_definitions(-DCK_EXPERIMENTAL_BUILDER)
else()
    list(APPEND SKIP_TESTS ck_builder_xdl.cpp ck_builder_shared.cpp)
endif()

function(add_gtest_negative_filter NEGATIVE_FILTER_TO_ADD)
    set (TMP_FILTER ${MIOPEN_GTEST_FILTER_NEGATIVE})

    if (NOT TMP_FILTER )
        set (TMP_FILTER "-")
    else()
        set (TMP_FILTER "${TMP_FILTER}:")
    endif()
    set (TMP_FILTER ${TMP_FILTER}${NEGATIVE_FILTER_TO_ADD})

    set (MIOPEN_GTEST_FILTER_NEGATIVE ${TMP_FILTER} PARENT_SCOPE)
endfunction()

if(DEFINED THEROCK_STAGE_INSTALL_ROOT)
  set (THEROCK_GTEST_TAG "miopen_gtest_")
else()
  set (THEROCK_GTEST_TAG "")
endif()

function(add_gtest TEST_NAME TEST_CPP)
  list(JOIN TEST_CPP "  " TESTS_LIST)

  if(TEST_NAME STREQUAL "miopen_gtest")
    set (GTEST_NAME "${TEST_NAME}")
  else()
    set (GTEST_NAME "${THEROCK_GTEST_TAG}${TEST_NAME}")
  endif()

  message("Adding gtest: " ${GTEST_NAME} " : " ${TESTS_LIST})
  add_executable(${GTEST_NAME} ${TEST_CPP})
  if(WIN32)
    target_compile_definitions(${GTEST_NAME} PRIVATE NOMINMAX)
  endif()

  add_dependencies(tests ${GTEST_NAME})
  add_dependencies(check ${GTEST_NAME})

  target_compile_options(${GTEST_NAME} PRIVATE -Wno-global-constructors -Wno-undef)
  target_include_directories(${GTEST_NAME} PRIVATE ../ ../../src/kernels)

  target_link_libraries(${GTEST_NAME} miopen_gtest_common)
  if(MIOPEN_ENABLE_AI_KERNEL_TUNING)
    target_link_libraries(${GTEST_NAME} frugally-deep::fdeep Eigen3::Eigen)
  endif()

  # Need to add hipblaslt since tests directly use gemm_v2.cpp
  if(hipblaslt_FOUND)
    target_link_libraries( ${GTEST_NAME} roc::hipblaslt )
  endif()
  # Need to add rocBLAS since static builds of MIOpen require rocBLAS linkage
  if(rocblas_FOUND)
    target_link_libraries( ${GTEST_NAME} $<BUILD_INTERFACE:roc::rocblas> )
  endif()

  # Workaround : change in rocm-cmake was causing linking error so had to add ${CMAKE_DL_LIBS}
  #               We can remove ${CMAKE_DL_LIBS} once root cause is identified.
  target_link_libraries(${GTEST_NAME} ${CMAKE_DL_LIBS} GTest::gtest GTest::gtest_main GTest::gmock MIOpen ${Boost_LIBRARIES} hip::host )
  if(NOT MIOPEN_EMBED_DB STREQUAL "")
      target_link_libraries(${GTEST_NAME} $<BUILD_INTERFACE:miopen_data>)
  endif()

  if(MIOPEN_CK_BUILDER_EXPERIMENTAL)
    find_package(composable_kernel 1.0.0 COMPONENTS device_conv_operations)
    target_link_libraries(${GTEST_NAME} ck_builder composable_kernel::device_conv_operations)
  endif()

# Individual test binaries are only added if single gtest is not being created.
  if(NOT MIOPEN_TEST_SINGLE_GTEST)
    string(CONCAT TEST_ENVIRONMENT_VARIABLES
    "ENVIRONMENT;MIOPEN_USER_DB_PATH=${CMAKE_CURRENT_BINARY_DIR};"
    "ENVIRONMENT;MIOPEN_INVOKED_FROM_CTEST=1;")

    set(MIOPEN_GTEST_PREFIX)
    set(MIOPEN_GTEST_FILTER_NEGATIVE)

    if (MIOPEN_TEST_ALL)
      set(MIOPEN_GTEST_PREFIX "")
    else()
      set(MIOPEN_GTEST_PREFIX "Smoke")
    endif()

    if (NOT MIOPEN_TEST_MLIR)
        add_gtest_negative_filter("*MLIRTest*")
    endif()

    if (NOT MIOPEN_TEST_DBSYNC)
        add_gtest_negative_filter("*DBSync*")
    endif()

    if (NOT MIOPEN_TEST_DEEPBENCH)
        add_gtest_negative_filter("*DeepBench*")
    endif()

    if (NOT MIOPEN_TEST_CONV)
        add_gtest_negative_filter("*MIOpenTestConv*")
    endif()

    if(MIOPEN_NO_GPU)
        add_gtest_negative_filter("*GPU*")
    endif()

    # NONE datatype is explicitly enabled for all the state, all the NONE tests take 4 seconds to complete.
    set(MIOPEN_GTEST_FILTER "${MIOPEN_GTEST_PREFIX}*${MIOPEN_GTEST_SUFFIX}*:${MIOPEN_GTEST_PREFIX}*NONE*${MIOPEN_GTEST_FILTER_NEGATIVE}")

    #it prints the message only once
    set(TMPFNAME _dummy_static_variable_${name})
    if (NOT COMMAND ${TMPFNAME})
      message(STATUS "MIOPEN_GTEST_FILTER ${MIOPEN_GTEST_FILTER}")
      # Define a function so next time it will exist
      function(${TMPFNAME})
      endfunction()
    endif()

    # Enable CMake to discover the test binary
    # Note: Due to the following cmake issue with gtest_discover_tests https://gitlab.kitware.com/cmake/cmake/-/issues/17812 you cannot pass PROPERTIES as a list.
    #       To work around this limitation, we are passing the environment variables in the format ENVIRONMENT;value1=${value1};ENVIRONMENT;value2=${value2}.
    # bn tests are run sequentially since running tests in parallel was causing large tensor case fail with insufficient memory error.
    if("${TEST_NAME}" STREQUAL "test_bn_bwd_serial_run" OR "${TEST_NAME}" STREQUAL "test_bn_fwd_train_serial_run" OR "${TEST_NAME}" STREQUAL "test_bn_infer_serial_run")
      gtest_discover_tests(${GTEST_NAME} DISCOVERY_TIMEOUT 300 DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${DATABASE_INSTALL_DIR} PROPERTIES RUN_SERIAL TRUE ${TEST_ENVIRONMENT_VARIABLES} TEST_FILTER "${MIOPEN_GTEST_FILTER}")
    else()
      gtest_discover_tests(${GTEST_NAME} DISCOVERY_TIMEOUT 300 DISCOVERY_MODE PRE_TEST WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${DATABASE_INSTALL_DIR} PROPERTIES ${TEST_ENVIRONMENT_VARIABLES} TEST_FILTER "${MIOPEN_GTEST_FILTER}")
    endif()
  endif()

  if(MIOPEN_TEST_SINGLE_GTEST AND TEST_NAME STREQUAL "miopen_gtest")
    set(MIOPEN_GTEST_PREFIX)
    set(MIOPEN_GTEST_FILTER_NEGATIVE)

    if (MIOPEN_TEST_ALL)
      set(MIOPEN_GTEST_PREFIX "")
    else()
      set(MIOPEN_GTEST_PREFIX "Smoke")
    endif()

    if (NOT MIOPEN_TEST_MLIR)
        add_gtest_negative_filter("*MLIRTest*")
    endif()

    if (NOT MIOPEN_TEST_DBSYNC)
        add_gtest_negative_filter("*DBSync*")
    endif()

    if (NOT MIOPEN_TEST_DEEPBENCH)
        add_gtest_negative_filter("*DeepBench*")
    endif()
    
    if (NOT MIOPEN_TEST_CONV)
        add_gtest_negative_filter("*MIOpenTestConv*")
    endif()

    if(MIOPEN_NO_GPU)
        add_gtest_negative_filter("*GPU*")
    endif()
    
    if(MIOPEN_TEST_GFX115X)
        add_gtest_negative_filter("Smoke/GPU_BNFWDTrainLargeFusedActivation2D_FP32.BnV2LargeFWD_TrainCKfp32Activation/NCHW_BNSpatial_testBNAPIV1_Dim_2_test_id_32") # Temporarily disabled until gfx1151 CI nodes have fw 31 or higher installed
        add_gtest_negative_filter("Smoke/GPU_BNFWDTrainLarge2D_FP32.BnV2LargeFWD_TrainCKfp32/NCHW_BNSpatial_testBNAPIV2_Dim_2_test_id_64") # Temporarily disabled until gfx1151 CI nodes have fw 31 or higher installed
        add_gtest_negative_filter("*SerialRun3D*") # These FP32 SerialRun3D tests use so much memory that they have a risk of timing out the machine during tests
    endif()

    set(MIOPEN_GTEST_FILTER "${MIOPEN_GTEST_PREFIX}*${MIOPEN_GTEST_SUFFIX}*:${MIOPEN_GTEST_PREFIX}*NONE*${MIOPEN_GTEST_FILTER_NEGATIVE}")

    message(STATUS "MIOPEN_GTEST_FILTER ${MIOPEN_GTEST_FILTER}")

    # when building the single gtest, add them in a sharded manner
    # loop over GTEST_PARALLEL_LEVEL
    if(NOT DEFINED GTEST_PARALLEL_LEVEL)
        set(GTEST_PARALLEL_LEVEL 4)
    endif()

    foreach(i RANGE 1 ${GTEST_PARALLEL_LEVEL})
      add_test(NAME ${GTEST_NAME}_shard${i}
        COMMAND ${GTEST_NAME} --gtest_filter="Full/GPU_GLU_fwd_FP32.*:Full/GPU_GLU_fwd_FP16.*:Full/GPU_GLU_fwd_BFP16.*:Full/GPU_GLU_bwd_FP32.*:Full/GPU_GLU_bwd_FP16.*:Full/GPU_GLU_bwd_BFP16.*"
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/${DATABASE_INSTALL_DIR}"
      )
      math(EXPR SHARD_INDEX "${i} - 1")
      set_tests_properties(${GTEST_NAME}_shard${i} PROPERTIES
        ENVIRONMENT "MIOPEN_USER_DB_PATH=${CMAKE_CURRENT_BINARY_DIR};MIOPEN_INVOKED_FROM_CTEST=1;GTEST_TOTAL_SHARDS=${GTEST_PARALLEL_LEVEL};GTEST_SHARD_INDEX=${SHARD_INDEX}"
      )
    endforeach()

  endif()

  # COMMAND ${TEST_NAME} --gtest_filter=${MIOPEN_GTEST_FILTER}
  target_link_libraries(${GTEST_NAME} BZip2::BZip2)
  if(WIN32)
    # Refer to https://en.cppreference.com/w/cpp/language/types for details.
    target_compile_options(${GTEST_NAME} PRIVATE $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:Clang>:-U__LP64__>>)
  endif()
  if(HAS_LIB_STD_FILESYSTEM)
    target_link_libraries(${GTEST_NAME} stdc++fs)
  endif()
endfunction()

file(GLOB TESTS *.cpp)
# Remove files that do not contain tests
foreach(SOURCE ${SOURCES})
    list(REMOVE_ITEM TESTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
endforeach()

foreach(SOURCE ${SKIP_TESTS})
    list(REMOVE_ITEM TESTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
endforeach()

# Common static library speeds up test assembly.
# Otherwise, all files in ${SOURCES} are rebuilt for each test.
add_library(miopen_gtest_common STATIC ${SOURCES})
target_include_directories(miopen_gtest_common PRIVATE ../ ../../src/kernels)
target_link_libraries(miopen_gtest_common PRIVATE GTest::gtest MIOpen)
if(WIN32)
  # Refer to https://en.cppreference.com/w/cpp/language/types for details.
  target_compile_options(miopen_gtest_common PRIVATE $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:Clang>:-U__LP64__>>)
endif()

if(MIOPEN_TEST_DISCRETE)
  foreach(TEST ${TESTS})
    get_filename_component(BASE_NAME ${TEST} NAME_WE)
    add_gtest(test_${BASE_NAME} ${BASE_NAME}.cpp)
  endforeach()
endif()
if(MIOPEN_TEST_SINGLE_GTEST)
  foreach(TEST ${TESTS})
    get_filename_component(BASE_NAME ${TEST} NAME)
    list(APPEND TESTS_CPP ${BASE_NAME})
  endforeach()

  add_gtest(miopen_gtest "${TESTS_CPP}")

  if(MIOPEN_USE_COMPOSABLEKERNEL)
    if(MIOPEN_BUILD_CK )
        # The include directories also dont propagate correctly when using the non aliased targets.
        # Manually including them fixes this problem for inline builds.
        target_include_directories(miopen_gtest SYSTEM PRIVATE
            ${MIOPEN_CK_INCLUDE_DIR}
            ${MIOPEN_CK_BUILD_INCLUDE_DIR}
            ${MIOPEN_CK_LIBRARY_INCLUDE_DIR})
    else()
        # While building on TheRock we are missing CK include headers for miopen_gtest target
        # But we need them, for example, for unit_implicitgemm_ck_util.cpp gtest, which is part of miopen_gtest target
        find_package(composable_kernel 1.0.0 COMPONENTS utility)
        target_link_libraries(miopen_gtest composable_kernel::utility)
    endif()
  endif()

  add_custom_command(
    OUTPUT test_list
    COMMAND miopen_gtest --gtest_list_tests > test_list
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_names.py --list test_list
    DEPENDS miopen_gtest ${CMAKE_CURRENT_SOURCE_DIR}/check_names.py
    COMMENT "Checking test names"
    VERBATIM
  )

  add_custom_target(
    miopen_gtest_check
    DEPENDS test_list
  )

set(DAPPER_ROOT_DIR "${PROJECT_SOURCE_DIR}/script/dependency-parser")
set(DAPPER_SRC_DIR "${DAPPER_ROOT_DIR}")
set(DAPPER_WORK_DIR "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(DAPPER_TAG "miopen_gtest_dapper_")

set(TESTS_TO_RUN_JSON "${DAPPER_WORK_DIR}/${DAPPER_TAG}tests_to_run.json")
set(BUILD_NINJA "${PROJECT_BINARY_DIR}/build.ninja")
set(MAPPING_JSON "enhanced_dependency_mapping.json")
set(DAPPER_MAPPING_JSON "${DAPPER_WORK_DIR}/${DAPPER_TAG}${MAPPING_JSON}")
set(FIXTURES_JSON "${DAPPER_WORK_DIR}/fixtures.json")

set(PY_FIXTURES "all_gtest_fixtures")
set(PY_PARSER "enhanced_ninja_parser")
set(PY_SELECT "selective_test_filter")
set(PY_RUN_GTEST "${DEPENDENCY_PARSER_DIR}/src/run_gtest")

set(DAPPER_BUILD_NINJA "${DAPPER_WORK_DIR}/${DAPPER_TAG}build.ninja")
set(DAPPER_PARSER "${DAPPER_WORK_DIR}/${DAPPER_TAG}${PY_PARSER}.py")
set(DAPPER_SELECT "${DAPPER_WORK_DIR}/${DAPPER_TAG}${PY_SELECT}.py")
set(DAPPER_FIXTURES "${DAPPER_WORK_DIR}/${DAPPER_TAG}${PY_FIXTURES}.py")

set(DAPPER_FILES_TO_INSTALL
  "${DAPPER_ROOT_DIR}/${PY_SELECT}.py"
  "${DAPPER_ROOT_DIR}/${PY_FIXTURES}.py"
)

#set(BN_BASE_SHA c5946cf2382a46b280c402f4ff397e382df1516e)
#set(BN_TEST_SHA 21db4f318f39999b618b3f163941ea4ae378afe6)
#set(GLU_BASE_SHA b567476bbd4946c66d6d48d5910899d820796ec4)
#set(GLU_TEST_SHA 59d8b9dbd727a8030f1038b14f89e3baabf152ca)

# Generate fixtures
  add_custom_command(
    OUTPUT ${FIXTURES_JSON}
    COMMENT "Generating ${FIXTURES_JSON}"
    COMMAND ${Python3_EXECUTABLE} ${DAPPER_FIXTURES} ${DAPPER_WORK_DIR}
    WORKING_DIRECTORY ${DAPPER_WORK_DIR}
    VERBATIM
  )

# Generate mapping
# On theROCK, the filter must run on the build machine since the runners don't have the object files
  if(DEFINED THEROCK_STAGE_INSTALL_ROOT)
    add_custom_command(
      OUTPUT ${DAPPER_MAPPING_JSON}
      COMMENT "Generating '${MAPPING_JSON}'..."
      COMMAND ${Python3_EXECUTABLE} "${DAPPER_ROOT_DIR}/${PY_PARSER}.py" ./build.ninja
      COMMENT "Copying to '${DAPPER_MAPPING_JSON}'..."
      COMMAND ${CMAKE_COMMAND} -E copy ${MAPPING_JSON} ${DAPPER_MAPPING_JSON}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT ${MAPPING_JSON}
      COMMENT "Generating '${MAPPING_JSON}'..."
      COMMAND ${Python3_EXECUTABLE} "${DAPPER_PARSER}" parse ${DAPPER_BUILD_NINJA}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      VERBATIM
    )
  endif()

# Generate TESTS_TO_RUN_JSON file
  add_custom_command(
    OUTPUT "${TESTS_TO_RUN_JSON}"
    COMMENT "Generating '${TESTS_TO_RUN_JSON}'..."
    COMMAND ${Python3_EXECUTABLE} "${DAPPER_SELECT}" ${DAPPER_MAPPING_JSON} 0 0  --fixturemap=${FIXTURES_JSON}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${DAPPER_MAPPING_JSON} ${FIXTURES_JSON}
    VERBATIM
  )

  add_custom_target(
    generate_gtest_filter
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS miopen_gtest tests "${TESTS_TO_RUN_JSON}"
  )

  add_custom_target(
    diff_check
    COMMENT "Running filtered gtests..."
    ${Python3_EXECUTABLE} "${PY_RUN_GTEST}.py" "${DAPPER_WORK_DIR}/miopen_gtest" "${TESTS_TO_RUN_JSON}"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS generate_gtest_filter
    VERBATIM
  )

  message(STATUS "{DAPPER_ROOT_DIR} : '${DAPPER_ROOT_DIR}'") # TRJS
  message(STATUS "{DAPPER_SRC_DIR} : '${DAPPER_SRC_DIR}'") # TRJS
  message(STATUS "{DAPPER_WORK_DIR} : '${DAPPER_WORK_DIR}'") # TRJS
  message(STATUS "{CMAKE_INSTALL_BINDIR}" : '${CMAKE_INSTALL_BINDIR}'") # TRJS

  # leverage the fact that theROCK copies miopen_gtest* to the test machines
  if(DEFINED THEROCK_STAGE_INSTALL_ROOT)
    add_custom_target(
      generate_mapping ALL
      COMMENT "Generating mapping of source files -> test executables..."
      DEPENDS miopen_gtest ${DAPPER_MAPPING_JSON}
    )
    add_dependencies(check generate_gtest_filter)
  endif()

  if( NOT ENABLE_ASAN_PACKAGING )
    install(TARGETS miopen_gtest
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT clients)
    foreach(file IN LISTS DAPPER_FILES_TO_INSTALL)
      CMAKE_PATH(GET file FILENAME file_name)
      install(
        FILES ${file}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${DAPPER_WORK_DIR}
        RENAME "${DAPPER_TAG}${file_name}"
      )
    endforeach()
  endif()
endif()

message(STATUS "gtest env: MIOPEN_USER_DB_PATH=${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "gtest env: MIOPEN_TEST_COMPOSABLEKERNEL=${MIOPEN_TEST_COMPOSABLEKERNEL}")
