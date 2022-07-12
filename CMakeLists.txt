cmake_minimum_required (VERSION 3.0)#要求cmake最低的版本号
project(mprpc) # 定义当前工程名字

#生成debug版本，可以进行调试
set(CMAKE_BUILD_TYPE "Debug")

#设置项目可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/example)
#设置项目库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

#进入子目录下执行 CMakeLists.txt文件 

#src包含了mprpc框架所有的相关代码
add_subdirectory(src) 
#example包含了mprpc框架使用的示例代码
add_subdirectory(example)