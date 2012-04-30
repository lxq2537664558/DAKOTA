# Setup the cmake build with desirable settings from a jenkins ExecuteShell step

set(CTEST_SITE "$ENV{NODE_NAME}")
set(CTEST_DASHBOARD_ROOT "$ENV{WORKSPACE}")
set(CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/source")
set(CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/build")

set(CTEST_BUILD_NAME "$ENV{JOB_NAME}")
set(CTEST_NOTES_FILES "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}")

# WJB - ToDo:  Generalize for nmake on Windows
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_COMMAND "make -i -j8" )

set(CTEST_BUILD_CONFIGURATION RelWithDebInfo)
set(CTEST_DAK_DISTRO "${CTEST_SOURCE_DIRECTORY}/config/DakotaDistro.cmake")

set(CTEST_CONFIGURE_COMMAND
  "${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION}")
set(CTEST_CONFIGURE_COMMAND
  "${CTEST_CONFIGURE_COMMAND} \"-G${CTEST_CMAKE_GENERATOR}\"")
set(CTEST_CONFIGURE_COMMAND
  "${CTEST_CONFIGURE_COMMAND} \"-C${CTEST_DASHBOARD_ROOT}/BuildSetup.cmake\"")
set(CTEST_CONFIGURE_COMMAND
  "${CTEST_CONFIGURE_COMMAND} \"-C${CTEST_DAK_DISTRO}\"")
set(CTEST_CONFIGURE_COMMAND
  "${CTEST_CONFIGURE_COMMAND} \"${CTEST_SOURCE_DIRECTORY}\"")


# Now execute the "configure and make" steps

ctest_start(Continuous)

ctest_configure(BUILD "${CTEST_BINARY_DIRECTORY}")
ctest_build(BUILD "${CTEST_BINARY_DIRECTORY}")

#ctest_submit()
