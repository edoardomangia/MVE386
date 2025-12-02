cmake_minimum_required(VERSION 3.16)
project(LiquidDoseSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})
find_package(assimp REQUIRED)

add_executable(run
    src/main.cc
    src/DetectorConstruction.cc
    src/PhysicsList.cc
    src/PrimaryGeneratorAction.cc
    src/ActionInitialization.cc
    src/RunAction.cc
    src/SteppingAction.cc
    src/DoseVoxelGrid.cc
    src/GenVTI.cc
    src/SceneConfig.cc
)

target_include_directories(run 
    PRIVATE 
        include
)

target_link_libraries(run
    PUBLIC
        ${Geant4_LIBRARIES}
        assimp::assimp
)

target_compile_definitions(run
    PRIVATE
        USE_CADMESH_ASSIMP_READER
)
