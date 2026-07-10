add_library(FileFlattenerCore STATIC
    src/FileScanner.cpp
    src/Deduplicator.cpp
    src/Renamer.cpp
    src/ZipWriter.cpp
    src/StructureExporter.cpp
    src/FilterEngine.cpp
    src/GraphModel.cpp
    src/ProjectDetector.cpp
    src/CodeLexer.cpp
    src/CallGraphAnalyzer.cpp
)

set_target_properties(FileFlattenerCore PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

target_include_directories(FileFlattenerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(ZLIB REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_path(PICOSHA2_INCLUDE_DIRS "picosha2.h")

target_include_directories(FileFlattenerCore PRIVATE ${PICOSHA2_INCLUDE_DIRS})

target_link_libraries(FileFlattenerCore PUBLIC
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:MINIZIP::minizip>,MINIZIP::minizip,MINIZIP::minizipstatic>
)


