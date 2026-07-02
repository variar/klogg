add_library(
  klogg_regex STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/hsregularexpression.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/regularexpression.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/booleanevaluator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/regularexpressionpattern.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/regularexpression.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hsregularexpression.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/booleanevaluator.h
)
target_include_directories(klogg_regex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
  klogg_regex
  PUBLIC project_options
         project_warnings
         klogg_logging
         klogg_settings
         klogg_utils
         Qt${QT_VERSION_MAJOR}::Core
         robin_hood
         exprtk
)

if(KLOGG_USE_HYPERSCAN)
  target_link_libraries(klogg_regex PUBLIC klogg_hyperscan)
  target_compile_definitions(klogg_regex PUBLIC KLOGG_HAS_HS)
elseif(KLOGG_USE_VECTORSCAN)
  target_link_libraries(klogg_regex PUBLIC klogg_vectorscan)
  target_compile_definitions(klogg_regex PUBLIC KLOGG_HAS_HS)
endif()

if(KLOGG_USE_LTO)
  set_property(TARGET klogg_regex PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
