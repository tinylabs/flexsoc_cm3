#
# Custom cmake functions to support this project
#

macro( fusesoc_gw NAME )
  add_custom_target( ${NAME}
    COMMENT "Generating gateware for ${NAME}..."
    COMMAND ${FUSESOC_EXECUTABLE} --config ${PROJECT_BINARY_DIR}/fusesoc.conf run --target=${NAME} flexsoc_cm3
    )
endmacro( fusesoc_gw )

function( fusesoc_gencsr TARGET GW_TOP )
  add_custom_command(
    COMMENT "Generating CSR definitions"
    COMMAND ${FUSESOC_EXECUTABLE} --config ${PROJECT_BINARY_DIR}/fusesoc.conf run --target=lint ${GW_TOP}
    COMMAND cmake -E make_directory ${PROJECT_BINARY_DIR}/generated
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR}/build/flexsoc_cm3_0.1/lint-verilator/ -name '*.h' -exec cp {} ${PROJECT_BINARY_DIR}/generated \\\;
    COMMAND cmake -E touch ${PROJECT_BINARY_DIR}/csr_generated.txt
    OUTPUT  ${PROJECT_BINARY_DIR}/csr_generated.txt
    )
  add_custom_target( gen_csr
    DEPENDS ${PROJECT_BINARY_DIR}/csr_generated.txt
    )
  add_dependencies( ${TARGET} gen_csr )
  include_directories( ${PROJECT_BINARY_DIR}/generated )
  set_property( DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_BINARY_DIR}/csr_generated.txt )
endfunction( fusesoc_gencsr )

# Add fusesoc lib
function( fusesoc_add_lib NAME REPO )
  add_custom_command(
    COMMENT "Adding fusesoc lib: ${NAME}"
    COMMAND ${FUSESOC_EXECUTABLE} library add --location ${PROJECT_BINARY_DIR}/fusesoc_libraries/${NAME} ${NAME} ${REPO}
    COMMAND cmake -E touch ${PROJECT_BINARY_DIR}/${NAME}-lib
    OUTPUT ${PROJECT_BINARY_DIR}/${NAME}-lib
    )
  # Force generation if not there
  add_custom_target( fusesoc-lib-add-${NAME} ALL
    DEPENDS ${PROJECT_BINARY_DIR}/${NAME}-lib
    )
endfunction( fusesoc_add_lib )

macro( test_setup )
  # Decide where to run tests
  if( DEFINED ENV{FLEXSOC_HW} )
    set( FLEXSOC_HW $ENV{FLEXSOC_HW} )
    message( STATUS "Running on hw: ${FLEXSOC_HW}" )
  else ()
    set( FLEXSOC_HW "127.0.0.1:5555" )
    message( STATUS "Running on simulator: ${FLEXSOC_HW}" )
    # TODO: codify version if possible
    set( VERILATOR_SIM ${PROJECT_BINARY_DIR}/test/build/flexsoc_cm3_0.1/sim-verilator/Vflexsoc_cm3 )
    add_custom_command(
      OUTPUT ${VERILATOR_SIM}
      COMMENT "Generating verilated sim for ${CMAKE_PROJECT_NAME}"
      COMMAND ${FUSESOC_EXECUTABLE} --config ${PROJECT_BINARY_DIR}/fusesoc.conf run --target=sim flexsoc_cm3
      )
  endif ()
  # Turn on testing
  enable_testing()
endmacro( test_setup )

macro( test_finalize )
  if( DEFINED ENV{FLEXSOC_HW} )
    add_custom_target( check
      COMMAND ${CMAKE_CTEST_COMMAND} )
  else ()
    add_custom_target( check
      DEPENDS ${VERILATOR_SIM}
      COMMAND ${CMAKE_CTEST_COMMAND} )
  endif ()
endmacro( test_finalize )

# Create test
function( hw_test NAME SOURCES)
  add_executable( ${NAME} ${SOURCES} ${ARGN} )
  target_link_libraries( ${NAME} flexsoc target test )
  add_test(
    NAME ${NAME}
    COMMAND ${PROJECT_SOURCE_DIR}/test/scripts/hw_test.sh ${VERILATOR_SIM} $<TARGET_FILE:${NAME}> ${FLEXSOC_HW} )    
endfunction( hw_test )

# Create CLI tests
function( cli_test NAME SYSMAP BINARY EXT )
  add_test(
    NAME ${NAME}
    COMMAND ${PROJECT_SOURCE_DIR}/test/scripts/cli_test.sh ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/test/map/${SYSMAP} ${PROJECT_SOURCE_DIR}/test/arm/bin/${BINARY} ${FLEXSOC_HW} ${EXT} ${VERILATOR_SIM}
    )
endfunction( cli_test )
