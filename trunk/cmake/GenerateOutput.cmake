#
# Copyright 2007 Google Inc.
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

IF(WIN32)
  MACRO(OUTPUT_EXECUTABLE TARGET_NAME)
    OUTPUT_TARGET(${TARGET_NAME} /)
  ENDMACRO(OUTPUT_EXECUTABLE TARGET_NAME)
  MACRO(OUTPUT_LIBRARY TARGET_NAME)
    OUTPUT_TARGET(${TARGET_NAME} /)
  ENDMACRO(OUTPUT_LIBRARY TARGET_NAME)
ELSE(WIN32)
  MACRO(OUTPUT_EXECUTABLE TARGET_NAME)
    OUTPUT_TARGET(${TARGET_NAME} bin ${TARGET_NAME}.bin)
    COPY_FILE(${CMAKE_SOURCE_DIR}/utils/bin_wrapper.sh ${CMAKE_BINARY_DIR}/output/bin ${TARGET_NAME})
  ENDMACRO(OUTPUT_EXECUTABLE TARGET_NAME)
  MACRO(OUTPUT_LIBRARY TARGET_NAME)
    OUTPUT_TARGET(${TARGET_NAME} lib)
  ENDMACRO(OUTPUT_LIBRARY TARGET_NAME)
ENDIF(WIN32)
