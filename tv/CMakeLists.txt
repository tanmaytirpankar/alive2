add_llvm_library(tv MODULE tv.cpp)

target_link_libraries(tv PRIVATE ${ALIVE_LIBS_LLVM})

if(APPLE)
    set_target_properties(tv PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)
