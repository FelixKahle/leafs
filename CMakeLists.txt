# Copyright 2023 Felix Kahle.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.15)

project(FKLeafs)

set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(third_party/abseil-cpp)

set(FKLEAFS_HEADER_FILES
	${CMAKE_CURRENT_LIST_DIR}/include/leafs.h
	${CMAKE_CURRENT_LIST_DIR}/include/module_info.h
	${CMAKE_CURRENT_LIST_DIR}/include/module_interface.h
	${CMAKE_CURRENT_LIST_DIR}/include/module_manager.h)

set(FKLEAFS_SOURCE_FILES
	${CMAKE_CURRENT_LIST_DIR}/src/module_manager.cpp)

add_library(${PROJECT_NAME} STATIC
	${FKLEAFS_HEADER_FILES}
	${FKLEAFS_SOURCE_FILES}
)
target_link_libraries(${PROJECT_NAME} PUBLIC
	absl::base
	absl::log
	absl::log_severity 
	absl::synchronization
	absl::flat_hash_map)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(SimpleExample ${CMAKE_CURRENT_LIST_DIR}/examples/simple_example.cpp)
target_link_libraries(SimpleExample PRIVATE ${PROJECT_NAME})