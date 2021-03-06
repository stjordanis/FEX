
# Careful. Globbing can't see changes to the contents of files
# Need to do a fresh clean to see changes
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS *.asm)

set(ASM_DEPENDS "")
foreach(ASM_SRC ${ASM_SOURCES})
  get_filename_component(ASM_NAME ${ASM_SRC} NAME)

  # Generate a temporary file
 set(ASM_TMP "${ASM_NAME}_TMP.asm")
 set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ASM_TMP}")
  add_custom_command(OUTPUT ${TMP_FILE}
    DEPENDS "${ASM_SRC}"
    COMMAND "cp" ARGS "${ASM_SRC}" "${TMP_FILE}"
    COMMAND "sed" ARGS "-i" "-e" "\'1s;^;BITS 64\\n;\'" "-e" "\'\$\$a\\ret\\n\'" "${TMP_FILE}"
    )

  set(OUTPUT_NAME "${ASM_NAME}.bin")
  set(OUTPUT_CONFIG_NAME "${ASM_NAME}.config.bin")

  add_custom_command(OUTPUT ${OUTPUT_NAME}
    DEPENDS "${TMP_FILE}"
    COMMAND "nasm" ARGS "${TMP_FILE}" "-o" "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")

  add_custom_command(OUTPUT ${OUTPUT_CONFIG_NAME}
    DEPENDS "${ASM_SRC}"
    COMMAND "python3" ARGS "${CMAKE_SOURCE_DIR}/Scripts/json_asm_parse.py" "${ASM_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_NAME}")

  list(APPEND ASM_DEPENDS "${OUTPUT_NAME};${OUTPUT_CONFIG_NAME}")

  # Format is "<Test Arguments>" "<Test Name>"
  set(TEST_ARGS
    "-c irint -n 1"      "int_1"
    "-c irint -n 500"    "int_500"
    "-c irint -n 500 -m" "int_500_m"
    "-c irjit -n 1"      "jit_1"
    "-c irjit -n 500"    "jit_500"
    "-c irjit -n 500 -m" "jit_500_m"
    "-c llvm -n 1"       "llvm_1"
    "-c llvm -n 500"     "llvm_500"
    "-c llvm -n 500 -m"  "llvm_500_m"
    )

  list(LENGTH TEST_ARGS ARG_COUNT)
  math(EXPR ARG_COUNT "${ARG_COUNT}-1")
  foreach(Index RANGE 0 ${ARG_COUNT} 2)
    math(EXPR TEST_NAME_INDEX "${Index}+1")

    list(GET TEST_ARGS ${Index} ARGS)
    list(GET TEST_ARGS ${TEST_NAME_INDEX} TEST_DESC)

    set(TEST_NAME "${TEST_DESC}/Test_${ASM_NAME}")
    string(REPLACE " " ";" ARGS_LIST ${ARGS})
    add_test(NAME ${TEST_NAME}
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/Scripts/testharness_runner.py"
      "${CMAKE_SOURCE_DIR}/unittests/ASM/Known_Failures"
      "Test_${ASM_NAME}"
      "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner"
      ${ARGS_LIST} "${OUTPUT_NAME}" "${OUTPUT_CONFIG_NAME}")
    # This will cause the ASM tests to fail if it can't find the TestHarness or ASMN files
    # Prety crap way to work around the fact that tests can't have a build dependency in a different directory
    # Just make sure to independently run `make all` then `make test`
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${CMAKE_BINARY_DIR}/Bin/TestHarnessRunner")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_NAME}")
    set_property(TEST ${TEST_NAME} APPEND PROPERTY DEPENDS "${OUTPUT_CONFIG_NAME}")
  endforeach()

endforeach()

add_custom_target(asm_files ALL
  DEPENDS "${ASM_DEPENDS}")
