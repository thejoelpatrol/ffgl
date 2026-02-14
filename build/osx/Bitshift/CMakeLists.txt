add_library(ffgl-plugin-add STATIC)
add_library(ffgl::plugin::add ALIAS ffgl-plugin-add)
target_sources(ffgl-plugin-add PRIVATE Bitshift.h Bitshift.cpp)
target_link_libraries(ffgl-plugin-add PRIVATE ffgl::sdk)

install(
    TARGETS     ffgl-plugin-bitshift
    EXPORT      ffgl-plugin-shift-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
