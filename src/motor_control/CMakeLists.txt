cmake_minimum_required(VERSION 3.8)
project(motor_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/WheelFineControl.action"
)

add_executable(my_node src/my_node.cpp)

ament_target_dependencies(my_node
  rclcpp
  std_msgs
  rclcpp_action
)

# Link the generated interfaces to my_node
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)
target_link_libraries(my_node "${cpp_typesupport_target}")

target_include_directories(my_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(my_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17

install(TARGETS my_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)

  # Skip linters 
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_uncrustify_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(${PROJECT_NAME}_test test/TESTS.cpp)
  if(TARGET ${PROJECT_NAME}_test)
    target_include_directories(${PROJECT_NAME}_test PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    )
    ament_target_dependencies(${PROJECT_NAME}_test
      rclcpp
      std_msgs
      rclcpp_action
    )
    # Link the generated interfaces to the test
    rosidl_get_typesupport_target(cpp_typesupport_target_test
      ${PROJECT_NAME} rosidl_typesupport_cpp)
    target_link_libraries(${PROJECT_NAME}_test "${cpp_typesupport_target_test}")
  endif()
endif()

ament_package()
