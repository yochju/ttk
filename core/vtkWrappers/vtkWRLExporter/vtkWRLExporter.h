/// \ingroup vtkWrappers
/// \class vtkWRLExporter 
/// \author Julien Tierny <julien.tierny@lip6.fr>
/// \date November 2014.
///
/// \brief TTK helper that fixes a few bugs in the texture support for the VRML 
/// export.

#ifndef _VTK_WRL_EXPORTER_H
#define _VTK_WRL_EXPORTER_H

// base code includes
#include                  <Debug.h>

// VTK includes
#include                  <vtkActor.h>
#include                  <vtkActorCollection.h>
#include                  <vtkAssemblyPath.h>
#include                  <vtkCamera.h>
#include                  <vtkCellArray.h>
#include                  <vtkCompositeDataGeometryFilter.h>
#include                  <vtkDataArray.h>
#include                  <vtkDataObject.h>
#include                  <vtkGeometryFilter.h>
#include                  <vtkLightCollection.h>
#include                  <vtkMapper.h>
#include                  <vtkMath.h>
#include                  <vtkObjectFactory.h>
#include                  <vtkPointData.h>
#include                  <vtkPoints.h>
#include                  <vtkPolyDataMapper.h>
#include                  <vtkRenderer.h>
#include                  <vtkRendererCollection.h>
#include                  <vtkRenderWindow.h>
#include                  <vtkSmartPointer.h>
#include                  <vtkTransform.h>
#include                  <vtkUnsignedCharArray.h>
#include                  <vtkVRMLExporter.h>

extern vtkPolyData *wrlExporterPolyData_;

#endif // _VTK_WRL_EXPORTER_H
