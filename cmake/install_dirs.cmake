# SPDX-License-Identifier: BSD-3-Clause

include(GNUInstallDirs)

set(OSEVENTS_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/osevents-${PROJECT_VERSION_MAJOR}"
	CACHE FILEPATH "Directory into which cmake related files (e.g. osevents-config.cmake) are installed")

set(OSEVENTS_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}"
	CACHE FILEPATH "Directory into which to install libPerm executables")

set(OSEVENTS_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}"
	CACHE FILEPATH "Directory into which libPerm libraries (except DLLs on Windows) are installed")

set(OSEVENTS_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}"
	CACHE FILEPATH "Directory into which the 'osevents' directory with all libPerm header files is installed")
