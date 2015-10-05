# Cmake template for generic projects by Dario Facchini
# Current implementation: OculusSDK 0.5 (static linking), OgreSDK 1.9 (dynamic linking)
# - - - - - - - - - - - - - - - - - - - - - - - - - - -
# WINDOWS: to make application find mandatory .dll (not the plugins), just add Ogre bin directories to PATH
# WINDOWS: to run this cmake, please set OGRE_HOME as environment variable to your OgreSDK main folder
# WINDOWS: to run this cmake, please set OCULUS_HOME as environment variable to your OculusSDK main folder
# - - - - - - - - - - - - - - - - - - - - - - - - - - -
# FULLY AND EASILY CUSTOMIZABLE directory names
# "CONFIGURE" and "GENERATE" a custom project in a subdirectory (the "working directory") of cmake current folder
# AUTOFINDS include (.h and .hpp) and source files (.cpp) recursively in the dedicated directories
# BUILD FILES are saved in a dedicated directory (different from project subfolder)
# DEDICATED distribution FOLDER (build, configuration and media files are automatically copied on INSTALL)
# ON "INSTALL" - for Ogre:
#	- release executable is copied (if existing)
#	- all needed release .dll/.so are copied to distribution dir
# 	- all needed release plugins .dll/.so (defined in plugins.cfg) are copied to plugins distribution subdir
#	- all needed resources (defined in resources.cfg, both from Ogre folder and from media folder) are copied to media distribution subdir
#	- all release configuration files are copied in config distribution subdir and updated to reference the copied files
# A PostInstall.cmake script template is used to parse configuration files and copy all contents.
# ------------------------------------------------------
# THIS IS REALLY HELPFUL TO UNDERSTAND HOW CMAKE WORKS MORE OR LESS: http://www.kaizou.org/2014/11/typical-cmake-project/
#

cmake_minimum_required(VERSION 2.8.12)
cmake_policy(VERSION 2.8.12)
#cmake_policy(SET CMP0048 OLD)			# how to set project version (retrocompatibility with OLD rule)
#cmake_policy(SET CMP0040 OLD)

# useful boolean variable to understand fast if this project is used as subproject or main project
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(IS_SUBPROJECT false)
else()
	set(IS_SUBPROJECT true)
endif()

# MACRO to set value of a variable if not already set
# useful when we don't know if this project is used as a subproject or not
# (therefore some variable could have been already set from top-level cmakefile)
macro(setifnotdef var_name var_value)
	if(NOT DEFINED ${var_name})
		if(IS_SUBPROJECT)
			set(${var_name}	"${var_value}" PARENT_SCOPE)
			set(${var_name}	"${var_value}")
		else()
			set(${var_name} "${var_value}")
		endif()
	endif()
endmacro()


################################################################
#                   PROJECT SETTINGS (CUSTOMIZE THIS)          #
################################################################
# Please refer to the README before modifying!!

# project name
project (OgreOculusApp)
set(OgreOculusApp_VERSION_MAJOR 0)
set(OgreOculusApp_VERSION_MINOR 2)
set(OgreOculusApp_VERSION_PATCH 2)

# directories names relative to CMAKE_CURRENT_SOURCE_DIR
set (libs_dir_name	 "libs")			# name of the dir containing (ALL or SOME) subprojects required to this project
set (source_dir_name "src")				# name of the dir containing ALL implementations
set (header_dir_name "include")			# name of the dir containing ALL declarations
set (exe_name "OgreOculusDemo")			# name of executable, IF built separately
set (sample_media_dir_name  "media")	# name of the dir containing SAMPLE/STANDARD media files for this project
set (sample_config_dir_name "cfg")		# name of the dir containing SAMPLE/STANDARD configuration files for this project
set (cmake_dir_name "cmake")			# name of the dir containing cmake files needed to this project
										# N.B.	cmake will search in the standard cmake directory,
										#		then in this directory,
										#		then in any cmake folder of any project including this project,
										#		then will search into sdk own cmake directories (if they have any)
set (install_script_name "PostInstall")	# name of the TEMPLATE cmake file for generating install script of this project
										# all @VARS@ in this file will be replaced with current specific values
										# at the end of this script with configure_file() and then used at install time.
										# This is done using local variable values in an install script!

# DEFAULT directory names relative to CMAKE_SOURCE_DIR
# these folder names will be used if:
#	- this is the main project
#	- this is not the main project and no parent project defined them
# simply SET these variables IN YOUR PARENT PROJECT to override standard names
setifnotdef (binary_dir_name "build")					# name of the dir containing build results (Release, Debug, ..), IF built separately
setifnotdef (distribution_dir_name "dist")				# name of the dir that will contain final distribution files on INSTALL, such as:
														#	- final release executable files and .dlls/.sos (in root)
														#	- data/media files (in /media_dir_name)
														#	- configuration files (in /config_dir_name)
														#	- runtime plugins (in /plugin_dir_name)
setifnotdef (media_dir_name "${sample_media_dir_name}")	# name of the dir containing final project's LOCAL media
														# (LOCAL = media included with the project, GLOBAL = media referenced from any other system directory)
														# this is ALSO the name of the subdir where both LOCAL and GLOBAL media will be INSTALLED in /distribution_dir_name
														# WARNING: BE SURE TO REFERENCE THESE FILES CORRECTLY IN ANY CONFIGURATION FILE!!
setifnotdef (config_dir_name "${sample_config_dir_name}")# name of the dir containing final project's CONFIGURATION FILES
														# this is ALSO the name of the subdir where .cfg files will be INSTALLED in /distribution_dir_name
														# WARNING: BE SURE TO REFERENCE THESE FILES CORRECTLY IN THE PROGRAM!!
setifnotdef (plugins_dir_name "plugins")				# name of the subdir in /distribution_dir_name that will contain plugins on INSTALL




################################################################
#                   PROJECT/BUILD SETUP     		           #
################################################################

# lib extensions dependent on OS (useful when in need to manually work on some lib files)
if(WIN32)
	set(sl "lib")
	set(dl "dll")
endif()
if(UNIX)
	set(sl "a")
	set(dl "so")
endif()

# cmake project variables
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}							# (this is the standard cmake directory)
	"${CMAKE_CURRENT_SOURCE_DIR}/${cmake_dir_name}" # append: where personal project cmake files are stored
	)

# cmake build variables
if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()
unset(LIBRARIES_TO_LINK)
unset(HEADERS_TO_INCLUDE_PUBLIC)
unset(HEADERS_TO_INCLUDE_PRIVATE)

# cmake binary/build location
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/${binary_dir_name})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
if(IS_SUBPROJECT)
	set (OUTPUT_FILE_NAME ${PROJECT_NAME})
else()
	set (OUTPUT_FILE_NAME ${exe_name})
endif()

# cmake install/distribution variables
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT DEFINED CMAKE_INSTALL_PREFIX)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/${distribution_dir_name}" CACHE PATH "Default install path" FORCE )
    message(STATUS "${PROJECT_NAME}: CMAKE_INSTALL_PREFIX was not defined. Setting install directory to: '${CMAKE_INSTALL_PREFIX}'")
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT false)
endif()
set(CMAKE_INSTALL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${cmake_dir_name}/${install_script_name}.cmake")	# this is cmake template for generating CMAKE_INSTALL_SCRIPT_SPECIFIC
set(CMAKE_INSTALL_SCRIPT_SPECIFIC "${CMAKE_CURRENT_SOURCE_DIR}/${cmake_dir_name}/${install_script_name}.specific.cmake") # result script with hardcoded variable values

# project configuration files location
set(PROJECT_CONFIG_DIR ${CMAKE_SOURCE_DIR}/${config_dir_name})
set(PROJECT_MEDIA_DIR ${CMAKE_SOURCE_DIR}/${media_dir_name})

################################################################
#                   OGRE SETUP	                               #
################################################################

# check if there is a bundled version of OculusSDK in this project lib folder
# NOTE:	default is giving priority to the bundled version, this is because it can be specifically tweaked for this project.
#		If you don't like it, disable it by setting USE_BUNDLED_OculusSDK to FALSE
if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/OGRE")
	message("Bundled OGRE found in '/${libs_dir_name}'.")
	set (BUNDLED_OGRE_FOUND true)
	# give the option to use it (true by default)
	set (USE_BUNDLED_OGRE true CACHE BOOL "Use the version of OGRE bundled in this project (within '/${libs_dir_name}' folder)" )
else()
	# message(STATUS "No bundled Ogre found in '/${libs_dir_name}'.")
	set (BUNDLED_OGRE_FOUND false)
	# don't show any option (not a cache variable)
	set (USE_BUNDLED_OGRE false)
endif()

# --------------------------------------------------------------
# - Ogre installation directory includes cmake files. findOculusSDK.cmake can be searched from there or, alternatively, from local /cmake folder.
# - findOgre.cmake uses OGRE_HOME if you set it, otherwise he will try to search for it on its own.
# --------------------------------------------------------------

# if using bundled OGRE
if(BUNDLED_OGRE_FOUND AND USE_BUNDLED_OGRE)
	# set OGRE_HOME cmake variable as OGRE local path
	if(DEFINED ENV{OGRE_HOME} OR DEFINED OGRE_HOME)
		message("Overriding 'ENV{OGRE_HOME}'. Uncheck 'USE_BUNDLED_OGRE' flag if you wish to use your OGRE_HOME instead.")
	endif()
	set(OGRE_HOME "${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/OGRE")
# else, USE STANDARD SEARCH OculusSDK
else()
	# check if OGRE_HOME is defined
	if (NOT DEFINED ENV{OGRE_HOME} AND NOT DEFINED OGRE_HOME)
		message(STATUS "OGRE_HOME not defined (as Environment variable or cmake variable). A valid findOgre.cmake must be found into project's directory. Then it will try to search for Ogre installation in your computer.")
		# give the option to setup OGRE_HOME manually
		set(OGRE_HOME "${OGRE_HOME}" CACHE PATH "Location of your Ogre libraries")
	# if at least one of them is defined, use it
	else()
		if (NOT DEFINED OGRE_HOME)
			message("Environment variable OGRE_HOME found. This will be used as OgreSDK folder.")
			set(OGRE_HOME $ENV{OGRE_HOME})
		endif()
		if (NOT DEFINED OGRE_BUILD OR "${OGRE_BUILD}" STREQUAL "")
			set(OGRE_BUILD ${OGRE_HOME})
		endif()
		if( NOT DEFINED OGRE_SOURCE_DIR OR "${OGRE_SOURCE_DIR}" STREQUAL "")
			set(OGRE_SOURCE_DIR ${OGRE_HOME})
		endif()

		# add cmake directory from installation, if available
		if(WIN32)
			# set cmake directory for OGRE (will be used when findOgre.cmake is not found in project's local "cmake" dir)
			set(CMAKE_MODULE_PATH "${OGRE_HOME}/CMake/;${CMAKE_MODULE_PATH}")	# append: OGRE personal cmake directory
			set(OGRE_SAMPLES_INCLUDEPATH "${OGRE_HOME}/Samples/Common/include")
		endif()

		if(UNIX)
			# look for cmake directory for OGRE (will be used when findOgre.cmake is not found in project's local "cmake" dir)
			# different options assumed here since in unix cmake installation directory is often different from Ogre lib directory
			if(EXISTS "/usr/local/lib/OGRE/cmake")
		 
			  set(CMAKE_MODULE_PATH "/usr/local/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
			  set(OGRE_SAMPLES_INCLUDEPATH "/usr/local/share/OGRE/samples/Common/include/") # We could just *assume* that developers uses this basepath : /usr/local
		 
			elseif(EXISTS "/usr/lib/OGRE/cmake")
		 
			  set(CMAKE_MODULE_PATH "/usr/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
			  set(OGRE_SAMPLES_INCLUDEPATH "/usr/share/OGRE/samples/Common/include/") # Otherwise, this one
		 
			elseif(EXISTS "/usr/share/OGRE/cmake")

			  set(CMAKE_MODULE_PATH "/usr/share/OGRE/cmake/;${CMAKE_MODULE_PATH}")
			  set(OGRE_SAMPLES_INCLUDEPATH "/usr/share/OGRE/samples/Common/include/") # Otherwise, this one
		 
			  message(STATUS "Failed to find Ogre cmake module path. A valid findOgre.cmake must be found into project's directory.")
			endif()

		endif(UNIX)
	endif()
endif()

# set always OCULUS_SDK_ROOT_DIR as OCULUS_HOME
set(OCULUS_SDK_ROOT_DIR ${OCULUS_HOME})

# force OGRE_SDK_ROOT_DIR to show OGRE_HOME value
#set(OGRE_SDK_ROOT_DIR "${OGRE_HOME}" CACHE PATH "Location of your Ogre3D libraries" FORCE)
	

################################################################
#                   OCULUSSDK SETUP 						   #
################################################################

# check if there is a bundled version of OculusSDK in this project lib folder
# NOTE:	default is giving priority to the bundled version, this is because it can be specifically tweaked for this project.
#		If you don't like it, disable it by setting USE_BUNDLED_OculusSDK to FALSE
if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/OculusSDK")
	message("Bundled OculusSDK found in '/${libs_dir_name}'.")
	set (BUNDLED_OculusSDK_FOUND true)
	# give the option to use it (true by default)
	set (USE_BUNDLED_OculusSDK true CACHE BOOL "Use the version of OculusSDK bundled in this project (within '/${libs_dir_name}' folder)" )
else()
	# message(STATUS "No bundled OculusSDK found in '/${libs_dir_name}'.")
	set (BUNDLED_OculusSDK_FOUND false)
	# don't show any option (not a cache variable)
	set (USE_BUNDLED_OculusSDK false)
endif()

# --------------------------------------------------------------
# - OculusSDK installation does not include any cmake file. findOculusSDK.cmake must be included in local /cmake folder.
# - findOculusSDK.cmake NEEDS you to set OCULUS_SDK_ROOT_DIR
# --------------------------------------------------------------

# if using bundled OculusSDK
if(BUNDLED_OculusSDK_FOUND AND USE_BUNDLED_OculusSDK)
	# set OCULUS_HOME cmake variable as OculusSDK local path
	if(DEFINED ENV{OCULUS_HOME} OR DEFINED OCULUS_HOME)
		message("Overriding 'ENV{OCULUS_HOME}'. Uncheck 'USE_BUNDLED_OculusSDK' flag if you wish to use your OCULUS_HOME instead.")
	endif()
	set(OCULUS_HOME "${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/OculusSDK")
	# find out if source code of bundled OculusSDK is also available
	if( EXISTS "${OCULUS_HOME}/LibOVR/Src/OVR_CAPI.cpp")
		set (OculusSDK_SOURCE_FOUND true)
		# give the option to compile it toghether with the project
		set (COMPILE_OculusSDK false CACHE BOOL "Compile the source of OculusSDK toghether with the project" )
	else()
		# hide the option to compile
		set (OculusSDK_SOURCE_FOUND false)
	endif()
# else, USE STANDARD SEARCH OculusSDK
else()
	# hide any option for compiling
	unset(COMPILE_OculusSDK CACHE)
	set (COMPILE_OculusSDK false)
	# if OCULUS_HOME is NOT defined (environment or cmake variable)
	if (NOT DEFINED ENV{OCULUS_HOME} AND NOT DEFINED OCULUS_HOME)
		# throw ERROR
		message(SEND_ERROR "Can't find OculusSDK. Must define OCULUS_HOME (as Environment variable or cmake variable), or put 'OculusSDK' folder into '${libs_dir_name}' directory. You can also set it manually now and run Configure again.")
		# give the option to setup OCULUS_HOME manually
		set(OCULUS_HOME "${OCULUS_HOME}" CACHE PATH "Location of your OculusSDK libraries")
	# if one of them is defined, use it	
	else()
		if (NOT DEFINED OCULUS_HOME)
			message(STATUS "Environment variable OCULUS_HOME found. This will be used as OculusSDK folder.")
			# assign OCULUS_HOME the value of environment variable
			set(OCULUS_HOME $ENV{OCULUS_HOME})
		endif()
		message("Using OculusSDK from '${OCULUS_HOME}'. Check 'USE_BUNDLED_OculusSDK' flag if you wish to use the version bundled in the project.")
	endif()
endif()

# add OculusSDK cmake directory to .cmake search path
set(CMAKE_MODULE_PATH "${OCULUS_HOME}/CMake/;${CMAKE_MODULE_PATH}")				# append: OCULUSSDK personal cmake directory

# set always OCULUS_SDK_ROOT_DIR as OCULUS_HOME
set(OCULUS_SDK_ROOT_DIR ${OCULUS_HOME})

# force OCULUS_SDK_ROOT_DIR to show OCULUS_HOME value
#set(OCULUS_SDK_ROOT_DIR "${OCULUS_HOME}" CACHE PATH "Location of your OculusSDK libraries" FORCE)
	

################################################################
#                   EXTERNAL DEPENDENCIES                      #
################################################################

# OIS + OGRE lib
find_package(OIS REQUIRED)							# OIS lib config
find_package(OGRE REQUIRED)							# OGRE lib config
list(APPEND LIBRARIES_TO_LINK							# OGRE + OIS lib link
	${OGRE_LIBRARIES}								
	${OGRE_Overlay_LIBRARIES}						
	${OIS_LIBRARIES}								
	)
list(APPEND HEADERS_TO_INCLUDE_PUBLIC					# OGRE + OIS lib include
	"${OIS_INCLUDE_DIRS}"
	"${OGRE_INCLUDE_DIRS}"
	"${OGRE_Overlay_INCLUDE_DIRS}"
	"${OGRE_SAMPLES_INCLUDEPATH}"
	)

# OculusSDK lib (if bundled version cannot be linked, compile from source)
unset(OCULUSSDK_FOUND CACHE)
set(OCULUSSDK_FOUND false)
if(USE_BUNDLED_OculusSDK)
	find_package(OculusSDK QUIET)					# OculusSDK lib config (if bundled)
	#message(${OCULUSSDK_FOUND})
	if(NOT OCULUSSDK_FOUND)
		if(OculusSDK_SOURCE_FOUND)
			message(STATUS "Failed to find bundled OculusSDK precompiled libraries. Source code available. Will force to compile it.")
			set (COMPILE_OculusSDK true CACHE BOOL "[FORCED] Compile the source of OculusSDK toghether with the project" FORCE)
		else()
			message(SEND_ERROR "Failed to find valid libraries/source in bundled OculusSDK folder. Uncheck the 'USE_BUNDLED_OculusSDK' flag and retry.")
		endif()
	endif()	
else()
	find_package(OculusSDK REQUIRED)				# OculusSDK lib config (if not bundled)
endif()

if(NOT COMPILE_OculusSDK)					
	list(APPEND LIBRARIES_TO_LINK ${OCULUS_SDK_LIBRARIES})		# OculusSDK lib link
	list(APPEND HEADERS_TO_INCLUDE_PUBLIC "${OCULUS_SDK_INCLUDE_DIRS}")# OculusSDK lib include
endif()


# OPENCV lib
find_package(OpenCV REQUIRED)						# OpenCV lib config
list(APPEND LIBRARIES_TO_LINK ${OpenCV_LIBS})		# OpenCV lib link
list(APPEND HEADERS_TO_INCLUDE_PUBLIC				# OpenCV lib include
	"${OpenCV_INCLUDE_DIRS}"
	)

################################################################
#                   ADD MAIN PROJECT FILES 					   #
################################################################

# main directories
set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${source_dir_name})
set (PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${header_dir_name})

# add files and group them
file(GLOB source_files "${source_dir_name}/*.cpp")
if(NOT IS_SUBPROJECT)
	list(APPEND source_files "${source_dir_name}/demo/main.cpp")	# main file to run the demo
endif()
source_group("${source_dir_name}" FILES ${source_files})
file(GLOB_RECURSE header_files "${header_dir_name}/*.hpp" "${header_dir_name}/*.h")
source_group("${header_dir_name}" FILES ${header_files})

# add header files to included directories
list(APPEND HEADERS_TO_INCLUDE_PUBLIC "${PROJECT_INCLUDE_DIR}")


################################################################
#                   ADD SUBPROJECTS (RECURSIVE)				   #
################################################################

# OculusSDK lib - add as SUBPROJECT if user wants to compile it from bundled version
if(USE_BUNDLED_OculusSDK AND COMPILE_OculusSDK)
	add_subdirectory(${OCULUS_HOME})					# OculusSDK subproject source
	#set_target_properties(ovr PROPERTIES FOLDER "3rdParty")
	list(APPEND LIBRARIES_TO_LINK ovr)					# OculusSDK subproject lib link
	list(APPEND HEADERS_TO_INCLUDE_PUBLIC				# OculusSDK subproject include
		"${OCULUS_HOME}/LibOVR/Include"
		"${OCULUS_HOME}/LibOVRKernel/Src"
	)
endif()

# arUco lib - always compiled with the project
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/arUco")	# arUco subproject src and config
list(APPEND LIBRARIES_TO_LINK aruco)									# arUco subproject lib link
list(APPEND HEADERS_TO_INCLUDE_PUBLIC									# arUco subproject include
		"${CMAKE_CURRENT_SOURCE_DIR}/${libs_dir_name}/arUco/src"
	)
if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")					# flags needed by arUco in linux
endif()


################################################################
#                   EXE OUTPUT                                 #
################################################################
if(IS_SUBPROJECT)
	add_library(${OUTPUT_FILE_NAME} STATIC
		${source_files}
		${header_files}
	)
else()
	ADD_EXECUTABLE("${OUTPUT_FILE_NAME}" ${source_files} ${header_files})
endif()


################################################################
#                   INCLUDING HEADERS                          #
################################################################

if(DEFINED HEADERS_TO_INCLUDE_PUBLIC)
	target_include_directories(${OUTPUT_FILE_NAME} PUBLIC ${HEADERS_TO_INCLUDE_PUBLIC})		# these headers will be accessible from parent projects and can be
endif()																							# included automatically by including:
																								# $<TARGET_PROPERTY:OgreOculusApp,INTERFACE_INCLUDE_DIRECTORIES>
																								# as PRIVATE or PUBLIC to your target.

if(DEFINED HEADERS_TO_INCLUDE_PRIVATE)
	target_include_directories(${OUTPUT_FILE_NAME} PRIVATE ${HEADERS_TO_INCLUDE_PRIVATE})		# these headers cannot be accessed from parent projects, unless
endif()																							# you include them manually by hardcoding their path.
																								# By the way, these are only the headers that are not meant to be
																								# used by parent project (hidden dependencies)

################################################################
#                   LINKING LIBRARIES                          #
################################################################

# All platforms
if(DEFINED LIBRARIES_TO_LINK)
	target_link_libraries("${OUTPUT_FILE_NAME}"
		${LIBRARIES_TO_LINK}
		#${Boost_LIBRARY_DIRS}							# Boost libraries (if your Ogre needs it)
	)
endif()			

# Win32 specific
if( WIN32 )
	target_link_libraries("${OUTPUT_FILE_NAME}"
		winmm.lib
		ws2_32.lib
	)
	
# Unix specific
elseif( UNIX )

	# Where to find UDEV:
	# find_package(UDEV REQUIRED)
	# Where to find XINERAMA:
	find_package(Xinerama REQUIRED)
	find_package(X11 REQUIRED)

	target_link_libraries("${OUTPUT_FILE_NAME}"
		librt.so
		libXrandr.so
		libGL.so
		libXxf86vm.so
		libpthread.so
		${XINERAMA_LIBRARY}
		${X11_LIBRARIES}
	)
endif()



################################################################
#           TARGET PROPRIETIES AND COMPILER OPTIONS            #
################################################################

# set "_d" postfix for debug configurations
set_target_properties(${OUTPUT_FILE_NAME} PROPERTIES DEBUG_POSTFIX _d)

# set correct c++ flag according to compiler in use (no longer necessary in cmake 3.0!)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# further specific flags
if( WIN32 )
	# /MP for multiple thread compiling
	# /DWIN32_LEAN_AND_MEAN to avoid conflicts between boost and Leap motion, which both seem to include winsock.h
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /DWIN32_LEAN_AND_MEAN")
endif()


################################################################
#                   OGRE RUNTIME FILES CONFIG                  #
################################################################
message(STATUS "Ogre Debug plugin dir found: ${OGRE_PLUGIN_DIR_DBG}")
message(STATUS "Ogre Release plugin dir found: ${OGRE_PLUGIN_DIR_REL}")

# If is a subproject and a needed configuration/media file is missing from top-level directory, make a copy from samples
# -------------------------
if(IS_SUBPROJECT)
	# config files
	# note: only .cfg files from config directory (and no subdirectories) are compared and copied
	file(GLOB config_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${sample_config_dir_name}" "${CMAKE_CURRENT_SOURCE_DIR}/${sample_config_dir_name}/*.cfg")
	foreach(config_file ${config_files})
		if(NOT EXISTS ${PROJECT_CONFIG_DIR}/${config_file})
			message(STATUS "No ${config_file} found in '${PROJECT_CONFIG_DIR}'. Copying default file from '${CMAKE_CURRENT_SOURCE_DIR}/${config_dir_name}' ..")
			file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${config_dir_name}/${config_file}"
				DESTINATION "${PROJECT_CONFIG_DIR}"
			)
		endif()
	endforeach()
	# media files
	# note: any content found in any subfolder will be copied to the main folder PRESERVING directory structure
	# (this means that also media used for demo/sample will be copied for simplicity)
	file(GLOB_RECURSE media_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${sample_media_dir_name}" "${CMAKE_CURRENT_SOURCE_DIR}/${sample_media_dir_name}/*")
	foreach(media_file ${media_files})
		string(REGEX MATCH "(.*/?)([^/]+)$" media_file_relative_path ${media_file})
		set(media_file_relative_path ${CMAKE_MATCH_1})		# if file is in main folder, this is an empty string
		set(media_file_name ${CMAKE_MATCH_2}) 				# this will only contain filename.extension
		if(NOT EXISTS ${PROJECT_MEDIA_DIR}/${media_file})
			message(STATUS "No ${CMAKE_MATCH_2} found in '${PROJECT_MEDIA_DIR}'. Copying file from '${CMAKE_CURRENT_SOURCE_DIR}/${media_dir_name}' ..")
			file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${media_dir_name}/${media_file}"
				DESTINATION "${PROJECT_MEDIA_DIR}/${media_file_relative_path}"
			)
		endif()
	endforeach()	
endif()

# Set path to Ogre plugins in plugins.cfg/plugins_d.cfg according to your system:
# -------------------------
file(READ ${PROJECT_CONFIG_DIR}/plugins.cfg PLUGINS_FILE)
file(READ ${PROJECT_CONFIG_DIR}/plugins_d.cfg PLUGINS_D_FILE)
string(REGEX REPLACE "PluginFolder=[^\n]*"
	"PluginFolder=${OGRE_PLUGIN_DIR_REL}" PLUGINS_FILE_MODIFIED
	${PLUGINS_FILE})
string(REGEX REPLACE "PluginFolder=[^\n]*"
	"PluginFolder=${OGRE_PLUGIN_DIR_DBG}" PLUGINS_D_FILE_MODIFIED
	${PLUGINS_D_FILE})
file(WRITE ${PROJECT_CONFIG_DIR}/plugins.cfg ${PLUGINS_FILE_MODIFIED})
file(WRITE ${PROJECT_CONFIG_DIR}/plugins_d.cfg ${PLUGINS_D_FILE_MODIFIED})
message(STATUS "plugins_d.cfg and plugins.cfg updated according to your setup.")

# Set absolute paths to Ogre media in resources.cfg according to your system:
# ---------------------------
# http://stackoverflow.com/questions/30226775/excluding-lines-containing-a-specific-string-from-regex-replace-in-cmake
string(REGEX REPLACE	# regex does not like escapes :)
	"\\\\"				# for regex this means "\"
	"/"
	OGRE_HOME_BACKSLASHES ${OGRE_HOME})
file(READ ${PROJECT_CONFIG_DIR}/resources.cfg RESOURCES_FILE)
# Set the variable "Esc" to some ASCII value - basically something
# which is unlikely to conflict with anything in the file contents.
string(ASCII 27 Esc)
# Turn the contents into a list of strings, each ending with an Esc.
# This allows us to preserve blank lines in the file since CMake
# automatically prunes empty list items during a foreach loop.
string(REGEX REPLACE "\n" "${Esc};" ContentsAsList "${RESOURCES_FILE}")
unset(RESOURCES_FILE_MODIFIED)	# empty the variable if previously used..
foreach(Line ${ContentsAsList})
  # Don't modify the line if resource path starts with one ore more "."
  if(NOT "${Line}" MATCHES "^FileSystem=[.]+")
  	# WE MAKE AN ASSUMPTION: Ogre global media files were in a path that contained the word "Media"
  	# so we assume that any modifications made to this file by you contain this word.
  	# But.. this MAY CHANGE in the future! So, to make it stronger, we try with "media", "Media" and "MEDIA".
  	# Paths not containing these words remain UNTOUCHED
    string(REGEX REPLACE "=.*/media" "=${OGRE_MEDIA_DIR}" Line ${Line})	# media
    string(REGEX REPLACE "=.*/Media" "=${OGRE_MEDIA_DIR}" Line ${Line})	# Media (most likely)
    string(REGEX REPLACE "=.*/MEDIA" "=${OGRE_MEDIA_DIR}" Line ${Line})	# MEDIA
    # now all those paths REFERENCE CORRECTLY the Ogre media folder installed in your system!																			
  endif()
  # Swap the appended Esc character back out in favour of a line feed
  string(REGEX REPLACE "${Esc}" "\n" Line ${Line})
  # Append modified line to output file
  set(RESOURCES_FILE_MODIFIED "${RESOURCES_FILE_MODIFIED}${Line}")
endforeach()
file(WRITE ${PROJECT_CONFIG_DIR}/resources.cfg ${RESOURCES_FILE_MODIFIED})
message(STATUS "resources.cfg updated according to your setup.")

# ok.. but now what is the name of YOUR Ogre media folder?
# better if we save it: we must use it to identify the absolute part of the paths
# and strip it when INSTALL is performed (and convert the path from absolute to relative)
# see PostInstall.cmake file for details on how I do it :)
string(REGEX MATCH "([^.]+)/([^/]+)$" OGRE_MEDIA_FOLDER_NAME ${OGRE_MEDIA_DIR})
set(OGRE_MEDIA_FOLDER_NAME ${CMAKE_MATCH_2})
# now OGRE_MEDIA_FOLDER_NAME contains the word extracted after the last "/" found



################################################################
#                   POST-BUILD AND INSTALL 					   #
################################################################
# Run "INSTALL" to copy exe, dll/so, media and cfg files into /dist folder

# define "dist" directory where finished project should be installed for final distribution
#if(WIN32 AND NOT MINGW)
#	add_custom_command( TARGET ${OUTPUT_FILE_NAME} PRE_BUILD
#		COMMAND if not exist ..\\${distribution_dir_name} mkdir ..\\${distribution_dir_name} )
	#add_custom_command( TARGET ${OUTPUT_FILE_NAME} POST_BUILD
	#	COMMAND copy \"$(TargetPath)\" ..\\${distribution_dir_name} )
#endif(WIN32 AND NOT MINGW)
 
#if(MINGW OR UNIX)
	#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_INSTALL_PREFIX})
#endif(MINGW OR UNIX)

# 
#install (SCRIPT "${CMAKE_SOURCE_DIR}/cmake/PreInstall.cmake")

# copy release executable
if(NOT IS_SUBPROJECT)
	install(TARGETS ${OUTPUT_FILE_NAME}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
		CONFIGURATIONS Release RelWithDebInfo
	)
endif()

# copy LOCAL /media folder AS IS into /dist (IF ANY!)
if(EXISTS "${PROJECT_MEDIA_DIR}" AND IS_DIRECTORY "${PROJECT_MEDIA_DIR}")
	install(DIRECTORY ${PROJECT_MEDIA_DIR}
		DESTINATION ${CMAKE_INSTALL_PREFIX}
		CONFIGURATIONS Release RelWithDebInfo Debug
	)
endif()

# [OGRE] copy needed cfg files from /cfg (MUST BE DONE BEFORE RUNNING PostInstall SCRIPT)
file(GLOB config_files "${CMAKE_SOURCE_DIR}/${sample_config_dir_name}/*.cfg")
if(DEFINED config_files)
	install(FILES ${config_files}
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${config_dir_name}
		CONFIGURATIONS Release RelWithDebInfo Debug
	)
endif()

# [OGRE] copy all MANDATORY DLLs for OGRE (to the same path of the executable)
# NOTE: from the 1.7.1 sdk the OIS dll is called OIS.dll instead of libOIS.dll
# so you'll have to change that to make it work with 1.7.1
# DON'T INCLUDE HERE ANY PLUGIN DEFINED IN PLUGINS.CFG !!
install(FILES
	${OGRE_PLUGIN_DIR_REL}/OgreMain.${dl}
	${OGRE_PLUGIN_DIR_REL}/OIS.${dl}
	#${OGRE_PLUGIN_DIR_REL}/libOIS.${dl}
	${OGRE_PLUGIN_DIR_REL}/cg.${dl}
	${OGRE_PLUGIN_DIR_REL}/OgreOverlay.${dl}
	DESTINATION ${CMAKE_INSTALL_PREFIX}
	CONFIGURATIONS Release RelWithDebInfo Debug
)

# Boost management: NOT TESTED - feel free to improve it!
if(Boost_FOUND AND OGRE_USE_BOOST)

	if(WIN32)
	 
		# NOTE: as of sdk 1.7.2 we need to copy the boost dll's as well
		# because they're not linked statically (it worked with 1.7.1 though)
		install(FILES ${Boost_DATE_TIME_LIBRARY_RELEASE}
			${Boost_THREAD_LIBRARY_RELEASE}
			DESTINATION ${CMAKE_INSTALL_PREFIX}
			CONFIGURATIONS Release RelWithDebInfo
		)
		install(FILES ${Boost_DATE_TIME_LIBRARY_DEBUG}
			${Boost_THREAD_LIBRARY_DEBUG}
			DESTINATION ${CMAKE_INSTALL_PREFIX}
			CONFIGURATIONS Debug
		)	
	 
	endif(WIN32)

	# Copy the Boost libraries:
	get_filename_component( DIR ${Boost_SYSTEM_LIBRARY_DEBUG} DIRECTORY )
	get_filename_component( S ${Boost_SYSTEM_LIBRARY_DEBUG} NAME_WE )

	add_custom_command(TARGET OculusOgreSample POST_BUILD        # Adds a post-build event to MyTest
		COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
			"${Boost_SYSTEM_LIBRARY_DEBUG}"      # <--this is in-file
			$<TARGET_FILE_DIR:OculusOgreSample>)                 # <--this is out-file path               # <--this is out-file path
			
	add_custom_command(TARGET OculusOgreSample POST_BUILD        # Adds a post-build event to MyTest
		COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
			"${DIR}/${S}.${dl}"      # <--this is in-file
			$<TARGET_FILE_DIR:OculusOgreSample>)                 # <--this is out-file path

	get_filename_component( DIR ${Boost_SYSTEM_LIBRARY_RELEASE} DIRECTORY )
	get_filename_component( S ${Boost_SYSTEM_LIBRARY_RELEASE} NAME_WE )

	add_custom_command(TARGET OculusOgreSample POST_BUILD        # Adds a post-build event to MyTest
		COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
			"${Boost_SYSTEM_LIBRARY_RELEASE}"      # <--this is in-file
			$<TARGET_FILE_DIR:OculusOgreSample>)                 # <--this is out-file path
			
	add_custom_command(TARGET OculusOgreSample POST_BUILD        # Adds a post-build event to MyTest
		COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
			"${DIR}/${S}.${dl}"      # <--this is in-file
			$<TARGET_FILE_DIR:OculusOgreSample>)                 # <--this is out-file path

endif()

# [OGRE] Generate an install script to be run at install time to:
#	1 - copy only NEEDED PLUGIN DLLs (parsing plugins.cfg) such as RenderSystems to /plugins
# 	2 - set path to Ogre plugins in plugins.cfg to RELATIVE
#	3 - copy only needed media (parsing resources.cfg) from Ogre media directory to /media
#	4 - set paths to Ogre media in resources.cfg to RELATIVE
configure_file(${CMAKE_INSTALL_SCRIPT} ${CMAKE_INSTALL_SCRIPT_SPECIFIC} @ONLY)	# this command allows to mirror variables from this file to another
																				# @ONLY means only @VARS@ (and not ${VARS}) will be replaced in destination file!
install (SCRIPT ${CMAKE_INSTALL_SCRIPT_SPECIFIC})

