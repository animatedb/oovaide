# Generated by oovCMaker
add_executable(oovBuilder BuildConfigWriter.cpp ComponentBuilder.cpp ComponentFinder.cpp 
  Coverage.cpp ObjSymbols.cpp oovBuilder.cpp srcFileParser.cpp)

target_link_libraries(oovBuilder oovCommon)

install(TARGETS oovBuilder
  EXPORT OovaideTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT lib)
