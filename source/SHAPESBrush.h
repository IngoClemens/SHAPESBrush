// ---------------------------------------------------------------------
//
//  SHAPESBrush.h
//  SHAPESBrush
//
//  Created by ingo on 3/28/14.
//  Copyright (c) 2021 Ingo Clemens. All rights reserved.
//
// ---------------------------------------------------------------------

#ifndef __SHAPESBrush__SHAPESBrush__
#define __SHAPESBrush__SHAPESBrush__

#include <iostream>
#include <cstdlib>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MPxToolCommand.h>
#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MSyntax.h>

#include <maya/M3dView.h>
#include <maya/MCursor.h>
#include <maya/MDagPath.h>
#include <maya/MEvent.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFnCamera.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFrameContext.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MMeshIntersector.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MRichSelection.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MToolsInfo.h>
#include <maya/MUIDrawManager.h>
#include <maya/MVector.h>

// ---------------------------------------------------------------------
// the tool
// ---------------------------------------------------------------------

class SHAPESBrushTool : public MPxToolCommand
{
public:

    SHAPESBrushTool();
    ~SHAPESBrushTool();

    static void* creator();
    static MSyntax newSyntax();

    MStatus parseArgs(const MArgList& args);

    MStatus doIt(const MArgList &args);
    MStatus redoIt();
    MStatus undoIt();
    MStatus finalize();

    bool isUndoable() const;

    // setting the attributes
    void setBlendMesh(MString name);
    void setColor(MColor color);
    void setCurve(int value);
    void setDepth(int value);
    void setDepthStart(int value);
    void setDrawBrush(bool value);
    void setEnterToolCommand(MString value);
    void setExitToolCommand(MString value);
    void setFlushCache(int value);
    void setInvertFreeze(bool value);
    void setInvertPull(bool value);
    void setLineWidth(int value);
    void setMessage(bool value);
    void setPlaneAngle(double value);
    void setRelax(bool value);
    void setSize(double value);
    void setSmooth(double value);
    void setSmoothMesh(MString name);
    void setStrength(double value);
    void setSymmetry(bool value);
    void setSymmetryAxis(int value);
    void setTolerance(double value);
    void setType(int value);
    void setUsePlane(bool value);
    void setVolume(bool value);

public:

    void setMesh(MDagPath dagPath);
    void setSourcePoints(MPointArray points);
    void setDeformedPoints(MPointArray points);

private:

    MString blendMeshVal;
    MColor colorVal;
    int curveVal;
    int depthVal;
    int depthStartVal;
    bool drawBrushVal;
    MString enterToolCommandVal;
    MString exitToolCommandVal;
    int flushCacheVal;
    bool invertFreezeVal;
    bool invertPullVal;
    int lineWidthVal;
    bool messageVal;
    double planeAngleVal;
    bool relaxVal;
    double sizeVal;
    double smoothVal;
    MString smoothMeshVal;
    double strengthVal;
    bool symmetryVal;
    int symmetryAxisVal;
    double toleranceVal;
    int typeVal;
    bool usePlaneVal;
    bool volumeVal;

    MPointArray deformedPoints;
    MDagPath meshDag;
    MPointArray sourcePoints;
};

// ---------------------------------------------------------------------
// the context
// ---------------------------------------------------------------------

class SHAPESBrushContext : public MPxContext
{
public:

    SHAPESBrushContext();
    void toolOnSetup(MEvent &event);
    void toolOffCleanup();

    void getClassName(MString &name) const;

    MStatus doPress(MEvent &event);
    MStatus doDrag(MEvent &event);
    MStatus doRelease(MEvent &event);

    void drawGlCircle3D(MPoint center, double size, MMatrix viewMatrix);

    // VP2.0
    MStatus doPress(MEvent &event,
                    MHWRender::MUIDrawManager &drawManager,
                    const MHWRender::MFrameContext &context);
    MStatus doDrag(MEvent &event,
                   MHWRender::MUIDrawManager &drawManager,
                   const MHWRender::MFrameContext &context);
    MStatus doRelease(MEvent &event,
                      MHWRender::MUIDrawManager &drawManager,
                      const MHWRender::MFrameContext &context);

    // common methods
    MStatus doPressCommon(MEvent event);
    MStatus doDragCommon(MEvent event);
    void doReleaseCommon(MEvent event);
    void displayPlaneHUD(MEvent event, bool create);

    // get meshes and component selection
    MStatus getMesh(MEvent event);
    MStatus getSelection(MDagPath &dagPath);
    MIntArray getSelectionVertices();
    bool getSecondaryMesh(MString meshName,
                          MDagPath &dagPath,
                          MPointArray &points,
                          MMatrix &matrix);
    MStatus setSlideMesh(MEvent event,
                         bool &isSet);
    double averageEdgeLength(unsigned int stepSize);

    // get the closest point on mesh
    bool eventIsValid(MEvent event);
    MStatus getIntersectionIndices(MEvent event,
                                   MIntArray &indices,
                                   MFloatArray &distances);
    MStatus getCameraClip(double &nearClip, double &farClip, MMatrix &camMat);
    unsigned int getViewPlane(MVector viewVector);
    bool adjustSymmetryPoint(MPoint &point);

    // process the vertices within the brush range
    void getPointsInRange(MEvent event);
    void setSoftSelection(bool active,
                          bool soft,
                          bool sym);
    void selectComponent(int index);
    MStatus getSymmetryVertex(int index,
                              int &indexSym);
    bool isFrozen(MColorArray &vtxColors);
    int sideMultiplier(int index);
    bool isFreezeColor(MColor color);
    MStatus matchSymmetryVertexByPosition(MPoint queryPoint,
                                          double tolerance,
                                          int &symmetryIndex);
    double getClosestConnectedVertexDistance(unsigned int index);

    // brush execution
    void editMesh(MEvent event);
    MPointArray computeEdit(MPointArray newPoints,
                            MVector delta,
                            short dragDistance,
                            MEvent event);
    double getFalloffWeight(double value);
    MPoint constrainPlanePoint(MPoint point,
                               MPoint constrainPoint);
    MStatus getClosestSurfacePoint(MPoint point,
                                   MFloatVector normal,
                                   MPoint &closestPoint);
    MPoint averagePoint(MIntArray vtxArray,
                        MPointArray points);
    MPoint centerSymmetryPoint(MPoint point,
                               MPoint referencePoint);
    void setInViewMessage(bool display);

    void getEnvironmentSettings();

    // setting the attributes
    void setBlendMesh(MString name);
    void setColorR(float value);
    void setColorG(float value);
    void setColorB(float value);
    void setCurve(int value);
    void setDepth(int value);
    void setDepthStart(int value);
    void setDrawBrush(bool value);
    void setEnterToolCommand(MString value);
    void setExitToolCommand(MString value);
    void setFlushCache(int value);
    void setInvertFreeze(bool value);
    void setInvertPull(bool value);
    void setLineWidth(int value);
    void setMessage(bool value);
    void setPlaneAngle(double value);
    void setRelax(bool value);
    void setSize(double value);
    void setSmooth(double value);
    void setSmoothMesh(MString name);
    void setStrength(double value);
    void setSymmetry(bool value);
    void setSymmetryAxis(int value);
    void setTolerance(double value);
    void setType(int value);
    void setUsePlane(bool value);
    void setVolume(bool value);

    // getting the attributes
    MString getBlendMesh();
    float getColorR();
    float getColorG();
    float getColorB();
    int getCurve();
    int getDepth();
    int getDepthStart();
    bool getDrawBrush();
    MString getEnterToolCommand();
    MString getExitToolCommand();
    int getFlushCache();
    bool getInvertFreeze();
    bool getInvertPull();
    int getLineWidth();
    bool getMessage();
    double getPlaneAngle();
    bool getRelax();
    double getSize();
    double getSmooth();
    MString getSmoothMesh();
    double getStrength();
    bool getSymmetry();
    int getSymmetryAxis();
    double getTolerance();
    int getType();
    bool getUsePlane();
    bool getVolume();

private:

    SHAPESBrushTool* cmd;

    bool performBrush;
    unsigned int undersamplingSteps;
    unsigned int undersamplingStepsPull;
    unsigned varStepsAdjust;
    unsigned varStepsPull;

    // the tool settings
    MString blendMeshVal;
    MColor colorVal;
    int curveVal;
    int depthVal;
    int depthStartVal;
    bool drawBrushVal;
    MString enterToolCommandVal;
    MString exitToolCommandVal;
    int flushCacheVal;
    bool invertFreezeVal;
    bool invertPullVal;
    int lineWidthVal;
    bool messageVal;
    double planeAngleVal;
    bool relaxVal;
    double sizeVal;
    double smoothVal;
    MString smoothMeshVal;
    double strengthVal;
    bool symmetryVal;
    int symmetryAxisVal;
    double toleranceVal;
    int typeVal;
    bool usePlaneVal;
    bool volumeVal;

    // Brush settings for adjusting.
    bool initAdjust;                // True after the first drag event.
                                    // Controls the adjust direction for
                                    // the size and the strength.
    MPoint surfacePointAdjust;      // Initital surface point of the
                                    // press event.
    MVector worldVectorAdjust;      // Initial view vector of the press
                                    // event.
    double adjustValue;             // The new value for the size or
                                    // strength.

    M3dView view;
    unsigned int width;
    unsigned int height;
    short viewCenterX;
    short viewCenterY;

    // The cursor position.
    short screenX;
    short screenY;
    short startScreenX;
    short startScreenY;
    MPoint worldPoint;
    MPoint worldDragPoint;
    MVector worldDragVector;

    double nearClip;                // The near clip value of the
                                    // camera.
    MPoint nearClipPoint;           // The near clip point in world
                                    // space.

    MPointArray surfacePoints;      // The cursor positions on the mesh
                                    // in world space.
    MVector worldVector;            // The view vector from the camera
                                    // to the surface point.

    MStatus selectionStatus;

    // The mesh being edited.
    MDagPath meshDag;
    MFnMesh meshFn;
    MMatrix meshMat;
    MFloatVectorArray meshNormals;  // The mesh normals.
    MPointArray meshPoints;         // The points of the mesh.
    unsigned int numVertices;

    MIntArray vtxSelection;         // The currently selected vertices.
                                    // This is used to limit the
                                    // operation.
    double symTolerance;

    // The meshes for additional operations.
    MDagPath blendMeshDag;
    bool blendMeshIsSet;
    MMatrix blendMeshMat;
    MPointArray blendMeshPoints;

    MDagPath eraseMeshDag;
    MPointArray eraseMeshPoints;

    MDagPath smoothMeshDag;
    bool smoothMeshIsSet;
    MMatrix smoothMeshMat;
    MPointArray smoothMeshPoints;

    MMeshIsectAccelParams slideMeshAccelParams;
    MFnMesh slideMeshFn;
    MObject slideMeshObj;
    bool slideMeshIsSet;

    unsigned int viewPlane;         // The index of the axis plane the
                                    // operation is performed on.

    MIntArray intersectionIndices;  // The arrays with all intersection
                                    // indices and their distances from
                                    // from the camera.
    MFloatArray intersectionDistances;

    MIntArray faceVerts;            // The array of vertices belong to
                                    // the intersected polygons of the
                                    // ray intersection.
    MIntArray faceVertsCount;       // The number of vertices per face
                                    // for each polygon along the
                                    // intersection ray.
    MFloatArray faceVertsWeights;   // The weights for all vertices of
                                    // all intersected polygons.
    double surfaceDistance;         // The distance of the first
                                    // intersection point to the camera.
                                    // This is used to create a define
                                    // the adjustment speeds as well as
                                    // a factor for the pull/push brush.
    double screenWeight;            // The scale factor for the mesh
                                    // edit based on the relation
                                    // between surface distance and
                                    // length of the intersection ray.
    short pushPullDistance;         // Stores the last dragDistance
                                    // value to determine the direction
                                    // for using the push/pull brush.

    // Variables for storing the current soft selection and symmetry
    // settings.
    MString previousAxis;
    double previousDistance;
    int previousFalloff;
    MString previousFalloffCurve;
    MSelectionList previousSelection;
    int previousSoftSelect;
    int previousSymmetry;

    MIntArray symIds;               // A temporary array to store all
                                    // indices of the symmetry found by
                                    // querying the symmetry.
    MIntArray indices;              // The array which holds all vertex
                                    // indices to edit.
    MIntArray indicesSym;           // The final array which holds all
                                    // vertex indices of the symmetry.
    MFloatArray weights;            // The weight per vertex for the
                                    // edit process.

    MPointArray meshDeformedPoints; // The resulting deformed points.
};

// ---------------------------------------------------------------------
// command to create the context
// ---------------------------------------------------------------------

class SHAPESBrushContextCmd : public MPxContextCommand
{
public:

    SHAPESBrushContextCmd();
    MPxContext* makeObj();
    static void* creator();
    MStatus appendSyntax();
    MStatus doEditFlags();
    MStatus doQueryFlags();

protected:

    SHAPESBrushContext* brushContext;
};

#endif

// ---------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2021 Ingo Clemens, brave rabbit
// SHAPESBrush is under the terms of the MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Author: Ingo Clemens    www.braverabbit.com
// ---------------------------------------------------------------------
