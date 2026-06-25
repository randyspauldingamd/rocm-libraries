macro(dapper_init)
    # TRJS Dapper currently requires all tests to be built. This may change prior to moving to TheRock CI,
    # so leave the logic and hardwire them on temporarily.
#    set(MIOPEN_TEST_DISCRETE 0)
    set(MIOPEN_TEST_SINGLE_GTEST 1)

    # TRJS
    message(STATUS "------------------------------------ CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "------------------------------------ CMAKE_SOURCE_DIR:       ${CMAKE_SOURCE_DIR}")
    message(STATUS "------------------------------------ CMAKE_BINARY_DIR:       ${CMAKE_BINARY_DIR}")

    set(MIOPEN_DAPPER_SRC_DIR "${CMAKE_BINARY_DIR}/../script/dependency-parser")
    set(MIOPEN_DAPPER_OUT_DIR "${CMAKE_BINARY_DIR}")
    set(SHARDS_FILE ${MIOPEN_DAPPER_OUT_DIR}/miopen_gtest_shards.txt)
    set(BUILD_NINJA "${MIOPEN_DAPPER_OUT_DIR}/build.ninja")
    set(SHAS_JSON "${MIOPEN_DAPPER_OUT_DIR}/miopen_dapper_shas.txt")
    set(MAPPING_JSON "${MIOPEN_DAPPER_OUT_DIR}/miopen_dapper_mapping.json")
    set(FIXTURES_JSON "${MIOPEN_DAPPER_OUT_DIR}/miopen_dapper_fixtures.json")
    set(TESTS_JSON "${MIOPEN_DAPPER_OUT_DIR}/miopen_dapper_tests.json")
    set(PY_MAIN "/${MIOPEN_DAPPER_SRC_DIR}/main.py")
    set(PY_FIXTURES "/${MIOPEN_DAPPER_SRC_DIR}/src/extract_gtest_fixtures.py")
    set(MIOPEN_GTEST_RUNNER "/${MIOPEN_DAPPER_SRC_DIR}/src/miopen_gtest_runner.py")
    set(MIOPEN_DAPPER_DIFF ${MIOPEN_DAPPER_SRC_DIR}/src/dapper_diff.py)

    # Get shas
    add_custom_command(
        OUTPUT ${SHAS_JSON}
        COMMENT "Generating ${SHAS_JSON}"
        COMMAND ${Python_EXECUTABLE} ${PY_MAIN} shas
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        VERBATIM
    )

    # Generate fixtures
    add_custom_command(
        OUTPUT ${FIXTURES_JSON}
        COMMENT "Generating ${FIXTURES_JSON}"
        COMMAND ${Python_EXECUTABLE} ${PY_FIXTURES}
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        DEPENDS ${SHAS_JSON}
        VERBATIM
    )

    # Generate mapping
    add_custom_command(
        OUTPUT ${MAPPING_JSON}
        COMMENT "Generating ${MAPPING_JSON}"
        COMMAND ${Python_EXECUTABLE} ${PY_MAIN} parse ${BUILD_NINJA}
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        DEPENDS tests
        VERBATIM
    )

    # Generate TESTS_JSON file
    add_custom_command(
        OUTPUT "${TESTS_JSON}"
        COMMENT "Generating ${TESTS_JSON}"
        COMMAND ${Python_EXECUTABLE} "${PY_MAIN}" select ${MAPPING_JSON}  --fixturemap=${FIXTURES_JSON} --shardsfile=${SHARDS_FILE}
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        DEPENDS ${MAPPING_JSON} ${FIXTURES_JSON}
        VERBATIM
    )
    add_custom_target(dapper_tests DEPENDS "${TESTS_JSON}")
    add_dependencies(check dapper_tests)

    add_custom_target(
        diff_check
        COMMENT "Running filtered gtests..."
        COMMAND ${Python_EXECUTABLE} "${MIOPEN_GTEST_RUNNER}" "${MIOPEN_DAPPER_OUT_DIR}/bin/miopen_gtest" "${TESTS_JSON}"
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        DEPENDS miopen_gtest tests "${TESTS_JSON}"
        VERBATIM
    )
endmacro()

macro(dapper_dev_filters)
# TODO Fix Dapper default minimal run if no work to do. It's in calc_union_filter but in mici this is ran by dapper_diff which does not run until after check

    message(STATUS "================== DAPPER DEVELOPMENT FILTERS")

    set(MIOPEN_DAPPER_DEV_FILTER_SHORT "*GPU_TestMhaFind20_FP32*:*GPU_TestMhaFind20_FP16*")
    set(MIOPEN_DAPPER_DEV_FILTER_LONGER "Smoke/GPU_BNCKFWDTrainLarge2D_FP16*:Smoke/GPU_BNOCLFWDTrainLarge2D_FP16*:Smoke/GPU_BNOCLFWDTrainLarge3D_FP16*:Smoke/GPU_BNCKFWDTrainLarge2D_BFP16*:Smoke/GPU_BNOCLFWDTrainLarge2D_BFP16*:Smoke/GPU_BNOCLFWDTrainLarge3D_BFP16*:Smoke/GPU_UnitTestConvSolverImplicitGemmFwdXdlops_FP16*:Smoke/GPU_UnitTestConvSolverImplicitGemmFwdXdlops_BFP16*-Smoke/GPU_BNOCLFWDTrainLarge2D_BFP16*:Smoke/GPU_BNOCLInferLarge2D_BFP16*:*/GPU_MIOpenDriver*:*GPU_TestMhaFind20_FP32*:*GPU_TestMhaFind20_FP16*:Smoke/GPU_BNOCLInferLarge2D_BFP16*:Smoke/GPU_BNOCLFWDTrainLarge2D_BFP16*")
    set(MIOPEN_DAPPER_DEV_FILTER_LONG "*Fusion*:*/GPU_BNBWD*_*:*/GPU_BNOCLBWD*_*:*/GPU_BNFWD*_*:*/GPU_BNOCLFWD*_*:*/GPU_BNInfer*_*:*/GPU_BNActivInfer_*:*/GPU_BNOCLInfer*_*:*/GPU_bn_infer*_*:CPU_*:*/CPU_*:*/GPU_Cat_*:*/GPU_ConvBiasActiv*:*/GPU_Conv*:*/GPU_conv*:*/GPU_UnitTestConv*:*/GPU_GetitemBwd*:*/GPU_GLU_*:*/GPU_GroupConv*:*/GPU_GroupNorm_*:*/GPU_GRUExtra_*:*/GPU_TestActivation*:*/GPU_HipBLASLtGEMMTest*:*/GPU_KernelTuningNetTestConv*:*/GPU_Kthvalue_*:*/GPU_LayerNormTest*:*/GPU_LayoutTransposeTest_*:*/GPU_Lrn*:*/GPU_lstm_extra*:*/GPU_MultiMarginLoss_*:*/GPU_ConvNonpack*:*/GPU_PerfConfig_HipImplicitGemm*:*/GPU_AsymPooling2d_*:*/GPU_WidePooling2d_*:*/GPU_PReLU_*:*/GPU_Reduce*:*/GPU_reduce_custom_*:*/GPU_regression_issue_*:*/GPU_RNNExtra_*:*/GPU_RoPE*:*/GPU_SoftMarginLoss*:*/GPU_T5LayerNormTest_*:*/GPU_Op4dTensorGenericTest_*:*/GPU_TernaryTensorOps_*:*/GPU_unaryTensorOps_*:*/GPU_Transformers*:*/GPU_TunaNetTest_*:*/")

    if(DEFINED MIOPEN_DEBUG_DEV_FILTER)
        set(MIOPEN_DEBUG_DEV_FILTER_IN "${MIOPEN_DEBUG_DEV_FILTER}")
        string(TOUPPER "${MIOPEN_DEBUG_DEV_FILTER_IN}" MIOPEN_DEBUG_DEV_FILTER)
        message(STATUS "User-specified development category='${MIOPEN_DEBUG_DEV_FILTER}'")

        if(MIOPEN_DEBUG_DEV_FILTER STREQUAL "SHORT")
            set(MIOPEN_DEV_FILTER "${MIOPEN_DEBUG_DAPPER_FILTER}:${MIOPEN_DAPPER_DEV_FILTER_SHORT}")
        elseif(MIOPEN_DEBUG_DEV_FILTER STREQUAL "LONGER")
            set(MIOPEN_DEV_FILTER "${MIOPEN_DEBUG_DAPPER_FILTER}:${MIOPEN_DAPPER_DEV_FILTER_LONGER}")
        elseif(MIOPEN_DEBUG_DEV_FILTER STREQUAL "LONG")
            set(MIOPEN_DEV_FILTER "${MIOPEN_DEBUG_DAPPER_FILTER}:${MIOPEN_DAPPER_DEV_FILTER_LONG}")
        else()
            message(FATAL_ERROR "Aborting. Unknown MIOPEN_DEBUG_DEV_FILTER: '{MIOPEN_DEBUG_DEV_FILTER}'")
        endif()

        set(MIOPEN_CATEGORY "${MIOPEN_DEBUG_DEV_FILTER}")
        message(STATUS "Applying Category '${MIOPEN_DEBUG_DEV_FILTER}' with user filters '${MIOPEN_DEBUG_DAPPER_FILTER}'")
        set(MIOPEN_GTEST_FILTER "${MIOPEN_DEV_FILTER}")
        message(STATUS "gtest_filter=MIOPEN_DEV_FILTER: ${MIOPEN_DEV_FILTER}")
    endif()
endmacro()

macro(dapper_add_sharded_test)
    if(NOT DEFINED MIOPEN_CATEGORY)
        set(MIOPEN_CATEGORY "NONE")
    endif()
    add_test(NAME miopen_gtest_sharded_dapper
        COMMAND ${Python_EXECUTABLE} ${MIOPEN_DAPPER_DIFF} "${TESTS_JSON}" "${MIOPEN_CATEGORY}" "${MIOPEN_GTEST_FILTER}"
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
    )
    set_tests_properties(miopen_gtest_sharded_dapper PROPERTIES DEPENDS "${MIOPEN_GTEST_SHARDS}")

    # TRJS dapper_diff may be redundant; 'miopen_gtest_sharded_dapper' is ran automatically by ctest
    add_custom_target(dapper_diff
        COMMAND ${Python_EXECUTABLE} ${MIOPEN_DAPPER_DIFF} "${TESTS_JSON}" "${MIOPEN_CATEGORY}" "${MIOPEN_GTEST_FILTER}"
        WORKING_DIRECTORY ${MIOPEN_DAPPER_OUT_DIR}
        DEPENDS check test_immed_conv3d test_tensor_vec test_conv3d_find2
        VERBATIM
    )
endmacro()
