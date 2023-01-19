include_directories(include)
include_directories(test_images)
add_executable(libgenpattern
			   "src/convex_area.c"
			   "src/polygon.c"
			   "src/convex_hull.c"
			   "src/doubly_linked_list.c"
			   "src/genpattern.c"
			   "src/convex_distance.c"
			   "src/polygon_translate.c"
			   "src/misc.c"
			   "include/convex_area.h"
			   "include/basic_geometry.h"
			   "include/convex_hull.h"
			   "include/doubly_linked_list.h"
			   "include/genpattern.h"
			   "include/misc.h"
			   "include/convex_distance.h"
			   "include/polygon_translate.h"
			   "test_images/test_image1._h"
 "src/misc.c" "src/convex_intersection_area.c" "include/convex_intersection_area.h" "src/box.c" "include/box.h" "include/polygon.h" "include/fibonacci.h")
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 17)
if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /clang:-Ofast /clang:-flto /O2")
	find_package(pthreads CONFIG REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE PThreads4W::PThreads4W)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PThreads4W_DLL_DIR}/pthreadVC3.dll ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/
		COMMAND_EXPAND_LISTS
	)
else()
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -flto")
	endif()
	find_package( Threads REQUIRED )
	target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

target_link_libraries(${PROJECT_NAME} m)
