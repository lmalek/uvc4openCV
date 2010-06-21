cmake_minimum_required (VERSION 2.6) 
PROJECT( uvc4openCV )

FIND_PACKAGE(OpenCV REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${OPENCV_INCLUDE_DIR} inc)

# In addition to specifying the libraries to link, the directories containing the
# libraries should be added in case the libraries are in a non-standard location
LINK_DIRECTORIES(${OPENCV_LINK_DIRECTORIES})
LINK_LIBRARIES(${OPENCV_LIBRARIES})

ADD_EXECUTABLE(uvc4openCV src/main.cpp src/color.cpp  src/utils.cpp  src/v4l2uvc.cpp)
TARGET_LINK_LIBRARIES(uvc4openCV ${OPENCV_LIBRARIES} )
