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

SET(PRODUCT_NAME libggadget)

PROJECT(google_gadgets_for_linux)

SET(GGL_MAJOR_VERSION 0)
SET(GGL_MINOR_VERSION 0)
SET(GGL_MICRO_VERSION 1)
SET(GGL_VERSION ${GGL_MAJOR_VERSION}.${GGL_MINOR_VERSION}.${GGL_MICRO_VERSION})

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)
  INCLUDE(Dart)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)

INCLUDE(CopyUtils)
INCLUDE(GenerateOutput)
INCLUDE(PkgConfigEx)
INCLUDE(TestSuite)
INCLUDE(ZipUtils)
INCLUDE(SysDeps)

ADD_DEFINITIONS(
  -DUNIX
  -DPREFIX=${CMAKE_INSTALL_PREFIX}
  -DPRODUCT_NAME=${PRODUCT_NAME}
  # For stdint.h macros like INT64_C etc.
  -D__STDC_CONSTANT_MACROS
  # TODO: only for Linux by now
  -DGGL_HOST_LINUX
  -DGGL_USE_X11
  -DGGL_MODULE_DIR=\\\"../modules\\\"
  -DGGL_RESOURCE_DIR=\\\".\\\")

IF(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Werror -Wconversion")
  # No "-Wall -Werror" for C flags, to avoid third_party code break.
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET(PROJECT_RESOURCE_DIR share/${PRODUCT_NAME})
ELSE(UNIX)
  SET(PROJECT_RESOURCE_DIR resource)
ENDIF(UNIX)
# SET(CMAKE_SKIP_RPATH ON)
ENABLE_TESTING()

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-D_DEBUG)
ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

SET(GGL_BUILD_SIMPLE_HOST TRUE)
SET(GGL_BUILD_QT_HOST TRUE)
SET(GGL_BUILD_LIBGGADGET_DBUS TRUE)
SET(GGL_BUILD_LIBGGADGET_GTK TRUE)
SET(GGL_BUILD_LIBGGADGET_QT TRUE)
SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT TRUE)
SET(GGL_BUILD_GST_AUDIO_FRAMEWORK TRUE)
SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME TRUE)
SET(GGL_BUILD_CURL_XML_HTTP_REQUEST TRUE)
SET(GGL_BUILD_LIBXML2_XML_PARSER TRUE)
SET(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK TRUE)

# Check necessary libraries.

FIND_PACKAGE(ZLIB REQUIRED)

GET_CONFIG(libxml-2.0 2.6.0 LIBXML2 LIBXML2_FOUND)
IF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)
  MESSAGE("Library libxml2 is not available, libxml2-xml-parser extension won't be built.")
  SET(GGL_BUILD_LIBXML2_XML_PARSER FALSE)
ENDIF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)

GET_CONFIG(libcurl 7.16.2 LIBCURL LIBCURL_FOUND)
IF(NOT LIBCURL_FOUND AND GGL_BUILD_CURL_XML_HTTP_REQUEST)
  MESSAGE("Library curl is not available, curl-xml-http-request extension won't be built.")
  SET(GGL_BUILD_CURL_XML_HTTP_REQUEST FALSE)
ENDIF(NOT LIBCURL_FOUND AND GGL_BUILD_CURL_XML_HTTP_REQUEST)

GET_CONFIG(gstreamer-0.10 0.10.0 GSTREAMER GSTREAMER_FOUND)
IF(NOT GSTREAMER_FOUND AND GGL_BUILD_GST_AUDIO_FRAMEWORK)
  MESSAGE("Library gstreamer is not available, gst-audio-framework extension won't be built.")
  SET(GGL_BUILD_GST_AUDIO_FRAMEWORK FALSE)
ENDIF(NOT GSTREAMER_FOUND AND GGL_BUILD_GST_AUDIO_FRAMEWORK)

GET_CONFIG(pango 1.10.0 PANGO PANGO_FOUND)
GET_CONFIG(cairo 1.0.0 CAIRO CAIRO_FOUND)
GET_CONFIG(gdk-2.0 2.8.0 GDK2 GDK2_FOUND)
GET_CONFIG(gtk+-2.0 2.8.0 GTK2 GTK2_FOUND)
IF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND)
  SET(GGL_BUILD_LIBGGADGET_GTK FALSE)
  SET(GGL_BUILD_SIMPLE_HOST FALSE)
  MESSAGE("Library cairo, pando or gtk-2.0 are not available, gtk-system-framework extension, libggadget-gtk, gtk-and simple host won't be built.")
ENDIF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND)

GET_CONFIG(librsvg-2.0 2.14 RSVG RSVG_FOUND)

SET(QT_MIN_VERSION 4.3.0)
FIND_PACKAGE(Qt4)
IF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  INCLUDE(${QT_USE_FILE})
  SET(QT_LINK_DIR ${QT_LIBRARY_DIR})
  SET(QT_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
ELSE(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  SET(GGL_BUILD_LIBGGADGET_QT FALSE)
  SET(GGL_BUILD_QT_HOST FALSE)
  MESSAGE("Library qt-4.3 or above is not available, libggadget-qt, qt host and qt related extensions won't be built.")
ENDIF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)

GET_CONFIG(dbus-1 1.0 DBUS DBUS_FOUND)
IF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)
  SET(GGL_BUILD_LIBGGADGET_DBUS FALSE)
  MESSAGE("Library D-Bus is not available, libggadget-dbus won't be built.")
ENDIF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)

INCLUDE(SpiderMonkey)
IF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)
  SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME FALSE)
  MESSAGE("Library SpiderMonkey is not available, smjs-script-runtime extension won't be built.")
ENDIF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)

IF(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)
  GET_CONFIG(xulrunner-gtkmozembed 1.8 GTKMOZEMBED GTKMOZEMBED_FOUND)
  IF(NOT GTKMOZEMBED_FOUND)
    GET_CONFIG(firefox2-gtkmozembed 2.0 GTKMOZEMBED GTKMOZEMBED_FOUND)
    IF(NOT GTKMOZEMBED_FOUND)
      GET_CONFIG(firefox-gtkmozembed 1.5 GTKMOZEMBED GTKMOZEMBED_FOUND)
    ENDIF(NOT GTKMOZEMBED_FOUND)
  ENDIF(NOT GTKMOZEMBED_FOUND)
  IF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
    SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT FALSE)
    MESSAGE("Library GtkMozEmbed is not available, gtkmoz-browser-element extension won't be built.")
  ENDIF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
ELSE(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)
  SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT FALSE)
  MESSAGE("Library gtk-2.0 or SpiderMonkey is not available, gtkmoz-browser-element extension won't be built.")
ENDIF(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)

MESSAGE("
Build options:
  Version                       ${GGL_VERSION}
  Build type                    ${CMAKE_BUILD_TYPE}

 Libraries:
  GTK SVG Support               ${RSVG_FOUND}
  Build libggadget-gtk          ${GGL_BUILD_LIBGGADGET_GTK}
  Build libggadget-qt           ${GGL_BUILD_LIBGGADGET_QT}
  Build libggadget-dbus         ${GGL_BUILD_LIBGGADGET_DBUS}

 Extensions:
  Build dbus-script-class       ${GGL_BUILD_LIBGGADGET_DBUS}
  Build gtkmoz-browser-element  ${GGL_BUILD_GTKMOZ_BROWSER_ELEMENT}
  Build gst-audio-framework     ${GGL_BUILD_GST_AUDIO_FRAMEWORK}
  Build linux-system-framework  ${GGL_BUILD_LINUX_SYSTEM_FRAMEWORK}
  Build smjs-script-runtime     ${GGL_BUILD_SMJS_SCRIPT_RUNTIME}
  Build curl-xml-http-request   ${GGL_BUILD_CURL_XML_HTTP_REQUEST}
  Build libxml2-xml-parser      ${GGL_BUILD_LIBXML2_XML_PARSER}
  Build gtk-edit-element        ${GGL_BUILD_LIBGGADGET_GTK}
  Build gtk-system-framework    ${GGL_BUILD_LIBGGADGET_GTK}
  Build qt-edit-element         ${GGL_BUILD_LIBGGADGET_QT}
  Build qt-system-framework     ${GGL_BUILD_LIBGGADGET_QT}

 Hosts:
  Build simple host             ${GGL_BUILD_SIMPLE_HOST}
  Build qt host                 ${GGL_BUILD_QT_HOST}
")
