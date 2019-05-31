CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#CMAKE_POLICY(SET CMP0028 OLD)

PROJECT(algo)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
      include/
        prototype/
      )
set ( CMAKE_PREFIX_PATH "/opt/Qt5.8/5.8/gcc_64/lib/cmake/")
find_package( Qt5Core REQUIRED )

SET(CMAKE_CXX_FLAGS "-std=c++0x")

SET ( COMPONENTS
      include
      prototype
      interface
      routing
      common
      )

FOREACH(COMPONENT ${COMPONENTS})
   ADD_SUBDIRECTORY(${COMPONENT})
ENDFOREACH(COMPONENT)

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
qt5_use_modules(${PROJECT_NAME} Core)
TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${COMPONENTS})

