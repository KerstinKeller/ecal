# ========================= eCAL LICENSE =================================
#
# Copyright (C) 2016 - 2024 Continental Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ========================= eCAL LICENSE =================================

project(test_memfile)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

set(memfile_test_src
    src/memfile_test.cpp
    src/memfile_naming_test.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/mtx/ecal_named_mutex.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/shm/ecal_memfile.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/shm/ecal_memfile_db.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/shm/ecal_memfile_naming.cpp
)

if(UNIX)
set(memfile_test_os_src
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/mtx/linux/ecal_named_mutex_impl.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/shm/linux/ecal_memfile_os.cpp
)
endif()

if(WIN32)
set(memfile_test_os_src
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/mtx/win32/ecal_named_mutex_impl.cpp
    ${ECAL_CORE_PROJECT_ROOT}/core/src/io/shm/win32/ecal_memfile_os.cpp
)
endif()

ecal_add_gtest(${PROJECT_NAME} ${memfile_test_src} ${memfile_test_os_src})

target_include_directories(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:eCAL::core,INCLUDE_DIRECTORIES>)

target_link_libraries(${PROJECT_NAME} 
  PRIVATE
    $<$<BOOL:${UNIX}>:dl>
    $<$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>:rt>
    Threads::Threads
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
target_compile_definitions(${PROJECT_NAME} PRIVATE ECAL_CORE_TRANSPORT_SHM)

ecal_install_gtest(${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER tests/cpp/io)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${${PROJECT_NAME}_src}
)
