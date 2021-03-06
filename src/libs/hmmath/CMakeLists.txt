#library file
set (HEADERS
	hmcompute.hpp
	spmat.hpp
	densemat.hpp
	piecewise.hpp
	partition01.hpp
	hmgraph.hpp
)

set (SOURCES
	hmcompute.cpp
	spmat.cpp
	densemat.cpp
	partition01.cpp
	hmgraph.cpp
)

source_group ("Header Files" FILES ${HEADERS} ${HEADERS})
source_group ("Source Files" FILES ${SOURCES} ${SOURCES})

USE_CXX11()
add_library(${HMMATH_TARGET} SHARED ${HEADERS} ${SOURCES})

# =========== linkage
target_link_libraries(${HMMATH_TARGET} ${HMPROJECT_TARGET})
target_link_libraries(${HMMATH_TARGET} ${SUITESPARSE_LIBRARIES})

include_directories(${HMPROJECT_INCLUDE})
include_directories(${SUITESPARSE_INCLUDE_DIRS})

install(TARGETS ${HMMATH_TARGET}
	RUNTIME DESTINATION ${LIB_INSTALL_DIR}
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
)
add_subdirectory(tests)
