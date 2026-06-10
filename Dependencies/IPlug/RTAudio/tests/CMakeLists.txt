include_directories(..)
if (WIN32)
    include_directories(../include)
endif (WIN32)

list(GET LIB_TARGETS 0 LIBRTAUDIO)

add_executable(audioprobe audioprobe.cpp)
target_link_libraries(audioprobe ${LIBRTAUDIO} ${LINKLIBS})

add_executable(playsaw playsaw.cpp)
target_link_libraries(playsaw ${LIBRTAUDIO} ${LINKLIBS})

add_executable(playraw playraw.cpp)
target_link_libraries(playraw ${LIBRTAUDIO} ${LINKLIBS})

add_executable(record record.cpp)
target_link_libraries(record ${LIBRTAUDIO} ${LINKLIBS})

add_executable(duplex duplex.cpp)
target_link_libraries(duplex ${LIBRTAUDIO} ${LINKLIBS})

add_executable(apinames apinames.cpp)
target_link_libraries(apinames ${LIBRTAUDIO} ${LINKLIBS})

add_executable(testall testall.cpp)
target_link_libraries(testall ${LIBRTAUDIO} ${LINKLIBS})

add_executable(teststops teststops.cpp)
target_link_libraries(teststops ${LIBRTAUDIO} ${LINKLIBS})

add_test(NAME apinames COMMAND apinames)
