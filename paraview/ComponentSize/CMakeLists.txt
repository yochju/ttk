cmake_minimum_required(VERSION 2.4)

project(ComponentSize)

option(withParaView "Enable ParaView support" true)
set(withParaView true)

# path to the common code base
find_path(TTK_DIR ttk.cmake
  PATHS
    core/
    ../core/
    ../../core/
    ../../../core/
)
include(${TTK_DIR}/ttk.cmake)

# include the vtk wrapper for the scalar field smoothing package
# HERE: change to whatever wrapper you want to use
ttk_add_vtkWrapper_package(vtkComponentSize)

# Add the plugin
ttk_wrapup_paraview_plugin(
  ComponentSize
  1.0)