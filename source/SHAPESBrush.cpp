// ---------------------------------------------------------------------
//
//  SHAPESBrush.cpp
//  SHAPESBrush
//
//  Created by ingo on 3/28/14.
//  Copyright (c) 2021 Ingo Clemens. All rights reserved.
//
// ---------------------------------------------------------------------

#include "SHAPESBrush.h"

const float DEGTORAD = 3.14159265359f / 180.0f;
const float RADTODEG = 180.0f / 3.14159265359f;

// Macro for the press/drag/release methods in case there is nothing
// selected or the tool gets applied outside any geometry. If the actual
// MStatus would get returned an error can get listed in terminal on
// Linux. But it's unnecessary and needs to be avoided. Therefore a
// kSuccess is returned just for the sake of being invisible.
#define CHECK_MSTATUS_AND_RETURN_SILENT(status) \
if (status != MStatus::kSuccess)                \
    return MStatus::kSuccess;                   \

const unsigned int PLANE_OFF = 0;
const unsigned int YZ_PLANE = 1;
const unsigned int XZ_PLANE = 2;
const unsigned int XY_PLANE = 3;

const MString FREEZE_SET = "SHAPESBrushFreezeSet";

const unsigned int UNDERSAMPLING = 2;
const unsigned int UNDERSAMPLING_PULL = 3;
const unsigned int MAX_DEPTH = 1;
const unsigned int EDGE_SKIP = 5;

// ---------------------------------------------------------------------
// the tool
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// general methods for the tool command
// ---------------------------------------------------------------------

SHAPESBrushTool::SHAPESBrushTool()
{
    setCommandString("SHAPESBrushCmd");

    blendMeshVal = "";
    colorVal = MColor(0.0, 0.0, 0.0);
    curveVal = 4;
    depthVal = MAX_DEPTH;
    depthStartVal = 1;
    drawBrushVal = false;
    enterToolCommandVal = "";
    exitToolCommandVal = "";
    flushCacheVal = 0;
    invertFreezeVal = false;
    invertPullVal = false;
    lineWidthVal = 1;
    messageVal = true;
    planeAngleVal = 30.0;
    relaxVal = true;
    sizeVal = 5.0;
    smoothVal = 0.25;
    smoothMeshVal = "";
    strengthVal = 0.25;
    symmetryVal = false;
    symmetryAxisVal = 0;
    toleranceVal = 0.0;
    typeVal = 0;
    usePlaneVal = false;
    volumeVal = false;
}

SHAPESBrushTool::~SHAPESBrushTool()
{}

void* SHAPESBrushTool::creator()
{
    return new SHAPESBrushTool;
}

bool SHAPESBrushTool::isUndoable() const
{
    return true;
}


// ---------------------------------------------------------------------
// define the syntax, needed to make it work with mel and python
// ---------------------------------------------------------------------

// The color flag for the brush is originally supposed to be a single
// flag taking three argument values for the rgb components. But the
// Maya issue MAYA-20162 as of 03/12/2018 prevents the MPxContextCommand
// to correctly utilize the MPxCommand::setResult() and ::appendResult()
// to query the three argument values. Therefore the color is divided
// into three separate flags for each color component. This is currently
// a workaround and might get resolved in future versions of Maya.

#define kBlendMeshFlag              "-bm"
#define kBlendMeshFlagLong          "-blendMesh"
#define kColorRFlag                 "-cr"
#define kColorRFlagLong             "-colorR"
#define kColorGFlag                 "-cg"
#define kColorGFlagLong             "-colorG"
#define kColorBFlag                 "-cb"
#define kColorBFlagLong             "-colorB"
#define kCurveFlag                  "-c"
#define kCurveFlagLong              "-curve"
#define kDepthFlag                  "-d"
#define kDepthFlagLong              "-depth"
#define kDepthStartFlag             "-ds"
#define kDepthStartFlagLong         "-depthStart"
#define kDrawBrushFlag              "-db"
#define kDrawBrushFlagLong          "-drawBrush"
#define kEnterToolCommandFlag       "-etc"
#define kEnterToolCommandFlagLong   "-enterToolCommand"
#define kExitToolCommandFlag        "-xtc"
#define kExitToolCommandFlagLong    "-exitToolCommand"
#define kFlushFlag                  "-fc"
#define kFlushFlagLong              "-flushCache"
#define kInvertFreezeFlag           "-if"
#define kInvertFreezeFlagLong       "-invertFreeze"
#define kInvertPullFlag             "-ip"
#define kInvertPullFlagLong         "-invertPull"
#define kLineWidthFlag              "-lw"
#define kLineWidthFlagLong          "-lineWidth"
#define kMessageFlag                "-m"
#define kMessageFlagLong            "-message"
#define kPlaneAngleFlag             "-pa"
#define kPlaneAngleFlagLong         "-planeAngle"
#define kRelaxFlag                  "-r"
#define kRelaxFlagLong              "-relax"
#define kSizeFlag                   "-s"
#define kSizeFlagLong               "-size"
#define kSmoothFlag                 "-sm"
#define kSmoothFlagLong             "-smooth"
#define kSmoothMeshFlag             "-smm"
#define kSmoothMeshFlagLong         "-smoothMesh"
#define kStrengthFlag               "-st"
#define kStrengthFlagLong           "-strength"
#define kSymmetryFlag               "-sym"
#define kSymmetryFlagLong           "-symmetry"
#define kSymmetryAxisFlag           "-sa"
#define kSymmetryAxisFlagLong       "-symmetryAxis"
#define kToleranceFlag              "-tol"
#define kToleranceFlagLong          "-tolerance"
#define kTypeFlag                   "-t"
#define kTypeFlagLong               "-type"
#define kUsePlaneFlag               "-up"
#define kUsePlaneFlagLong           "-usePlane"
#define kVolumeFlag                 "-v"
#define kVolumeFlagLong             "-volume"


MSyntax SHAPESBrushTool::newSyntax()
{
    MSyntax syntax;

    syntax.addFlag(kBlendMeshFlag, kBlendMeshFlagLong, MSyntax::kString);
    syntax.addFlag(kColorRFlag, kColorRFlagLong, MSyntax::kDouble);
    syntax.addFlag(kColorGFlag, kColorGFlagLong, MSyntax::kDouble);
    syntax.addFlag(kColorBFlag, kColorBFlagLong, MSyntax::kDouble);
    syntax.addFlag(kCurveFlag, kCurveFlagLong, MSyntax::kLong);
    syntax.addFlag(kDepthFlag, kDepthFlagLong, MSyntax::kLong);
    syntax.addFlag(kDepthStartFlag, kDepthStartFlagLong, MSyntax::kLong);
    syntax.addFlag(kDrawBrushFlag, kDrawBrushFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kEnterToolCommandFlag, kEnterToolCommandFlagLong, MSyntax::kString);
    syntax.addFlag(kExitToolCommandFlag, kExitToolCommandFlagLong, MSyntax::kString);
    syntax.addFlag(kFlushFlag, kFlushFlagLong, MSyntax::kLong);
    syntax.addFlag(kInvertFreezeFlag, kInvertFreezeFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kInvertPullFlag, kInvertPullFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kLineWidthFlag, kLineWidthFlagLong, MSyntax::kLong);
    syntax.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kPlaneAngleFlag, kPlaneAngleFlagLong, MSyntax::kDouble);
    syntax.addFlag(kRelaxFlag, kRelaxFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kSizeFlag, kSizeFlagLong, MSyntax::kDouble);
    syntax.addFlag(kSmoothFlag, kSmoothFlagLong, MSyntax::kDouble);
    syntax.addFlag(kSmoothMeshFlag, kSmoothMeshFlagLong, MSyntax::kString);
    syntax.addFlag(kStrengthFlag, kStrengthFlagLong, MSyntax::kDouble);
    syntax.addFlag(kSymmetryFlag, kSymmetryFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kSymmetryAxisFlag, kSymmetryAxisFlagLong, MSyntax::kLong);
    syntax.addFlag(kToleranceFlag, kToleranceFlagLong, MSyntax::kDouble);
    syntax.addFlag(kTypeFlag, kTypeFlagLong, MSyntax::kLong);
    syntax.addFlag(kUsePlaneFlag, kUsePlaneFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kVolumeFlag, kVolumeFlagLong, MSyntax::kBoolean);

    return syntax;
}


MStatus SHAPESBrushTool::parseArgs(const MArgList& args)
{
    MStatus status = MStatus::kSuccess;

    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet(kBlendMeshFlag))
    {
        status = argData.getFlagArgument(kBlendMeshFlag, 0, blendMeshVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kColorRFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorRFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor((float)value, colorVal.g, colorVal.b);
    }
    if (argData.isFlagSet(kColorGFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorGFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor(colorVal.r, (float)value, colorVal.b);
    }
    if (argData.isFlagSet(kColorBFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorBFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor(colorVal.r, colorVal.g, (float)value);
    }
    if (argData.isFlagSet(kCurveFlag))
    {
        status = argData.getFlagArgument(kCurveFlag, 0, curveVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    // Disabled because currently only a depth of 1 is supported.
    /*
    if (argData.isFlagSet(kDepthFlag))
    {
        status = argData.getFlagArgument(kDepthFlag, 0, depthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    */
    if (argData.isFlagSet(kDepthStartFlag))
    {
        status = argData.getFlagArgument(kDepthStartFlag, 0, depthStartVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kDrawBrushFlag))
    {
        status = argData.getFlagArgument(kDrawBrushFlag, 0, drawBrushVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kEnterToolCommandFlag))
    {
        status = argData.getFlagArgument(kEnterToolCommandFlag, 0, enterToolCommandVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kExitToolCommandFlag))
    {
        status = argData.getFlagArgument(kExitToolCommandFlag, 0, exitToolCommandVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kFlushFlag))
    {
        status = argData.getFlagArgument(kFlushFlag, 0, flushCacheVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kInvertFreezeFlag))
    {
        status = argData.getFlagArgument(kInvertFreezeFlag, 0, invertFreezeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kInvertPullFlag))
    {
        status = argData.getFlagArgument(kInvertPullFlag, 0, invertPullVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kLineWidthFlag))
    {
        status = argData.getFlagArgument(kLineWidthFlag, 0, lineWidthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kMessageFlag))
    {
        status = argData.getFlagArgument(kMessageFlag, 0, messageVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kPlaneAngleFlag))
    {
        status = argData.getFlagArgument(kPlaneAngleFlag, 0, planeAngleVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kRelaxFlag))
    {
        status = argData.getFlagArgument(kRelaxFlag, 0, relaxVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSizeFlag))
    {
        status = argData.getFlagArgument(kSizeFlag, 0, sizeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSmoothFlag))
    {
        status = argData.getFlagArgument(kSmoothFlag, 0, smoothVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSmoothMeshFlag))
    {
        status = argData.getFlagArgument(kSmoothMeshFlag, 0, smoothMeshVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kStrengthFlag))
    {
        status = argData.getFlagArgument(kStrengthFlag, 0, strengthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSymmetryFlag))
    {
        status = argData.getFlagArgument(kSymmetryFlag, 0, symmetryVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSymmetryAxisFlag))
    {
        status = argData.getFlagArgument(kSymmetryAxisFlag, 0, symmetryAxisVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kToleranceFlag))
    {
        status = argData.getFlagArgument(kToleranceFlag, 0, toleranceVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kTypeFlag))
    {
        status = argData.getFlagArgument(kTypeFlag, 0, typeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kUsePlaneFlag))
    {
        status = argData.getFlagArgument(kUsePlaneFlag, 0, usePlaneVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kVolumeFlag))
    {
        status = argData.getFlagArgument(kVolumeFlag, 0, volumeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return status;
}


// ---------------------------------------------------------------------
// main methods for the tool command
// ---------------------------------------------------------------------

MStatus SHAPESBrushTool::doIt(const MArgList &args)
{
    MStatus status = MStatus::kSuccess;

    status = parseArgs(args);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}


MStatus SHAPESBrushTool::redoIt()
{
    MStatus status = MStatus::kSuccess;

    if (deformedPoints.length())
    {
        MFnMesh meshFn(meshDag, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        meshFn.setPoints(deformedPoints, MSpace::kWorld);
    }

    return status;
}


MStatus SHAPESBrushTool::undoIt()
{
    MStatus status = MStatus::kSuccess;

    if (sourcePoints.length())
    {
        MFnMesh meshFn(meshDag, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        meshFn.setPoints(sourcePoints, MSpace::kWorld);
    }

    return status;
}

MStatus SHAPESBrushTool::finalize()
{
    // Store the current settings as an option var. This way they are
    // properly available for the next usage.

    MString cmd;
    cmd = "SHAPESBrushContext -edit ";
    cmd += "-image1 \"SHAPESBrush.svg\" -image2 \"vacantCell.png\" -image3 \"vacantCell.png\" ";
    cmd += " " + MString(kBlendMeshFlag) + " ";
    cmd += "\"" + blendMeshVal + "\"";
    cmd += " " + MString(kColorRFlag) + " ";
    cmd += colorVal.r;
    cmd += " " + MString(kColorGFlag) + " ";
    cmd += colorVal.g;
    cmd += " " + MString(kColorBFlag) + " ";
    cmd += colorVal.b;
    cmd += " " + MString(kCurveFlag) + " ";
    cmd += curveVal;
    cmd += " " + MString(kDepthFlag) + " ";
    cmd += depthVal;
    cmd += " " + MString(kDepthStartFlag) + " ";
    cmd += depthStartVal;
    cmd += " " + MString(kDrawBrushFlag) + " ";
    cmd += drawBrushVal;
    cmd += " " + MString(kEnterToolCommandFlag) + " ";
    cmd += "\"" + enterToolCommandVal + "\"";
    cmd += " " + MString(kExitToolCommandFlag) + " ";
    cmd += "\"" + exitToolCommandVal + "\"";
    cmd += " " + MString(kFlushFlag) + " ";
    cmd += flushCacheVal;
    cmd += " " + MString(kInvertFreezeFlag) + " ";
    cmd += invertFreezeVal;
    cmd += " " + MString(kInvertPullFlag) + " ";
    cmd += invertPullVal;
    cmd += " " + MString(kLineWidthFlag) + " ";
    cmd += lineWidthVal;
    cmd += " " + MString(kMessageFlag) + " ";
    cmd += messageVal;
    cmd += " " + MString(kPlaneAngleFlag) + " ";
    cmd += planeAngleVal;
    cmd += " " + MString(kRelaxFlag) + " ";
    cmd += relaxVal;
    cmd += " " + MString(kSizeFlag) + " ";
    cmd += sizeVal;
    cmd += " " + MString(kSmoothFlag) + " ";
    cmd += smoothVal;
    cmd += " " + MString(kSmoothMeshFlag) + " ";
    cmd += "\"" + smoothMeshVal + "\"";
    cmd += " " + MString(kStrengthFlag) + " ";
    cmd += strengthVal;
    cmd += " " + MString(kSymmetryFlag) + " ";
    cmd += symmetryVal;
    cmd += " " + MString(kSymmetryAxisFlag) + " ";
    cmd += symmetryAxisVal;
    cmd += " " + MString(kToleranceFlag) + " ";
    cmd += toleranceVal;
    cmd += " " + MString(kTypeFlag) + " ";
    cmd += typeVal;
    cmd += " " + MString(kUsePlaneFlag) + " ";
    cmd += usePlaneVal;
    cmd += " " + MString(kVolumeFlag) + " ";
    cmd += volumeVal;
    cmd += " SHAPESBrushContext1;";

    MGlobal::setOptionVarValue("SHAPESBrushContext1", cmd);

    // Finalize the command by adding it to the undo queue and the
    // journal.
    MArgList command;
    command.addArg(commandString());

    return MPxToolCommand::doFinalize(command);
}


// ---------------------------------------------------------------------
// getting values from the command flags
// ---------------------------------------------------------------------

void SHAPESBrushTool::setBlendMesh(MString name)
{
    blendMeshVal = name;
}


void SHAPESBrushTool::setColor(MColor value)
{
    colorVal = value;
}


void SHAPESBrushTool::setCurve(int value)
{
    curveVal = value;
}


void SHAPESBrushTool::setDepth(int value)
{
    depthVal = MAX_DEPTH;
}


void SHAPESBrushTool::setDepthStart(int value)
{
    depthStartVal = value;
}


void SHAPESBrushTool::setDrawBrush(bool value)
{
    drawBrushVal = value;
}


void SHAPESBrushTool::setEnterToolCommand(MString value)
{
    enterToolCommandVal = value;
}


void SHAPESBrushTool::setExitToolCommand(MString value)
{
    exitToolCommandVal = value;
}


void SHAPESBrushTool::setFlushCache(int value)
{
    flushCacheVal = value;
}


void SHAPESBrushTool::setInvertFreeze(bool value)
{
    invertFreezeVal = value;
}


void SHAPESBrushTool::setInvertPull(bool value)
{
    invertPullVal = value;
}


void SHAPESBrushTool::setLineWidth(int value)
{
    lineWidthVal = value;
}


void SHAPESBrushTool::setMessage(bool value)
{
    messageVal = value;
}


void SHAPESBrushTool::setPlaneAngle(double value)
{
    planeAngleVal = value;
}


void SHAPESBrushTool::setRelax(bool value)
{
    relaxVal = value;
}


void SHAPESBrushTool::setSize(double value)
{
    sizeVal = value;
}


void SHAPESBrushTool::setSmooth(double value)
{
    smoothVal = value;
}


void SHAPESBrushTool::setSmoothMesh(MString name)
{
    smoothMeshVal = name;
}


void SHAPESBrushTool::setStrength(double value)
{
    strengthVal = value;
}


void SHAPESBrushTool::setSymmetry(bool value)
{
    symmetryVal = value;
}


void SHAPESBrushTool::setSymmetryAxis(int value)
{
    symmetryAxisVal = value;
}


void SHAPESBrushTool::setTolerance(double value)
{
    toleranceVal = value;
}


void SHAPESBrushTool::setType(int value)
{
    typeVal = value;
}


void SHAPESBrushTool::setUsePlane(bool value)
{
    usePlaneVal = value;
}


void SHAPESBrushTool::setVolume(bool value)
{
    volumeVal = value;
}


// ---------------------------------------------------------------------
// public methods for setting the undo/redo variables
// ---------------------------------------------------------------------

void SHAPESBrushTool::setMesh(MDagPath dagPath)
{
    meshDag = dagPath;
}


void SHAPESBrushTool::setSourcePoints(MPointArray points)
{
    sourcePoints = points;
}


void SHAPESBrushTool::setDeformedPoints(MPointArray points)
{
    deformedPoints = points;
}


// ---------------------------------------------------------------------
// the context
// ---------------------------------------------------------------------

const char helpString[] = "Grab Brush. Use Shift to smooth and Ctrl to slide/push/pull/erase/blend";


// ---------------------------------------------------------------------
// general methods when calling the context
// ---------------------------------------------------------------------

SHAPESBrushContext::SHAPESBrushContext()
{
    setTitleString("SHAPES Brush Tool");
    setImage("SHAPESBrush.svg", MPxContext::kImage1);
    setCursor(MCursor::crossHairCursor);

    // Define the default values for the context.
    // These values will be used to reset the tool from the tool
    // properties window.
    blendMeshVal = "";
    colorVal = MColor(0.0, 0.0, 0.0);
    curveVal = 4;
    depthVal = MAX_DEPTH;
    depthStartVal = 1;
    drawBrushVal = false;
    enterToolCommandVal = "";
    exitToolCommandVal = "";
    flushCacheVal = 0;
    invertFreezeVal = false;
    invertPullVal = false;
    lineWidthVal = 1;
    messageVal = true;
    planeAngleVal = 30.0;
    relaxVal = true;
    sizeVal = 5.0;
    smoothVal = 0.25;
    smoothMeshVal = "";
    strengthVal = 0.25;
    symmetryVal = false;
    symmetryAxisVal = 0;
    toleranceVal = 0.0;
    typeVal = 0;
    usePlaneVal = false;
    volumeVal = false;

    // True, editing is performed. False when adjusting the brush
    // settings. It's used to control whether undo/redo needs to get
    // called.
    performBrush = false;
}


void SHAPESBrushContext::toolOnSetup(MEvent &event)
{
    getEnvironmentSettings();

    setHelpString(helpString);

    MGlobal::executeCommand(enterToolCommandVal);

    setInViewMessage(true);
}


void SHAPESBrushContext::toolOffCleanup()
{
    setInViewMessage(false);

    MGlobal::executeCommand(exitToolCommandVal);
}


void SHAPESBrushContext::getClassName(MString & name) const
{
    name.set("SHAPESBrush");
}


// ---------------------------------------------------------------------
// legacy viewport
// ---------------------------------------------------------------------

MStatus SHAPESBrushContext::doPress(MEvent &event)
{
    selectionStatus = doPressCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    displayPlaneHUD(event, true);
    doDrag(event);
    return MStatus::kSuccess;
}


MStatus SHAPESBrushContext::doDrag(MEvent &event)
{
    // If doPressCommon failed then don't continue.
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    MStatus status = MStatus::kSuccess;

    // Execute the brush action or change the attribute values.
    status = doDragCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(status);

    // In order to draw the circle in 3d space but oriented to the view
    // get the model view matrix and reset the translation and scale.
    // The points to draw are then multiplied by the inverse matrix.
    MMatrix modelViewMat;
    view.modelViewMatrix(modelViewMat);
    MTransformationMatrix transMat(modelViewMat);
    transMat.setTranslation(MVector(), MSpace::kWorld);
    const double scale[3] = {1.0, 1.0, 1.0};
    transMat.setScale(scale, MSpace::kWorld);
    modelViewMat = transMat.asMatrix();

    view.beginXorDrawing(false, true, (float)lineWidthVal, M3dView::kStippleNone);

    // -----------------------------------------------------------------
    // display when painting or setting the brush size
    // -----------------------------------------------------------------
    if (drawBrushVal || event.mouseButton() == MEvent::kMiddleMouse)
    {
        // Draw the circle in regular paint mode.
        if (event.mouseButton() == MEvent::kLeftMouse)
        {
            // During smoothing and using the secondary brush the circle
            // remains static.
            MPoint centerPoint = surfacePoints[0];

            // To make the circle move with the mouse position during
            // the drag modify the current world drag point, which is
            // generated during editMesh(), by moving it along it's
            // vector, which is scaled by the surface distance.
            if (event.isModifierNone())
                centerPoint = worldDragPoint + worldDragVector * surfaceDistance;

            drawGlCircle3D(centerPoint, sizeVal, modelViewMat);
        }
        // Adjusting the brush settings with the middle mouse button.
        else if (event.mouseButton() == MEvent::kMiddleMouse)
        {
            // When adjusting the size the circle needs to remain with
            // a static position but the size needs to change.
            if (event.isModifierNone())
            {
                drawGlCircle3D(surfacePointAdjust, adjustValue, modelViewMat);
            }
            // When adjusting the strength the circle needs to remain
            // fixed and only the strength indicator changes.
            else
            {
                drawGlCircle3D(surfacePointAdjust, sizeVal, modelViewMat);
            }
        }
    }

    view.endXorDrawing();

    return status;
}


MStatus SHAPESBrushContext::doRelease(MEvent &event)
{
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    doReleaseCommon(event);
    displayPlaneHUD(event, false);
    return MStatus::kSuccess;
}


void SHAPESBrushContext::drawGlCircle3D(MPoint center, double size, MMatrix viewMatrix)
{
    unsigned int i;

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < 360; i +=2)
    {
        double degInRad = i * DEGTORAD;
        MPoint point(cos(degInRad) * size, sin(degInRad) * size, 0.0);
        point *= viewMatrix.inverse();
        glVertex3f(float(point.x + center.x), float(point.y + center.y), float(point.z + center.z));
    }
    glEnd();
}


// ---------------------------------------------------------------------
// viewport 2.0
// ---------------------------------------------------------------------

MStatus SHAPESBrushContext::doPress(MEvent &event,
                                    MHWRender::MUIDrawManager &drawMgr,
                                    const MHWRender::MFrameContext &context)
{
    selectionStatus = doPressCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    displayPlaneHUD(event, true);
    doDrag(event, drawMgr, context);
    return MStatus::kSuccess;
}


MStatus SHAPESBrushContext::doDrag(MEvent &event,
                                   MHWRender::MUIDrawManager &drawManager,
                                   const MHWRender::MFrameContext &context)
{
    // If doPressCommon failed then don't continue.
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    MStatus status = MStatus::kSuccess;

    // Execute the brush action or change the attribute values.
    status = doDragCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(status);

    drawManager.beginDrawable();

    drawManager.setColor(MColor((pow(colorVal.r, 0.454f)),
                                (pow(colorVal.g, 0.454f)),
                                (pow(colorVal.b, 0.454f))));
    drawManager.setLineWidth((float)lineWidthVal);

    // -----------------------------------------------------------------
    // display when painting or setting the brush size
    // -----------------------------------------------------------------
    if (drawBrushVal || event.mouseButton() == MEvent::kMiddleMouse)
    {
        // Draw the circle in regular paint mode.
        if (event.mouseButton() == MEvent::kLeftMouse)
        {
            // During smoothing and using the secondary brush the circle
            // remains static.
            MPoint centerPoint = surfacePoints[0];
            MVector circleVector = worldVector;

            // To make the circle move with the mouse position during
            // the drag modify the current world drag point, which is
            // generated during editMesh(), by moving it along it's
            // vector, which is scaled by the surface distance.
            if (event.isModifierNone())
            {
                centerPoint = worldDragPoint + worldDragVector * surfaceDistance;
                circleVector = worldDragVector;
            }
            drawManager.circle(centerPoint, circleVector, sizeVal);
        }
        // Adjusting the brush settings with the middle mouse button.
        else if (event.mouseButton() == MEvent::kMiddleMouse)
        {
            // When adjusting the size the circle needs to remain with
            // a static position but the size needs to change.
            if (event.isModifierNone())
            {
                drawManager.circle(surfacePointAdjust, worldVectorAdjust, adjustValue);
            }
            // When adjusting the strength the circle needs to remain
            // fixed and only the strength indicator changes.
            else
            {
                drawManager.circle(surfacePointAdjust, worldVectorAdjust, sizeVal);

                MPoint start(startScreenX, startScreenY);
                MPoint end(startScreenX, startScreenY + adjustValue * 500);
                drawManager.line2d(start, end);

                drawManager.circle2d(end, lineWidthVal + 3.0, true);
            }
        }
    }

    drawManager.endDrawable();

    return status;
}


MStatus SHAPESBrushContext::doRelease(MEvent &event,
                                      MHWRender::MUIDrawManager &drawMgr,
                                      const MHWRender::MFrameContext &context)
{
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    doReleaseCommon(event);
    displayPlaneHUD(event, false);
    return MStatus::kSuccess;
}


// ---------------------------------------------------------------------
//
// common event methods for legacy viewport and viewport 2.0
//
// ---------------------------------------------------------------------

MStatus SHAPESBrushContext::doPressCommon(MEvent event)
{
    MStatus status = MStatus::kSuccess;

    view = M3dView::active3dView();
    // Get the screen position before the mesh so that it's possible to
    // select from screen in case nothing is currently selected.
    event.getPosition(screenX, screenY);

    // -----------------------------------------------------------------
    // component selection
    // -----------------------------------------------------------------

    // Get the component selection before evaluating the mesh or the
    // component selection will get overwritten.
    vtxSelection = getSelectionVertices();

    // -----------------------------------------------------------------
    // mesh
    // -----------------------------------------------------------------

    getMesh(event);
    // Update the in-view message to account for changes regarding the
    // blend or smooth meshes.
    setInViewMessage(true);

    // Make sure that the mesh is valid and the mouse button is pressed.
    if (meshDag.node().isNull() || !eventIsValid(event))
        return MStatus::kNotFound;

    // initialize
    performBrush = false;
    undersamplingSteps = 0;
    undersamplingStepsPull = 0;
    pushPullDistance = 0;
    viewPlane = PLANE_OFF;

    // Get the size of the viewport and calculate the center for placing
    // the value messages when adjusting the brush settings.
    unsigned int x;
    unsigned int y;
    view.viewport(x, y, width, height);
    viewCenterX = (short)width / 2;
    viewCenterY = (short)height / 2;

    // Store the initial mouse position. These get used when adjusting
    // the brush size and strength values.
    startScreenX = screenX;
    startScreenY = screenY;

    // Reset the adjustment from the previous drag.
    initAdjust = false;
    adjustValue = 0.0;

    // -----------------------------------------------------------------
    // closest point on surface
    // -----------------------------------------------------------------

    // Get the vertex index which is closest to the cursor position.
    // This method also defines the surface point and view vector.
    // It also includes a check that the array of closest indices is not
    // empty so that it can be used without raising any errors.
    intersectionIndices.clear();
    intersectionDistances.clear();
    status = getIntersectionIndices(event, intersectionIndices, intersectionDistances);
    if (status == MStatus::kNotFound)
        return status;

    // Store the initial surface point and view vector to use when
    // the brush settings are adjusted because the brush circle
    // needs to be static during the adjustment.
    surfacePointAdjust = surfacePoints[0];
    worldVectorAdjust = worldVector;

    // -----------------------------------------------------------------
    // affected points
    // -----------------------------------------------------------------
    if (event.mouseButton() == MEvent::kLeftMouse)
        getPointsInRange(event);

    return status;
}


MStatus SHAPESBrushContext::doDragCommon(MEvent event)
{
    MStatus status = MStatus::kSuccess;

    // -----------------------------------------------------------------
    // Dragging with the left mouse button performs the brush action.
    // -----------------------------------------------------------------
    if (event.mouseButton() == MEvent::kLeftMouse)
    {
        if (event.isModifierControl() && typeVal == 1)
        {
            // For the push/pull brush skip several evaluation steps.
            // This improves the differentiation for the direction of
            // the brush.
            undersamplingStepsPull ++;
            if (undersamplingStepsPull < varStepsPull)
                return status;
            undersamplingStepsPull = 0;
        }
        editMesh(event);
    }
    // -----------------------------------------------------------------
    // Dragging with the middle mouse button adjusts the settings.
    // -----------------------------------------------------------------
    else if (event.mouseButton() == MEvent::kMiddleMouse)
    {
        // Skip several evaluation steps. This has several reasons:
        // - It lets adjusting the brush appear smoother because the
        //   lines show less flicker.
        // - It also improves the differentiation between horizontal and
        //   vertical dragging when adjusting.
        undersamplingSteps ++;
        if (undersamplingSteps < varStepsAdjust)
            return status;
        undersamplingSteps = 0;

        // Get the current and initial cursor position and calculate the
        // delta movement from them.
        event.getPosition(screenX, screenY);
        MPoint currentPos(screenX, screenY);
        MPoint startPos(startScreenX, startScreenY);
        MPoint deltaPos(currentPos - startPos);

        // InitAdjust makes sure that direction gets set on the first
        // drag event and gets reset the next time a mouse button is
        // pressed.
        if (!initAdjust)
            initAdjust = true;

        // Define the settings for either setting the brush size or the
        // brush strength.
        MString message = "Brush Size";
        MString slider = "Size";
        double dragDistance = deltaPos.x;
        double min = 0.0;
        unsigned int max = 1000;
        double baseValue = sizeVal;
        double speed = 1.0;

        // Vary the settings if the strength gets adjusted.
        if (!event.isModifierNone())
        {
            message = "Smooth Strength";
            slider = "Smooth";
            baseValue = smoothVal;
            if (event.isModifierControl())
            {
                message = "Brush Strength";
                slider = "Strength";
                baseValue = strengthVal;
            }
            dragDistance = deltaPos.y;
            max = 1;
            speed = 0.01;
        }
        else
        {
            // The adjustment speed depends on the distance to the mesh.
            // Closer distances allows for a finer control whereas
            // larger distances need a coarser control.
            speed = pow(0.001 * surfaceDistance, 0.9);
        }

        // Calculate the new value by adding the drag distance to the
        // start value.
        double value = baseValue + dragDistance * speed;

        // Clamp the values to the min/max range.
        if (value < min)
            value = min;
        else if (value > max)
            value = max;

        // Store the modified value for drawing and for setting the
        // values when releasing the mouse button.
        adjustValue = value;

        // -------------------------------------------------------------
        // value display in the viewport
        // -------------------------------------------------------------
        char info[32];
#ifdef _WIN64
        sprintf_s(info, "%s: %.2f", message.asChar(), adjustValue);
#else
        sprintf(info, "%s: %.2f", message.asChar(), adjustValue);
#endif

        // Calculate the position for the value display. Since the
        // heads-up message starts at the center of the viewport an
        // offset needs to get calculated based on the view size and the
        // initial adjust position of the cursor.
        short offsetX = startScreenX - viewCenterX;
        short offsetY = startScreenY - viewCenterY - 50;

        MString cmd = "headsUpMessage -horizontalOffset ";
        cmd += offsetX;
        cmd += " -verticalOffset ";
        cmd += offsetY;
        cmd += " -time 0.1 ";
        cmd += "\"" + MString(info) + "\"";
        MGlobal::executeCommand(cmd);

        // Also, adjust the slider in the tool settings window if it's
        // currently open.
        MString tool("SHAPESBrush");
        MGlobal::executeCommand("if (`columnLayout -exists " + tool + "`) " +
                                "floatSliderGrp -edit -value " + (MString() + adjustValue) + " " +
                                tool + slider + ";");
    }

    return status;
}


void SHAPESBrushContext::doReleaseCommon(MEvent event)
{
    // Don't continue if no mesh has been set.
    if (meshFn.object().isNull())
        return;

    // Define, which brush setting has been adjusted and needs to get
    // stored.
    if (event.mouseButton() == MEvent::kMiddleMouse)
    {
        if (event.isModifierNone())
            sizeVal = adjustValue;
        else
        {
            if (event.isModifierShift())
                smoothVal = adjustValue;
            else if (event.isModifierControl())
                strengthVal = adjustValue;
        }
    }

    // Refresh the view to erase the drawn circle. This might not
    // always be necessary but is just included to complete the process.
    view.refresh(false, true);

    // Free the intersection accelerators
    meshFn.freeCachedIntersectionAccelerator();
    if (slideMeshIsSet)
    {
        slideMeshFn.freeCachedIntersectionAccelerator();
        MGlobal::deleteNode(slideMeshObj);
    }

    // If the editing has been performed send the current values to
    // the tool command along with the necessary data for undo and redo.
    if (performBrush)
    {
        cmd = (SHAPESBrushTool*)newToolCommand();

        cmd->setBlendMesh(blendMeshVal);
        cmd->setColor(colorVal);
        cmd->setCurve(curveVal);
        cmd->setDepth(depthVal);
        cmd->setDepthStart(depthStartVal);
        cmd->setDrawBrush(drawBrushVal);
        cmd->setEnterToolCommand(enterToolCommandVal);
        cmd->setExitToolCommand(exitToolCommandVal);
        cmd->setFlushCache(flushCacheVal);
        cmd->setInvertFreeze(invertFreezeVal);
        cmd->setInvertPull(invertPullVal);
        cmd->setLineWidth(lineWidthVal);
        cmd->setMessage(messageVal);
        cmd->setPlaneAngle(planeAngleVal);
        cmd->setRelax(relaxVal);
        cmd->setSize(sizeVal);
        cmd->setSmooth(smoothVal);
        cmd->setSmoothMesh(smoothMeshVal);
        cmd->setStrength(strengthVal);
        cmd->setSymmetry(symmetryVal);
        cmd->setSymmetryAxis(symmetryAxisVal);
        cmd->setTolerance(toleranceVal);
        cmd->setType(typeVal);
        cmd->setUsePlane(usePlaneVal);
        cmd->setVolume(volumeVal);

        cmd->setMesh(meshDag);
        cmd->setSourcePoints(meshPoints);
        cmd->setDeformedPoints(meshDeformedPoints);

        // Regular context implementations usually call
        // (MPxToolCommand)::redoIt at this point but in this case it
        // is not necessary since the the smoothing already has been
        // performed. There is no need to apply the values twice.

        cmd->finalize();
    }
}


//
// Description:
//      Create or remove the plane HUD during the editing.
//
// Input Arguments:
//      event               The mouse event.
//      create              True, if the HUD should be created.
//
void SHAPESBrushContext::displayPlaneHUD(MEvent event, bool create)
{
    if ((typeVal >= 0 && typeVal <= 2) && event.mouseButton() == MEvent::kLeftMouse)
    {
        unsigned int plane = viewPlane;
        if (!create)
            plane = 0;
        MString cmd;
        cmd = "SHAPESBrushCreatePlaneHUD ";
        cmd += plane;
        MGlobal::executeCommand(cmd);
    }
}


// ---------------------------------------------------------------------
//
// get meshes and component selection
//
// ---------------------------------------------------------------------

MStatus SHAPESBrushContext::getMesh(MEvent event)
{
    MStatus status = MStatus::kSuccess;

    // Clear the previous data.
    meshDag = MDagPath();
    blendMeshIsSet = false;
    smoothMeshIsSet = false;
    slideMeshIsSet = false;

    // -----------------------------------------------------------------
    // mesh
    // -----------------------------------------------------------------
    MDagPath dagPath;
    status = getSelection(meshDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set the mesh.
    meshFn.setObject(meshDag);
    numVertices = (unsigned)meshFn.numVertices();

    meshFn.getPoints(meshPoints, MSpace::kWorld);
    meshFn.getVertexNormals(true, meshNormals, MSpace::kWorld);
    meshMat = meshDag.inclusiveMatrix();

    if (toleranceVal == 0)
    {
        double edgeLength = averageEdgeLength(EDGE_SKIP);
        symTolerance = edgeLength / 10.0;
    }
    else
        symTolerance = toleranceVal;

    // -----------------------------------------------------------------
    // blend mesh
    // -----------------------------------------------------------------
    MDagPath blendDag;
    MPointArray blendPoints;
    blendMeshIsSet = getSecondaryMesh(blendMeshVal,
                                      blendDag,
                                      blendPoints,
                                      blendMeshMat);

    // Store the mesh points for the blend brush.
    if (blendMeshDag == blendDag && blendPoints.length() == meshPoints.length())
        ;
    else
    {
        if (blendMeshIsSet && blendPoints.length() == meshPoints.length())
        {
            blendMeshDag = blendDag;
            blendMeshPoints = blendPoints;
        }
        else
        {
            blendMeshVal = "";
            blendMeshIsSet = false;
        }
    }

    // -----------------------------------------------------------------
    // smooth base mesh
    // -----------------------------------------------------------------
    MDagPath smoothDag;
    MPointArray smoothPoints;
    smoothMeshIsSet = getSecondaryMesh(smoothMeshVal,
                                       smoothDag,
                                       smoothPoints,
                                       smoothMeshMat);

    // Store the mesh points for the smooth mesh based smoothing.
    if (smoothMeshDag == smoothDag && smoothPoints.length() == meshPoints.length())
        ;
    else
    {
        if (smoothMeshIsSet && smoothPoints.length() == meshPoints.length())
        {
            smoothMeshDag = smoothDag;
            smoothMeshPoints = smoothPoints;
        }
        else
        {
            smoothMeshVal = "";
            smoothMeshIsSet = false;
        }
    }

    // -----------------------------------------------------------------
    // matrices
    // -----------------------------------------------------------------

    // Get the transform matrix to correctly calculate the point
    // positions during drag.
    if (!blendMeshIsSet)
        blendMeshMat = meshMat;
    if (!smoothMeshIsSet)
        smoothMeshMat = meshMat;

    // -----------------------------------------------------------------
    // erase data
    // -----------------------------------------------------------------
    if (meshDag == eraseMeshDag && meshPoints.length() == eraseMeshPoints.length())
        ;
    else
    {
        eraseMeshDag = meshDag;
        eraseMeshPoints = meshPoints;
    }

    // -----------------------------------------------------------------
    // sliding & smoothing mesh
    // -----------------------------------------------------------------

    status = setSlideMesh(event, slideMeshIsSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // -----------------------------------------------------------------
    // selection
    // -----------------------------------------------------------------

    // Select only the mesh in case the blend mesh is selected as well.
    if (!vtxSelection.length())
    {
        MSelectionList sel;
        sel.add(meshDag);
        MGlobal::setActiveSelectionList(sel);
    }

    return status;
}


//
// Description:
//      Get the dagPath of the currently selected object's shape node.
//      If there are multiple shape nodes return the first
//      non-intermediate shape. Return kNotFound if the object is not a
//      mesh.
//
// Input Arguments:
//      dagPath             The MDagPath of the selected mesh.
//
// Return Value:
//      MStatus             Return kNotFound if nothing is selected or
//                          the selection is not a mesh.
//
MStatus SHAPESBrushContext::getSelection(MDagPath &dagPath)
{
    MStatus status = MStatus::kSuccess;

    unsigned int i;

    MSelectionList sel;
    status = MGlobal::getActiveSelectionList(sel);

    // If nothing is selected get the object at the cursor position.
    if (sel.isEmpty())
    {
        if (MGlobal::selectFromScreen(screenX, screenY, MGlobal::kReplaceList) == MStatus::kFailure)
            return MStatus::kNotFound;
        MGlobal::getActiveSelectionList(sel);
    }

    if (sel.isEmpty())
        return MStatus::kNotFound;

    // Get the dagPath of the mesh before evaluating any selected
    // components. If there are no components selected the dagPath would
    // be empty.
    sel.getDagPath(0, dagPath);
    status = dagPath.extendToShape();

    if (sel.length() == 2)
    {
        MDagPath blendDagPath;
        sel.getDagPath(1, blendDagPath);
        status = blendDagPath.extendToShape();
        blendMeshVal = blendDagPath.fullPathName();
    }

    // If there is more than one shape node extend to shape will fail.
    // In this case the shape node needs to be found differently.
    if (status != MStatus::kSuccess)
    {
        unsigned int numShapes;
        dagPath.numberOfShapesDirectlyBelow(numShapes);
        for (i = 0; i < numShapes; i ++)
        {
            status = dagPath.extendToShapeDirectlyBelow(i);
            if (status == MStatus::kSuccess)
            {
                MFnDagNode shapeDag(dagPath);
                if (!shapeDag.isIntermediateObject())
                    break;
            }
        }
    }

    if (!dagPath.hasFn(MFn::kMesh))
    {
        dagPath = MDagPath();
        MGlobal::displayWarning("Only mesh objects are supported.");
        return MStatus::kNotFound;
    }

    if (!status)
    {
        MGlobal::clearSelectionList();
        dagPath = MDagPath();
    }

    return status;
}


//
// Description:
//      Parse the currently selected components and return a list of
//      related vertex indices. Edge or polygon selections are converted
//      to vertices.
//
// Input Arguments:
//      None
//
// Return Value:
//      The array of selected vertex indices
//
MIntArray SHAPESBrushContext::getSelectionVertices()
{
    unsigned int i;

    MIntArray vtxIndices;
    MDagPath dagPath;
    MObject compObj;

    MSelectionList sel;
    MGlobal::getActiveSelectionList(sel);

    for (MItSelectionList selIter(sel, MFn::kMeshVertComponent); !selIter.isDone(); selIter.next())
    {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull())
        {
            for (MItMeshVertex vertexIter(dagPath, compObj); !vertexIter.isDone(); vertexIter.next())
            {
                vtxIndices.append(vertexIter.index());
            }
        }
    }

    for (MItSelectionList selIter(sel, MFn::kMeshEdgeComponent); !selIter.isDone(); selIter.next())
    {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull())
        {
            for (MItMeshEdge edgeIter(dagPath, compObj); !edgeIter.isDone(); edgeIter.next())
            {
                vtxIndices.append(edgeIter.index(0));
                vtxIndices.append(edgeIter.index(1));
            }
        }
    }

    for (MItSelectionList selIter(sel, MFn::kMeshPolygonComponent); !selIter.isDone(); selIter.next())
    {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull())
        {
            for (MItMeshPolygon polyIter(dagPath, compObj); !polyIter.isDone(); polyIter.next())
            {
                MIntArray vertices;
                polyIter.getVertices(vertices);
                for (i = 0; i < vertices.length(); i ++)
                    vtxIndices.append(vertices[i]);
            }
        }
    }

    // Remove any double entries from the component list.
    // MFnSingleIndexedComponent does that automatically.
    MFnSingleIndexedComponent compFn;
    MObject verticesObj = compFn.create(MFn::kMeshVertComponent);
    compFn.addElements(vtxIndices);
    // Put the processed ids back into the array.
    compFn.getElements(vtxIndices);

    return vtxIndices;
}


//
// Description:
//      Process the mesh from the given name and return it's data.
//
// Input Arguments:
//      meshName            The name string of the mesh to process.
//      dagPath             The dag path of the mesh.
//      points              The MPoints of the mesh.
//      matrix              The MMatrix fo the mesh.
//      isSet               True, if getting the mesh was successful.
//      displayName         The name to display in the in-view message.
//
// Return Value:
//      bool                If getting the mesh was succeesful.
//
//
bool SHAPESBrushContext::getSecondaryMesh(MString meshName,
                                          MDagPath &dagPath,
                                          MPointArray &points,
                                          MMatrix &matrix)
{
    MStatus status = MStatus::kSuccess;

    bool isSet = false;

    MSelectionList sel;

    if (meshName == "")
        return isSet;

    status = sel.add(meshName);
    if (status != MStatus::kSuccess)
        return isSet;
    status = sel.getDagPath(0, dagPath);
    if (status != MStatus::kSuccess || !dagPath.isValid())
        return isSet;

    dagPath.extendToShape();

    if (!dagPath.hasFn(MFn::kMesh))
    {
        MGlobal::displayWarning("Only mesh objects are supported for ." + meshName);
        return isSet;
    }

    MFnMesh secondaryFn(dagPath, &status);
    if (status != MStatus::kSuccess)
        return isSet;
    status = secondaryFn.getPoints(points, MSpace::kWorld);
    if (status != MStatus::kSuccess)
        return isSet;
    matrix = dagPath.inclusiveMatrix();

    isSet = true;

    return isSet;
}


//
// Description:
//      Create a copy of the sculpt mesh and define it as the slide
//      mesh for sliding and smoothing operations.
//
// Input Arguments:
//      event               The mouse event.
//      isSet               True, if setting the mesh was successful.
//
// Return Value:
//      MStatus             If setting the mesh was succeesful.
//
MStatus SHAPESBrushContext::setSlideMesh(MEvent event,
                                         bool &isSet)
{
    MStatus status = MStatus::kSuccess;

    isSet = false;

    // When sliding or smoothing create a duplicate of the mesh to
    // project the points on. The duplicated mesh is added as an
    // intermediate object.
    if (event.isModifierControl() || event.isModifierShift())
    {
        MDagPath transform(meshDag);
        transform.pop();

        slideMeshObj = slideMeshFn.copy(meshDag.node(), transform.node(), &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        MFnDependencyNode slideFnDependNode(slideMeshObj);
        MPlug intermediatePlug = slideFnDependNode.findPlug("intermediateObject", false);
        intermediatePlug.setValue(true);

        // Even though slideMeshFn is used to create the copy of the
        // mesh it cannot be used directly for the closest intersection
        // calculation, because it doesn't produce any intersection.
        // The reason for this is unknown.
        // Instead get the name of the shape node and update the
        // slideMeshFn.

        MSelectionList sel;
        sel.add(slideFnDependNode.name());
        MDagPath dagPath;
        status = sel.getDagPath(0, dagPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = slideMeshFn.setObject(dagPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        slideMeshAccelParams = slideMeshFn.autoUniformGridParams();

        isSet = true;
    }

    return status;
}


//
// Description:
//      Calculate the average edge length of the mesh for automatically
//      setting the tolerance value for finding symmetry vertices.
//      In order to speed up the process not every edge is take into
//      account but rather every n-th edge. By default the step size is
//      set to 5. This should be enough to calculate an estimate which
//      should be more accurate than defining a fixed value manually.
//
// Input Arguments:
//      stepSize            The number of edges to be skipped during the
//                          iteration.
//
// Return Value:
//      double              The average edge length of every n-th edge.
//
double SHAPESBrushContext::averageEdgeLength(unsigned int stepSize)
{
    unsigned int i;

    MItMeshEdge edgeIter(meshDag);
    MFnMesh meshFn(meshDag);
    int numEdges = meshFn.numEdges();

    int counter = 0;
    double length = 0.0;

    for (i = 0; i < unsigned(numEdges); i += stepSize)
    {
        int prevIndex;
        edgeIter.setIndex(int(i), prevIndex);

        MPoint p1 = meshPoints[unsigned(edgeIter.index(0))];
        p1 *= meshMat.inverse();
        MPoint p2 = meshPoints[unsigned(edgeIter.index(1))];
        p2 *= meshMat.inverse();
        length += fabs(MVector(p1 - p2).length());

        counter ++;
    }
    double average = length / counter;

    return average;
}


// ---------------------------------------------------------------------
//
// get the closest point on mesh
//
// ---------------------------------------------------------------------

//
// Description:
//      Return if the given event is valid by querying the mouse button
//      and testing the returned MStatus.
//
// Input Arguments:
//      event               The MEvent to test.
//
// Return Value:
//      bool                True, if the event is valid.
//
bool SHAPESBrushContext::eventIsValid(MEvent event)
{
    MStatus status;
    event.mouseButton(&status);
    if (!status)
        return false;
    return true;
}


//
// Description:
//      Get the closest mesh vertex indices at the cursor position.
//      Depending the brush depth setting one or more vertices and their
//      distance to the click point are returned.
//      This method also sets the surfaceDistance which is needed to
//      define the speed for the brush adjustment.
//
// Input Arguments:
//      event               The mouse event.
//      indices             The list of vertex indices along the
//                          intersection ray.
//      distances           The list of distances of the vertices to the
//                          intersection ray.
//
// Return Value:
//      MStatus             kSuccess if intersections been found,
//                          kNotFound if not.
//
MStatus SHAPESBrushContext::getIntersectionIndices(MEvent event,
                                                   MIntArray &indices,
                                                   MFloatArray &distances)
{
    MStatus status = MStatus::kNotFound;

    unsigned int i, j;

    event.getPosition(screenX, screenY);
    view.viewToWorld(screenX, screenY, worldPoint, worldVector);

    // Get the camera near clip and matrix because the world point of
    // the current view is positioned on the near clip plane. As a
    // result changing the near clipping of the camera influences any
    // world point related operations such as drag distances and
    // surface distances.
    double farClip;
    MMatrix camMat;
    getCameraClip(nearClip, farClip, camMat);
    // Create a clipping point which is used to modify view-based world
    // points during the intersection ray check and the drag
    // computation to compensate any changes to the near clip value.
    nearClipPoint = MPoint(0, 0, nearClip);
    nearClipPoint *= camMat;

    viewPlane = getViewPlane(worldVector);

    // Note: MMeshIsectAccelParams is not used on purpose to speed up
    // the search for intersections. In particular cases (i.e. bend
    // elbow) the acceleration sometimes fails to detect the foremost
    // intersection and instead returns the surface behind it as the
    // first intersection causing the edit to appear on the back side of
    // the mesh. Therefore the acceleration has been disabled.

    MFloatPointArray hitPoints;
    MFloatArray hitRayParams;
    MIntArray hitFaces;

    bool foundIntersect = meshFn.allIntersections(worldPoint,
                                                  worldVector,
                                                  NULL,
                                                  NULL,
                                                  true,
                                                  MSpace::kWorld,
                                                  1000000,
                                                  false,
                                                  NULL,
                                                  true,
                                                  hitPoints,
                                                  &hitRayParams,
                                                  &hitFaces,
                                                  NULL,
                                                  NULL,
                                                  NULL);

    if (!foundIntersect)
        return status;

    MItMeshPolygon polyIter(meshFn.object());
    surfacePoints.clear();
    faceVerts.clear();
    faceVertsCount.clear();
    faceVertsWeights.clear();

    // Make sure that the depth value does not go below 0.
    if (depthVal < 1)
        depthVal = 1;

    // Make sure that the depth start value does not go below 0.
    if (depthStartVal < 1)
        depthStartVal = 1;

    // Define how many hit points need to be evaluated depending on the
    // depth setting of the brush.
    unsigned int numHits = hitRayParams.length();
    unsigned int maxDepth = (unsigned)depthVal + (unsigned)depthStartVal - 1;
    if (numHits > maxDepth)
        numHits = maxDepth;

    // In volume mode only the first hit is required to make sure that
    // the smoothing loop only runs once.
    if (volumeVal)
        numHits = 1;

    // Define the start depth value based on the tool settings and the
    // number of hits.
    unsigned int startIndex = (unsigned)depthStartVal - 1;
    if (startIndex >= numHits)
        startIndex = numHits - 1;

    // Store the closest distance to the mesh for the adjustment speed.
    // Take the near clip value of the camera into account in case it
    // has been adjusted.
    surfaceDistance = hitRayParams[startIndex] + nearClip;

    for (i = startIndex; i < numHits; i ++)
    {
        // If an intersection has been found go through the vertices of
        // the intersected polygon and find the closest vertex.

        int prevIndex;
        polyIter.setIndex(hitFaces[i], prevIndex);
        MIntArray vertices;
        polyIter.getVertices(vertices);

        int intersectIndex = 0;
        float intersectDistance = 0.0;

        // Store the number of vertices per intersected polygon as a
        // guide to access vertices and weights per depth step.
        faceVertsCount.append(int(vertices.length()));

        for (j = 0; j < vertices.length(); j ++)
        {
            MPoint position = meshPoints[unsigned(vertices[j])];

            // Convert the vertex position point because the hit points
            // are float points.
            MFloatPoint posPoint;
            posPoint.setCast(position);
            float delta = (float)MVector(posPoint - hitPoints[i]).length();
            // Find which index is closest and store it along with the
            // distance.
            if (j == 0 || intersectDistance > delta)
            {
                intersectIndex = vertices[j];
                intersectDistance = delta;
            }

            // Store the weights for the face vertices depending on the
            // hit point to make sure that the center vertex for the
            // soft selection still receives the correct weight if the
            // hit point is not directly on the vertex.
            float weight = delta / (float)sizeVal;
            if (weight < 0)
                weight = 0;
            else if (weight > 1)
                weight = 1;
            faceVerts.append(vertices[j]);
            faceVertsWeights.append(1.0f - (float)weight);
        }

        surfacePoints.append(hitPoints[i]);
        indices.append(intersectIndex);
        distances.append(intersectDistance);
        status = MStatus::kSuccess;

        // In case of enabled symmetry move the vertex with the given
        // index to the absolute line of symmetry if it has a slight
        // offset. The reason for this is that Maya symmetry seems to
        // miss certain vertices with slight offsets if the center
        // vertex is not at perfect 0 and therefore aren't affected at
        // all.
        MPoint centerPoint = meshPoints[unsigned(intersectIndex)];
        if (adjustSymmetryPoint(centerPoint))
            meshFn.setPoint(intersectIndex, centerPoint, MSpace::kWorld);
    }

    if (indices.length())
    {
        // Calculate a factor to correctly assign the cursor movement to
        // the mesh. The actual cursor drag point appears at a different
        // depth than the target surface.
        screenWeight = surfaceDistance * 0.05;
    }

    return status;
}


//
// Description:
//      Get the camera of the current 3dview.
//
// Input Arguments:
//      nearClip            The near clip value of the current camera.
//      farClip             The far clip value of the current camera.
//      camMat              The inclusive MMatrix of the camera.
//
// Return Value:
//      MStatus             kSuccess if the camera has been found.
//
MStatus SHAPESBrushContext::getCameraClip(double &nearClip,
                                          double &farClip,
                                          MMatrix &camMat)
{
    MStatus status = MStatus::kSuccess;

    MDagPath camDag;
    status = view.getCamera(camDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MFnCamera camFn(camDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MPlug nearClipPlug = camFn.findPlug("nearClipPlane", false);
    nearClipPlug.getValue(nearClip);
    MPlug farClipPlug = camFn.findPlug("farClipPlane", false);
    farClipPlug.getValue(farClip);
    camMat = camDag.inclusiveMatrix();

    return status;
}


//
// Description:
//      Return on which axis plane the edit should occur.
//
// Input Arguments:
//      viewVector          The vector of the camera view.
//
// Return Value:
//      int                 The index of the axis plane.
//
unsigned int SHAPESBrushContext::getViewPlane(MVector viewVector)
{
    if (!usePlaneVal)
        return PLANE_OFF;

    double xDeg = viewVector.angle(MFloatVector(1.0, 0.0, 0.0)) * RADTODEG;
    double yDeg = viewVector.angle(MFloatVector(0.0, 1.0, 0.0)) * RADTODEG;
    double zDeg = viewVector.angle(MFloatVector(0.0, 0.0, 1.0)) * RADTODEG;

    if (abs(xDeg) < planeAngleVal || abs(xDeg) > 180.0 - planeAngleVal)
        return YZ_PLANE;
    else if (abs(yDeg) < planeAngleVal || abs(yDeg) > 180.0 - planeAngleVal)
        return XZ_PLANE;
    else if (abs(zDeg) < planeAngleVal || abs(zDeg) > 180.0 - planeAngleVal)
        return XY_PLANE;

    return PLANE_OFF;
}


//
// Description:
//      Adjust the given point position to match the line of symmetry
//      in case there is a slight offset and the Maya symmetry isn't
//      able to find it.
//
// Input Arguments:
//      point               The MPoint to adjust.
//
// Return Value:
//      bool                True, if the point has been modified.
//
bool SHAPESBrushContext::adjustSymmetryPoint(MPoint &point)
{
    bool setPoint = false;
    if (symmetryVal)
    {
        point *= meshMat.inverse();
        if (symmetryAxisVal == 0 && fabs(point.x) < symTolerance)
        {
            point.x = 0.0;
            setPoint = true;
        }
        else if (symmetryAxisVal == 1 && fabs(point.y) < symTolerance)
        {
            point.y = 0.0;
            setPoint = true;
        }
        else if (symmetryAxisVal == 2 && fabs(point.y) < symTolerance)
        {
            point.z = 0.0;
            setPoint = true;
        }
        point *= meshMat;
    }
    return setPoint;
}


// ---------------------------------------------------------------------
//
// process the vertices within the brush range
//
// ---------------------------------------------------------------------

void SHAPESBrushContext::getPointsInRange(MEvent event)
{
    MStatus status = MStatus::kSuccess;

    unsigned int i, j;

    // With the symmetry introduced in Maya 2015 the selection and it's
    // symmetry components cannot be queried at the same time because
    // for some reasons the selection is returned as symmetry and vice
    // versa.
    // As a workaround first the vertex under the mouse cursor gets
    // selected and Maya returns the symmetry component. This way it's
    // easy to identify the selected from the symmetry vertex.
    // In a second step the soft selection for both sides are queried
    // individually.
    // One drawback to address is that both soft selection arrays can
    // contain components from the opposite side since the Maya symmetry
    // hasn't been used. To clean up the arrays the vertex position gets
    // compared and shared indices are being removed.

    // TODO:
    // Currently the brush only works with a depth of 1.
    int index = intersectionIndices[0];

    // -----------------------------------------------------------------
    // Step 1:
    // Get the symmetry component for the vertex at the mouse position.
    // -----------------------------------------------------------------
    setSoftSelection(true, false, symmetryVal);

    selectComponent(index);
    int indexSym;
    bool hasIndexSym = false;
    status = getSymmetryVertex(index, indexSym);
    if (status == MStatus::kSuccess)
        hasIndexSym = true;
    // Reset the previously stored settings.
    setSoftSelection(false, false, false);

    // -----------------------------------------------------------------
    // Step 2:
    // Get the soft selection for both sides.
    // -----------------------------------------------------------------
    setSoftSelection(true, true, false);

    selectComponent(index);

    MRichSelection softSel;
    MGlobal::getRichSelection(softSel);
    MSelectionList sel;
    MSelectionList selSym;
    softSel.getSelection(sel);

    if (hasIndexSym)
    {
        selectComponent(indexSym);
        MGlobal::getRichSelection(softSel);
        softSel.getSelection(selSym);
    }
    // Reset the previously stored settings.
    setSoftSelection(false, false, false);

    // -----------------------------------------------------------------
    // Step 3:
    // Process all components.
    // -----------------------------------------------------------------
    setSoftSelection(true, true, symmetryVal);

    selectComponent(index);

    MObject vertexObj;
    MObject vertexSymObj;
    MDagPath dagPath;
    MDagPath dagPathSym;

    MFnSingleIndexedComponent vertexFn(vertexObj);
    MFnSingleIndexedComponent vertexSymFn(vertexSymObj);

    sel.getDagPath(0, dagPath, vertexObj);
    vertexFn.setObject(vertexObj);
    selSym.getDagPath(0, dagPathSym, vertexSymObj);
    vertexSymFn.setObject(vertexSymObj);

    // Copy the symmetry ids to a new array to be able modify the list
    // when looking for symmetry components. Found components will get
    // removed from the list which may speed up the process during
    // iteration.
    symIds.clear();
    vertexSymFn.getElements(symIds);

    MGlobal::executeCommand("softSelect -softSelectEnabled 0");

    // prepare for checking frozen areas
    MColorArray vtxColors;
    bool frozen = isFrozen(vtxColors);

    indices.clear();
    indicesSym.clear();
    weights.clear();

    int sideMult = sideMultiplier(index);

    float colorVal = fabs(invertFreezeVal - 1.0f);

    // -----------------------------------------------------------------
    // Process which vertices are affected.
    // -----------------------------------------------------------------
    MItMeshVertex vertIter(meshDag);
    int prevIndex;
    for (i = 0; i < (unsigned)vertexFn.elementCount(); i ++)
    {
        int vtxId = vertexFn.element((int)i);

        vertIter.setIndex(vtxId, prevIndex);

        float vtxColorB = vtxColors[(unsigned)vtxId].b;
        bool freezeVtx = isFreezeColor(vtxColors[(unsigned)vtxId]);

        // Only include boundary vertices if using the grab brush.
        if ((event.isModifierControl() || event.isModifierShift()) &&
            vertIter.onBoundary())
            ;
        // Skip any frozen vertices.
        else if (frozen &&
                 ((freezeVtx && !invertFreezeVal && vtxColorB == colorVal) ||
                 (!freezeVtx && invertFreezeVal && vtxColorB == colorVal)))
            ;
        else
        {
            MPoint pnt = meshPoints[(unsigned)vtxId] * meshMat.inverse();

            // If there is no component selection add the current soft
            // selection vertex to the id list.
            //
            // Keep track of which indices have been added. If there is
            // a component selection it's necessary to compare the
            // current selection to the soft selection list. If a vertex
            // is not in both lists it's not added and the weights won't
            // need to get applied.
            bool addedVert = false;
            if (!vtxSelection.length())
            {
                if (!symmetryVal)
                {
                    indices.append(vtxId);
                    addedVert = true;
                }
                else
                {
                    // In case of enabled symmetry check if the vertex
                    // is on the correct side and remove it from the
                    // symmetry side.
                    // If the mouse pointer is close to the line of
                    // symmetry the two soft selections overlap and
                    // vertices are contained in both lists which need
                    // to be avoided.
                    if ((symmetryAxisVal == 0 && pnt.x * sideMult > symTolerance * -1) ||
                        (symmetryAxisVal == 1 && pnt.y * sideMult > symTolerance * -1) ||
                        (symmetryAxisVal == 2 && pnt.z * sideMult > symTolerance * -1))
                    {
                        indices.append(vtxId);
                        addedVert = true;

                        // Remove the vertex from the symmetry list.
                        for (j = 0; j < symIds.length(); j ++)
                        {
                            if (symIds[j] == vtxId)
                            {
                                symIds.remove(j);
                                break;
                            }
                        }
                    }
                }
            }
            // Only add vertices which are part of the component
            // selection.
            else
            {
                for (j = 0; j < vtxSelection.length(); j ++)
                {
                    if (vtxSelection[j] == vtxId)
                    {
                        indices.append(vtxId);
                        addedVert = true;
                    }
                }
            }

            if (addedVert)
            {
                if (symmetryVal)
                {
                    int vtxSymId = -1;

                    // Find the symmetry vertex by iterating through the
                    // symmetry indices and matching vertex positions.
                    // In case a matching position cannot be found the
                    // default Maya symmetry is used to query the
                    // symmetry vertex. This is not ideal but fast
                    // enough to be a valid approach. This way ensures
                    // that every symmetry vertex can be found.

                    if (symmetryAxisVal == 0)
                        pnt.x *= -1;
                    else if (symmetryAxisVal == 1)
                        pnt.y *= -1;
                    else if (symmetryAxisVal == 2)
                        pnt.z *= -1;

                    //
                    // Matching pass 1
                    //

                    // Every time a symmetry point is found it's removed
                    // from the list. This way the query gets shorter
                    // with every symmetry point.
                    status = matchSymmetryVertexByPosition(pnt, symTolerance, vtxSymId);

                    if (status == MStatus::kNotFound)
                    {
                        //
                        // Matching pass 2
                        //

                        // If the symmetry vertex is not found get the
                        // connected vertices and find the closest.
                        // Use this distance as a new tolerance value
                        // when searching for the symmetry id again.
                        double distance = getClosestConnectedVertexDistance((unsigned)vtxId);
                        status = matchSymmetryVertexByPosition(pnt, distance * 0.5, vtxSymId);

                        if (status == MStatus::kNotFound)
                        {
                            //
                            // Matching pass 3
                            //

                            // If everything has failed try the regular
                            // Maya symmetry.
                            selectComponent(vtxId);
                            getSymmetryVertex(vtxId, vtxSymId);
                            // In case all matching attempts failed the
                            // index of -1 will be added to the list.
                        }
                    }
                    indicesSym.append(vtxSymId);
                }

                // Get the color value to create a color weight factor.
                float colorWeight = 1.0;
                if (frozen && freezeVtx)
                {
                    if (!invertFreezeVal)
                        colorWeight = 1 - vtxColorB;
                    else
                        colorWeight = vtxColorB;
                }

                // Filter the points which belong to the face that got
                // hit by the eye ray in case the current index is not
                // one of the hit face points. In this case get the
                // weight from the soft selection.
                bool isFaceVtx = false;
                for (j = 0; j < unsigned(faceVertsCount[0]); j ++)
                {
                    if (faceVerts[j] == vtxId)
                    {
                        weights.append(faceVertsWeights[j] * colorWeight);
                        isFaceVtx = true;
                    }
                }
                if (!isFaceVtx)
                    weights.append(vertexFn.weight((int)i).influence() * colorWeight);
            }
        }
    }

    // Reset the previously stored settings.
    setSoftSelection(false, false, false);
}


//
// Description:
//      Set the symmetry and soft selection when getting the affected
//      vertices.
//
// Input Arguments:
//      mode                True, if the soft selected should be set.
//                          False to reset to the previous state.
//      soft                True, if soft selection should be enabled.
//                          This is set to false when symmetry vertices
//                          are queried.
//      sym                 True, if symmetry should be enabled.
//
void SHAPESBrushContext::setSoftSelection(bool mode,
                                          bool soft,
                                          bool sym)
{
    if (mode)
    {
        // Get the current selection and command values to be able to
        // reset them later.
        MGlobal::getActiveSelectionList(previousSelection);
        MGlobal::executeCommand("symmetricModelling -query -symmetry", previousSymmetry);
        previousAxis = MGlobal::executeCommandStringResult("symmetricModelling -query -axis");
        MGlobal::executeCommand("softSelect -query -softSelectEnabled", previousSoftSelect);
        MGlobal::executeCommand("softSelect -query -softSelectFalloff", previousFalloff);
        MGlobal::executeCommand("softSelect -query -softSelectDistance", previousDistance);
        previousFalloffCurve = MGlobal::executeCommandStringResult("softSelect -query -softSelectCurve");

        MString axis = "x";
        int symmetry = 0;
        if (sym)
        {
            if (symmetryVal)
            {
                if (symmetryAxisVal == 1)
                    axis = "y";
                else if (symmetryAxisVal == 2)
                    axis = "z";
                symmetry = 1;
            }
        }

        // Set the symmetry and soft selection attributes. The curve is
        // linear.
        MString cmd;
        cmd = "symmetricModelling -edit -symmetry ";
        cmd += symmetry;
        cmd += " -axis ";
        cmd += axis;
        cmd += ";softSelect -softSelectEnabled ";
        cmd += soft;
        cmd += " -softSelectFalloff ";
        cmd += (1-volumeVal);
        cmd += " -softSelectDistance ";
        cmd += sizeVal;
        cmd += " -softSelectCurve \"0,1,0,1,0,1,0,1,1\";";
        MGlobal::executeCommand(cmd);
    }
    else
    {
        // Reset the previously stored settings.
        MString cmd;
        cmd = "symmetricModelling -edit -symmetry ";
        cmd += previousSymmetry;
        cmd += " -axis ";
        cmd += previousAxis;
        cmd += ";softSelect -softSelectEnabled ";
        cmd += previousSoftSelect;
        cmd += " -softSelectFalloff ";
        cmd += previousFalloff;
        cmd += " -softSelectDistance ";
        cmd += previousDistance;
        cmd += " -softSelectCurve \"";
        cmd += previousFalloffCurve;
        cmd += "\";";
        MGlobal::executeCommand(cmd);
        MGlobal::setActiveSelectionList(previousSelection);
    }
}


//
// Description:
//      Set the given vertex component and it's symmetry component.
//      There seems to be no documentation on how to select a symmetry
//      component through the API therefore the default select command
//      is used.
//
// Input Arguments:
//      index               The index of the vertex to select.
//
void SHAPESBrushContext::selectComponent(int index)
{
    MString cmd;
    cmd = "select -replace -symmetry ";
    cmd += meshDag.fullPathName();
    cmd += ".vtx[";
    cmd += index;
    cmd += "]";
    MGlobal::executeCommand(cmd);
}


//
// Description:
//      Return the symmetry index of the given vertex index.
//
// Input Arguments:
//      index               The index of the vertex to get the symmetry
//                          vertex from.
//      indexSym            The symmetry index.
//
// Return Value:
//      MStatus             kSuccess if the vertex has been found,
//                          kNotFound if not.
//
MStatus SHAPESBrushContext::getSymmetryVertex(int index,
                                              int &indexSym)
{
    MStatus status = MStatus::kNotFound;

    MRichSelection softSel;
    MGlobal::getRichSelection(softSel);
    MSelectionList sel;
    softSel.getSelection(sel);
    MSelectionList selSym;
    softSel.getSymmetry(selSym);

    MDagPath dagPath;
    MObject vertexObj;
    sel.getDagPath(0, dagPath, vertexObj);
    MFnSingleIndexedComponent vertexFn(vertexObj);

    MDagPath dagPathSym;
    MObject vertexSymObj;
    selSym.getDagPath(0, dagPathSym, vertexSymObj);
    MFnSingleIndexedComponent vertexSymFn(vertexSymObj);

    // Check which component returned is the symmetry one.
    if (vertexFn.elementCount() && vertexFn.element(0) != index)
    {
        indexSym = vertexFn.element(0);
        status = MStatus::kSuccess;
    }
    else if (vertexSymFn.elementCount() && vertexSymFn.element(0) != index)
    {
        indexSym = vertexSymFn.element(0);
        status = MStatus::kSuccess;
    }

    return status;
}


//
// Description:
//      Return if the mesh contains frozen areas.
//
// Input Arguments:
//      vtxColors           The vertex color array for the frozen areas.
//
// Return Value:
//      bool                True, if the mesh is frozen.
//
bool SHAPESBrushContext::isFrozen(MColorArray &vtxColors)
{
    MStatus status = MStatus::kSuccess;

    unsigned int i;

    bool result = false;

    vtxColors = MColorArray(meshPoints.length(), MColor(0.0, 0.0, 0.0));
    unsigned int numSets = (unsigned)meshFn.numColorSets(&status);
    if (status == MStatus::kSuccess && numSets > 0)
    {
        bool exists = false;
        MString currentSetName = meshFn.currentColorSetName();
        if (currentSetName != FREEZE_SET)
        {
            MStringArray allColorSets;
            meshFn.getColorSetNames(allColorSets);
            if (allColorSets.length())
            {
                for (i = 0; i < allColorSets.length(); i ++)
                {
                    if (allColorSets[i] == FREEZE_SET)
                        exists = true;
                }
            }
        }
        else
            exists = true;

        if (exists)
        {
            meshFn.setCurrentColorSetName(FREEZE_SET);
            result = true;
            meshFn.getVertexColors(vtxColors);
        }
    }

    return result;
}


//
// Description:
//      Return a side multiplier depending on which side of the mesh the
//      the origin of the brush action lies.
//
// Input Arguments:
//      index               The index of the vertex under the mouse
//                          cursor.
//
// Return Value:
//      int                 The multiplier value.
//
int SHAPESBrushContext::sideMultiplier(int index)
{
    int multiplier = 1;
    MPoint point;
    meshFn.getPoint(index, point, MSpace::kWorld);
    point *= meshMat.inverse();
    if ((symmetryAxisVal == 0 && point.x < 0) ||
        (symmetryAxisVal == 1 && point.y < 0) ||
        (symmetryAxisVal == 2 && point.z < 0))
        multiplier = -1;
    return multiplier;
}


//
// Description:
//      Return if the given color matches the freeze color.
//
// Input Arguments:
//      color               The MColor to test.
//
// Return Value:
//      bool                True, if the color matches the freeze color.
//
bool SHAPESBrushContext::isFreezeColor(MColor color)
{
    bool isColor = false;
    float factor = 1.0f / color.b;
    if (fabs(color.r * factor - 0.4f) < 0.001 &&
        fabs(color.g * factor - 0.7f) < 0.001 &&
        fabs(color.b * factor - 1.0f) < 0.001)
        isColor = true;
    return isColor;
}


//
// Description:
//      Go through the array of temporary symmetry indices and check
//      which one matches the given point position. The found index gets
//      added to the final array of symmetrical indices and removed from
//      the temporary list.
//
// Input Arguments:
//      point               The MPoint to test the vertex position
//                          against.
//      tolerance           The tolerance value for the position match.
//      symmetryIndex       The index of the found symmetry vertex.
//
// Return Value:
//      MStatus             kSuccess if the vertex has been found,
//                          kNotFound if not.
//
MStatus SHAPESBrushContext::matchSymmetryVertexByPosition(MPoint point,
                                                          double tolerance,
                                                          int &symmetryIndex)
{
    MStatus status = MStatus::kNotFound;

    unsigned int i;

    for (i = 0; i < symIds.length(); i ++)
    {
        MPoint symPt = meshPoints[(unsigned)symIds[i]];
        symPt *= meshMat.inverse();
        if (symPt.isEquivalent(point, tolerance))
        {
            symmetryIndex = symIds[i];
            symIds.remove(i);
            status = MStatus::kSuccess;
            break;
        }
    }
    return status;
}


//
// Description:
//      Return the closest distance between the given vertex index and
//      it's connected vertices.
//
// Input Arguments:
//      index               The vertex index to get the closest distance
//                          from.
//
// Return Value:
//      double              The clostest distance.
//
double SHAPESBrushContext::getClosestConnectedVertexDistance(unsigned int index)
{
    unsigned int i;

    MItMeshVertex vertIter(meshDag);
    int prevIndex;

    vertIter.setIndex((int)index, prevIndex);

    MIntArray connectedVerts;
    vertIter.getConnectedVertices(connectedVerts);

    MPoint point(meshPoints[index]);
    MPoint localPoint = point * meshMat.inverse();

    double distance = 0.0;

    for (i = 0; i < connectedVerts.length(); i ++)
    {
        MPoint nextPoint(meshPoints[(unsigned)connectedVerts[i]]);
        MPoint localNextPoint = nextPoint * meshMat.inverse();
        MVector delta(localNextPoint - localPoint);
        if (distance == 0.0 || delta.length() < distance)
            distance = delta.length();
    }
    return distance;
}


// ---------------------------------------------------------------------
//
// brush execution
//
// ---------------------------------------------------------------------

//
// Description:
//      Collect all data about the current drag which is used to
//      determine the parameters for the edit. After performing the
//      computation the returned point data is used to deform the mesh.
//
// Input Arguments:
//      event               The mouse event.
//
void SHAPESBrushContext::editMesh(MEvent event)
{
    short dragX;
    short dragY;
    event.getPosition(dragX, dragY);

    view.viewToWorld(dragX, dragY, worldDragPoint, worldDragVector);

    MVector delta(MVector(worldDragPoint + nearClipPoint) - (worldPoint + nearClipPoint));
    // The drag delta value depends on the near clipping of the camera.
    delta *= 20 / nearClip;

    // Set the flag that the mesh is being edited to prevent that the
    // release of the mouse re-applies the last mesh changes.
    if (delta.length() > 0)
        performBrush = true;
    else
        return;

    // Calculate the drag distance for the pull/push brush, which also
    // depends on the distance to the camera.
    short dragDistance = dragX - screenX;

    MPointArray newPoints;
    // When smoothing get the already modified point position of the
    // mesh to increase the smoothing effect.
    if (event.isModifierShift() ||
        (event.isModifierControl() && typeVal == 1) ||
        (event.isModifierControl() && typeVal == 3) ||
        (event.isModifierControl() && typeVal == 4))
        meshFn.getPoints(newPoints, MSpace::kWorld);
    // Copy the original mesh points, so that each drag calculation
    // starts with the default point position.
    else
        newPoints = meshPoints;

    newPoints = computeEdit(newPoints, delta, dragDistance, event);

    // Set the points for the interactive preview.
    meshFn.setPoints(newPoints, MSpace::kWorld);
    meshDeformedPoints = newPoints;

    view.refresh(true);
}


//
// Description:
//      Modify the mesh point data based on the mouse drag and return
//      the deformed points.
//
// Input Arguments:
//      newPoints           The mesh points as a copied array.
//      delta               The MVector of the current mouse drag as a
//                          world space vector.
//      dragDistance        The distance of the current drag in screen
//                          space pixels.
//      event               The mouse event.
//
// Return Value:
//      double              The clostest distance.
//
MPointArray SHAPESBrushContext::computeEdit(MPointArray newPoints,
                                            MVector delta,
                                            short dragDistance,
                                            MEvent event)
{
    MStatus status = MStatus::kSuccess;

    unsigned int i;

    // Calculate the drag distance for the pull/push brush, which also
    // depends on the distance to the camera.
    int direction = 1;
    if (dragDistance < pushPullDistance)
        direction = -1;
    float factor = float(surfaceDistance * 0.005 * direction);
    if (invertPullVal)
        factor *= -1;
    pushPullDistance = dragDistance;

    // Twist brush:
    // Get the position from the vertex under the mouse and create an
    // offset matrix for the twist around this point.
    MPoint axisPoint;
    meshFn.getPoint(intersectionIndices[0], axisPoint, MSpace::kObject);
    MTransformationMatrix offsetTransMat;
    offsetTransMat.setTranslation(axisPoint, MSpace::kObject);
    MMatrix offsetMat = offsetTransMat.asMatrix();

    MItMeshVertex vertIter(meshDag);
    int prevIndex;

    for (i = 0; i < indices.length(); i ++)
    {
        int index = indices[i];
        int indexSym = -1;

        if (index != -1)
        {
            if (symmetryVal)
                indexSym = indicesSym[i];

            MPoint point(newPoints[(unsigned)index]);
            MPoint localPoint = point * meshMat.inverse();

            vertIter.setIndex(index, prevIndex);

            // Define the brush falloff.
            double brushWeight = getFalloffWeight(weights[i]);

            // ---------------------------------------------------------
            // brush types
            // ---------------------------------------------------------

            if (event.isModifierNone() || (event.isModifierControl() && !event.isModifierShift()))
            {
                // -----------------------------------------------------
                // grab and slide brush
                // -----------------------------------------------------
                if (event.isModifierNone() || (event.isModifierControl() && typeVal == 0))
                {
                    point += delta * screenWeight * brushWeight;
                }
                // -----------------------------------------------------
                // pull/push brush
                // -----------------------------------------------------
                else if (event.isModifierControl() && typeVal == 1)
                {
                    point += meshNormals[(unsigned)index] * ((float)strengthVal * factor) * (float)brushWeight;
                }
                // -----------------------------------------------------
                // twist brush
                // -----------------------------------------------------
                else if (event.isModifierControl() && typeVal == 2)
                {
                    point *= meshMat.inverse() * offsetMat.inverse();
                    MVector rotateVec = worldVector;
                    rotateVec.normalize();
                    MQuaternion quat;
                    quat.setAxisAngle(rotateVec, strengthVal * 0.01 * dragDistance * brushWeight);
                    MMatrix rotateMat = quat.asMatrix();
                    point *= rotateMat * offsetMat * meshMat;
                }
                // -----------------------------------------------------
                // blend brush
                // -----------------------------------------------------
                else if (event.isModifierControl() && typeVal == 3 && blendMeshIsSet)
                {
                    point *= meshMat.inverse();
                    MPoint cachePoint = blendMeshPoints[(unsigned)index] * blendMeshMat.inverse();
                    MVector blendVec(cachePoint - point);
                    point += blendVec * brushWeight * (strengthVal / 5);
                    point *= meshMat;
                }
                // -----------------------------------------------------
                // erase brush
                // -----------------------------------------------------
                else if (event.isModifierControl() && typeVal == 4)
                {
                    MVector eraseVec(eraseMeshPoints[(unsigned)index] - point);
                    point += eraseVec * brushWeight * (strengthVal / 5);
                }

                // -----------------------------------------------------
                // slide brush
                // -----------------------------------------------------
                if (event.isModifierControl() && typeVal == 0)
                {
                    MPoint surfacePoint;
                    status = getClosestSurfacePoint(point, meshNormals[(unsigned)index], surfacePoint);
                    if (status)
                        point = surfacePoint;
                }

                point = constrainPlanePoint(point, localPoint);

                point = centerSymmetryPoint(point, localPoint);
            }

            // ---------------------------------------------------------
            // smoothing brush
            // ---------------------------------------------------------

            if (event.isModifierShift())
            {
                MIntArray connectedVerts;
                vertIter.getConnectedVertices(connectedVerts);
                MPoint midPoint(averagePoint(connectedVerts, newPoints));

                // Smooth in relation to the smooth base mesh to keep the
                // point spacing.
                if (event.isModifierControl() && smoothMeshIsSet)
                {
                    MPoint midPointSmooth(averagePoint(connectedVerts, smoothMeshPoints));
                    midPoint *= meshMat.inverse();
                    midPointSmooth *= smoothMeshMat.inverse();

                    MPoint midDelta = midPoint - midPointSmooth;
                    midDelta *= meshMat;

                    midPoint = smoothMeshPoints[(unsigned)index] + midDelta;
                }

                MVector smoothVec(midPoint - point);
                point += smoothVec * brushWeight * smoothVal;

                if (relaxVal)
                {
                    MPoint surfacePoint;
                    status = getClosestSurfacePoint(point, meshNormals[(unsigned)index], surfacePoint);
                    if (status)
                        point = surfacePoint;
                }

                point = constrainPlanePoint(point, localPoint);

                point = centerSymmetryPoint(point, localPoint);
            }

            // replace the point position in the array
            newPoints.set(point, (unsigned)index);

            // replace the symmetry point position in the array
            if (symmetryVal && indexSym != -1)
            {
                MPoint localPoint = point * meshMat.inverse();
                if (symmetryAxisVal == 0)
                    localPoint.x *= -1;
                if (symmetryAxisVal == 1)
                    localPoint.y *= -1;
                if (symmetryAxisVal == 2)
                    localPoint.z *= -1;
                point = localPoint * meshMat;
                newPoints.set(point, (unsigned)indexSym);
            }
        }
    }

    return newPoints;
}


//
// Description:
//      Calculate the brush weight value based on the given linear
//      falloff value.
//
// Input Arguments:
//      value               The linear falloff value.
//      strength            The brush strength value.
//
// Return Value:
//      double              The brush curve-based falloff value.
//
double SHAPESBrushContext::getFalloffWeight(double value)
{
    double weight = 0.0;

    // linear
    if (curveVal == 0)
        weight = value;
    // soft - inverse quadratic
    else if (curveVal == 1)
        weight = 1 - pow((1 - value) / 1, 2.0);
    // wide
    else if (curveVal == 2)
        weight = 1 - pow((1 - value) / 1, 5.0);
    // narrow - quadratic
    else if (curveVal == 3)
        weight = 1 - pow((1 - value) / 1, 0.4);
    // smooth1 - smoothstep
    else if (curveVal == 4)
        weight = value * value * (3 - 2 * value);
    // smooth2 - smootherstep
    else if (curveVal == 5)
        weight = value * value * value * (value * (value * 6 - 15) + 10);

    return weight;
}


//
// Description:
//      Constrain the given point to the given reference point along the
//      view plane axis.
//
// Input Arguments:
//      point               The MPoint to constrain.
//      constrainPoint      The MPoint to constrain to.
//
// Return Value:
//      bool                True, if the point has been modified.
//
MPoint SHAPESBrushContext::constrainPlanePoint(MPoint point,
                                               MPoint constrainPoint)
{
    if (viewPlane != PLANE_OFF && (typeVal >= 0 && typeVal <= 2))
    {
        point *= meshMat.inverse();
        if (viewPlane == YZ_PLANE)
            point.x = constrainPoint.x;
        else if (viewPlane == XZ_PLANE)
            point.y = constrainPoint.y;
        else if (viewPlane == XY_PLANE)
            point.z = constrainPoint.z;
        point *= meshMat;
    }

    return point;
}


//
// Description:
//      Find the closest point to the given point and return it.
//
// Input Arguments:
//      point               The MPoint to find the closest point to.
//      normal              The vertex normal.
//      closestPoint        The closest point found.
//
// Return Value:
//      MStatus             kSuccess if the point has been found,
//                          kNotFound if not.
//
MStatus SHAPESBrushContext::getClosestSurfacePoint(MPoint point,
                                                   MFloatVector normal,
                                                   MPoint &closestPoint)
{
    MStatus status = MStatus::kNotFound;

    MFloatPoint raySrc(point);
    MFloatPoint hitPoint;
    int hitFace;

    bool foundClosest = slideMeshFn.closestIntersection(raySrc,
                                                        normal,
                                                        NULL,
                                                        NULL,
                                                        true,
                                                        MSpace::kWorld,
                                                        1000000,
                                                        true,
                                                        &slideMeshAccelParams,
                                                        hitPoint,
                                                        NULL,
                                                        &hitFace,
                                                        NULL,
                                                        NULL,
                                                        NULL);
    if (foundClosest && hitPoint != point)
    {
        status = MStatus::kSuccess;
        closestPoint = hitPoint;
    }

    return status;
}


//
// Description:
//      Return an averaged point from the list of given points.
//
// Input Arguments:
//      vtxArray            The array of point indices to average.
//      points              The array of points.
//
// Return Value:
//      MPoint              The averaged point.
//
MPoint SHAPESBrushContext::averagePoint(MIntArray vtxArray,
                                        MPointArray points)
{
    unsigned int i;

    MPoint averagePoint(0.0, 0.0, 0.0);
    for (i = 0; i < vtxArray.length(); i ++)
    {
        averagePoint.x += points[(unsigned)vtxArray[i]].x;
        averagePoint.y += points[(unsigned)vtxArray[i]].y;
        averagePoint.z += points[(unsigned)vtxArray[i]].z;
    }
    averagePoint = averagePoint / vtxArray.length();
    return averagePoint;
}


//
// Description:
//      Compare if the given point is equivalent to the given reference
//      point on the current symmetry axis and adjust the position if
//      necessary. The given point gets modified.
//
// Input Arguments:
//      point               The MPoint to test.
//      referencePoint      The MPoint to compare to.
//
// Return Value:
//      MPoint              The modified point.
//
MPoint SHAPESBrushContext::centerSymmetryPoint(MPoint point,
                                               MPoint referencePoint)
{
    if (symmetryVal)
    {
        point *= meshMat.inverse();
        if (symmetryAxisVal == 0 && referencePoint.isEquivalent(MPoint(0.0,
                                                                       referencePoint.y,
                                                                       referencePoint.z),
                                                                symTolerance))
            point.x = 0.0;
        else if (symmetryAxisVal == 1 && referencePoint.isEquivalent(MPoint(referencePoint.x,
                                                                            0.0,
                                                                            referencePoint.z),
                                                                     symTolerance))
            point.y = 0.0;
        else if (symmetryAxisVal == 2 && referencePoint.isEquivalent(MPoint(referencePoint.x,
                                                                            referencePoint.y,
                                                                            0.0),
                                                                     symTolerance))
            point.z = 0.0;
        point *= meshMat;
    }
    return point;
}


void SHAPESBrushContext::setInViewMessage(bool display)
{
    if (display && messageVal)
        MGlobal::executeCommand("SHAPESBrushShowInViewMessage");
    else
        MGlobal::executeCommand("SHAPESBrushHideInViewMessage");
}


//
// Description:
//      Read the settings from the environment variables.
//
// Input Arguments:
//      None
//
// Return Value:
//      None
//
void SHAPESBrushContext::getEnvironmentSettings()
{
    if (MGlobal::optionVarExists("SHAPESBrushUndersamplingAdjust"))
        varStepsAdjust = (unsigned)MGlobal::optionVarIntValue("SHAPESBrushUndersamplingAdjust");
    else
        varStepsAdjust = UNDERSAMPLING;

    if (MGlobal::optionVarExists("SHAPESBrushUndersamplingPull"))
        varStepsPull = (unsigned)MGlobal::optionVarIntValue("SHAPESBrushUndersamplingPull");
    else
        varStepsPull = UNDERSAMPLING_PULL;
}


// ---------------------------------------------------------------------
// setting values from the command flags
// ---------------------------------------------------------------------

void SHAPESBrushContext::setBlendMesh(MString name)
{
    blendMeshVal = name;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setColorR(float value)
{
    colorVal.r = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setColorG(float value)
{
    colorVal.g = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setColorB(float value)
{
    colorVal.b = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setCurve(int value)
{
    curveVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setDepth(int value)
{
    depthVal = MAX_DEPTH;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setDepthStart(int value)
{
    depthStartVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setDrawBrush(bool value)
{
    drawBrushVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setEnterToolCommand(MString value)
{
    enterToolCommandVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setExitToolCommand(MString value)
{
    exitToolCommandVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setFlushCache(int value)
{
    // Delete the cached points.
    if (value == 1)
    {
        eraseMeshPoints.clear();
    }
    // Reset the mesh.
    else if (value == 2 && eraseMeshPoints.length() != 0)
    {
        meshFn.setPoints(eraseMeshPoints, MSpace::kWorld);
        view.refresh(true);
    }
    flushCacheVal = 0;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setInvertFreeze(bool value)
{
    invertFreezeVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setInvertPull(bool value)
{
    invertPullVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setLineWidth(int value)
{
    lineWidthVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setMessage(bool value)
{
    messageVal = value;
    MToolsInfo::setDirtyFlag(*this);

    setInViewMessage(true);
}


void SHAPESBrushContext::setPlaneAngle(double value)
{
    planeAngleVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setRelax(bool value)
{
    relaxVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setSize(double value)
{
    sizeVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setSmooth(double value)
{
    smoothVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setSmoothMesh(MString name)
{
    smoothMeshVal = name;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setStrength(double value)
{
    strengthVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setSymmetry(bool value)
{
    symmetryVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setSymmetryAxis(int value)
{
    symmetryAxisVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setTolerance(double value)
{
    toleranceVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setType(int value)
{
    typeVal = value;
    MToolsInfo::setDirtyFlag(*this);

    setInViewMessage(true);
}


void SHAPESBrushContext::setUsePlane(bool value)
{
    usePlaneVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


void SHAPESBrushContext::setVolume(bool value)
{
    volumeVal = value;
    MToolsInfo::setDirtyFlag(*this);
}


// ---------------------------------------------------------------------
// getting values from the command flags
// ---------------------------------------------------------------------

MString SHAPESBrushContext::getBlendMesh()
{
    return blendMeshVal;
}


float SHAPESBrushContext::getColorR()
{
    return colorVal.r;
}


float SHAPESBrushContext::getColorG()
{
    return colorVal.g;
}


float SHAPESBrushContext::getColorB()
{
    return colorVal.b;
}


int SHAPESBrushContext::getCurve()
{
    return curveVal;
}


int SHAPESBrushContext::getDepth()
{
    return depthVal;
}


int SHAPESBrushContext::getDepthStart()
{
    return depthStartVal;
}


bool SHAPESBrushContext::getDrawBrush()
{
    return drawBrushVal;
}


MString SHAPESBrushContext::getEnterToolCommand()
{
    return enterToolCommandVal;
}


MString SHAPESBrushContext::getExitToolCommand()
{
    return exitToolCommandVal;
}


int SHAPESBrushContext::getFlushCache()
{
    if (!eraseMeshPoints.length())
        return 1;
    else
        return 0;
}


bool SHAPESBrushContext::getInvertFreeze()
{
    return invertFreezeVal;
}


bool SHAPESBrushContext::getInvertPull()
{
    return invertPullVal;
}


int SHAPESBrushContext::getLineWidth()
{
    return lineWidthVal;
}


bool SHAPESBrushContext::getMessage()
{
    return messageVal;
}


double SHAPESBrushContext::getPlaneAngle()
{
    return planeAngleVal;
}


bool SHAPESBrushContext::getRelax()
{
    return relaxVal;
}


double SHAPESBrushContext::getSize()
{
    return sizeVal;
}


double SHAPESBrushContext::getSmooth()
{
    return smoothVal;
}


MString SHAPESBrushContext::getSmoothMesh()
{
    return smoothMeshVal;
}


double SHAPESBrushContext::getStrength()
{
    return strengthVal;
}


bool SHAPESBrushContext::getSymmetry()
{
    return symmetryVal;
}


int SHAPESBrushContext::getSymmetryAxis()
{
    return symmetryAxisVal;
}


double SHAPESBrushContext::getTolerance()
{
    return toleranceVal;
}


int SHAPESBrushContext::getType()
{
    return typeVal;
}


bool SHAPESBrushContext::getUsePlane()
{
    return usePlaneVal;
}


bool SHAPESBrushContext::getVolume()
{
    return volumeVal;
}


// ---------------------------------------------------------------------
// command to create the context
// ---------------------------------------------------------------------

SHAPESBrushContextCmd::SHAPESBrushContextCmd()
{}


MPxContext* SHAPESBrushContextCmd::makeObj()
{
    brushContext = new SHAPESBrushContext();
    return brushContext;
}


void* SHAPESBrushContextCmd::creator()
{
    return new SHAPESBrushContextCmd();
}


// ---------------------------------------------------------------------
// pointers for the argument flags
// ---------------------------------------------------------------------

MStatus SHAPESBrushContextCmd::appendSyntax()
{
    MSyntax syn = syntax();

    syn.addFlag(kBlendMeshFlag, kBlendMeshFlagLong, MSyntax::kString);
    syn.addFlag(kColorRFlag, kColorRFlagLong, MSyntax::kDouble);
    syn.addFlag(kColorGFlag, kColorGFlagLong, MSyntax::kDouble);
    syn.addFlag(kColorBFlag, kColorBFlagLong, MSyntax::kDouble);
    syn.addFlag(kCurveFlag, kCurveFlagLong, MSyntax::kLong);
    syn.addFlag(kDepthFlag, kDepthFlagLong, MSyntax::kLong);
    syn.addFlag(kDepthStartFlag, kDepthStartFlagLong, MSyntax::kLong);
    syn.addFlag(kDrawBrushFlag, kDrawBrushFlagLong, MSyntax::kBoolean);
    syn.addFlag(kEnterToolCommandFlag, kEnterToolCommandFlagLong, MSyntax::kString);
    syn.addFlag(kExitToolCommandFlag, kExitToolCommandFlagLong, MSyntax::kString);
    syn.addFlag(kFlushFlag, kFlushFlagLong, MSyntax::kLong);
    syn.addFlag(kInvertFreezeFlag, kInvertFreezeFlagLong, MSyntax::kBoolean);
    syn.addFlag(kInvertPullFlag, kInvertPullFlagLong, MSyntax::kBoolean);
    syn.addFlag(kLineWidthFlag, kLineWidthFlagLong, MSyntax::kLong);
    syn.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kBoolean);
    syn.addFlag(kPlaneAngleFlag, kPlaneAngleFlagLong, MSyntax::kDouble);
    syn.addFlag(kRelaxFlag, kRelaxFlagLong, MSyntax::kBoolean);
    syn.addFlag(kSizeFlag, kSizeFlagLong, MSyntax::kDouble);
    syn.addFlag(kSmoothFlag, kSmoothFlagLong, MSyntax::kDouble);
    syn.addFlag(kSmoothMeshFlag, kSmoothMeshFlagLong, MSyntax::kString);
    syn.addFlag(kStrengthFlag, kStrengthFlagLong, MSyntax::kDouble);
    syn.addFlag(kSymmetryFlag, kSymmetryFlagLong, MSyntax::kBoolean);
    syn.addFlag(kSymmetryAxisFlag, kSymmetryAxisFlagLong, MSyntax::kLong);
    syn.addFlag(kToleranceFlag, kToleranceFlagLong, MSyntax::kDouble);
    syn.addFlag(kTypeFlag, kTypeFlagLong, MSyntax::kLong);
    syn.addFlag(kUsePlaneFlag, kUsePlaneFlagLong, MSyntax::kBoolean);
    syn.addFlag(kVolumeFlag, kVolumeFlagLong, MSyntax::kBoolean);

    return MStatus::kSuccess;
}


MStatus SHAPESBrushContextCmd::doEditFlags()
{
    MStatus status = MStatus::kSuccess;

    MArgParser argData = parser();

    if (argData.isFlagSet(kBlendMeshFlag))
    {
        MString value;
        status = argData.getFlagArgument(kBlendMeshFlag, 0, value);
        brushContext->setBlendMesh(value);
    }

    if (argData.isFlagSet(kColorRFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorRFlag, 0, value);
        brushContext->setColorR((float)value);
    }

    if (argData.isFlagSet(kColorGFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorGFlag, 0, value);
        brushContext->setColorG((float)value);
    }

    if (argData.isFlagSet(kColorBFlag))
    {
        double value;
        status = argData.getFlagArgument(kColorBFlag, 0, value);
        brushContext->setColorB((float)value);
    }

    if (argData.isFlagSet(kCurveFlag))
    {
        int value;
        status = argData.getFlagArgument(kCurveFlag, 0, value);
        brushContext->setCurve(value);
    }

    // Disabled because currently only a depth of 1 is supported.
    /*
    if (argData.isFlagSet(kDepthFlag))
    {
        int value;
        status = argData.getFlagArgument(kDepthFlag, 0, value);
        brushContext->setDepth(value);
    }
    */

    if (argData.isFlagSet(kDepthStartFlag))
    {
        int value;
        status = argData.getFlagArgument(kDepthStartFlag, 0, value);
        brushContext->setDepthStart(value);
    }

    if (argData.isFlagSet(kDrawBrushFlag))
    {
        bool value;
        status = argData.getFlagArgument(kDrawBrushFlag, 0, value);
        brushContext->setDrawBrush(value);
    }

    if (argData.isFlagSet(kEnterToolCommandFlag))
    {
        MString value;
        status = argData.getFlagArgument(kEnterToolCommandFlag, 0, value);
        brushContext->setEnterToolCommand(value);
    }

    if (argData.isFlagSet(kExitToolCommandFlag))
    {
        MString value;
        status = argData.getFlagArgument(kExitToolCommandFlag, 0, value);
        brushContext->setExitToolCommand(value);
    }

    if (argData.isFlagSet(kFlushFlag))
    {
        int value;
        status = argData.getFlagArgument(kFlushFlag, 0, value);
        brushContext->setFlushCache(value);
    }

    if (argData.isFlagSet(kInvertFreezeFlag))
    {
        bool value;
        status = argData.getFlagArgument(kInvertFreezeFlag, 0, value);
        brushContext->setInvertFreeze(value);
    }

    if (argData.isFlagSet(kInvertPullFlag))
    {
        bool value;
        status = argData.getFlagArgument(kInvertPullFlag, 0, value);
        brushContext->setInvertPull(value);
    }

    if (argData.isFlagSet(kLineWidthFlag))
    {
        int value;
        status = argData.getFlagArgument(kLineWidthFlag, 0, value);
        brushContext->setLineWidth(value);
    }

    if (argData.isFlagSet(kMessageFlag))
    {
        bool value;
        status = argData.getFlagArgument(kMessageFlag, 0, value);
        brushContext->setMessage(value);
    }

    if (argData.isFlagSet(kPlaneAngleFlag))
    {
        double value;
        status = argData.getFlagArgument(kPlaneAngleFlag, 0, value);
        brushContext->setPlaneAngle(value);
    }

    if (argData.isFlagSet(kRelaxFlag))
    {
        bool value;
        status = argData.getFlagArgument(kRelaxFlag, 0, value);
        brushContext->setRelax(value);
    }

    if (argData.isFlagSet(kSizeFlag))
    {
        double value;
        status = argData.getFlagArgument(kSizeFlag, 0, value);
        brushContext->setSize(value);
    }

    if (argData.isFlagSet(kSmoothFlag))
    {
        double value;
        status = argData.getFlagArgument(kSmoothFlag, 0, value);
        brushContext->setSmooth(value);
    }

    if (argData.isFlagSet(kSmoothMeshFlag))
    {
        MString value;
        status = argData.getFlagArgument(kSmoothMeshFlag, 0, value);
        brushContext->setSmoothMesh(value);
    }

    if (argData.isFlagSet(kStrengthFlag))
    {
        double value;
        status = argData.getFlagArgument(kStrengthFlag, 0, value);
        brushContext->setStrength(value);
    }

    if (argData.isFlagSet(kSymmetryFlag))
    {
        bool value;
        status = argData.getFlagArgument(kSymmetryFlag, 0, value);
        brushContext->setSymmetry(value);
    }

    if (argData.isFlagSet(kSymmetryAxisFlag))
    {
        int value;
        status = argData.getFlagArgument(kSymmetryAxisFlag, 0, value);
        brushContext->setSymmetryAxis(value);
    }

    if (argData.isFlagSet(kToleranceFlag))
    {
        double value;
        status = argData.getFlagArgument(kToleranceFlag, 0, value);
        brushContext->setTolerance(value);
    }

    if (argData.isFlagSet(kTypeFlag))
    {
        int value;
        status = argData.getFlagArgument(kTypeFlag, 0, value);
        brushContext->setType(value);
    }

    if (argData.isFlagSet(kUsePlaneFlag))
    {
        bool value;
        status = argData.getFlagArgument(kUsePlaneFlag, 0, value);
        brushContext->setUsePlane(value);
    }

    if (argData.isFlagSet(kVolumeFlag))
    {
        bool value;
        status = argData.getFlagArgument(kVolumeFlag, 0, value);
        brushContext->setVolume(value);
    }

    return MStatus::kSuccess;
}

MStatus SHAPESBrushContextCmd::doQueryFlags()
{
    MArgParser argData = parser();

    if (argData.isFlagSet(kBlendMeshFlag))
        setResult(brushContext->getBlendMesh());

    if (argData.isFlagSet(kColorRFlag))
        setResult(brushContext->getColorR());

    if (argData.isFlagSet(kColorGFlag))
        setResult(brushContext->getColorG());

    if (argData.isFlagSet(kColorBFlag))
        setResult(brushContext->getColorB());

    if (argData.isFlagSet(kCurveFlag))
        setResult(brushContext->getCurve());

    if (argData.isFlagSet(kDepthFlag))
        setResult(brushContext->getDepth());

    if (argData.isFlagSet(kDepthStartFlag))
        setResult(brushContext->getDepthStart());

    if (argData.isFlagSet(kDrawBrushFlag))
        setResult(brushContext->getDrawBrush());

    if (argData.isFlagSet(kEnterToolCommandFlag))
        setResult(brushContext->getEnterToolCommand());

    if (argData.isFlagSet(kExitToolCommandFlag))
        setResult(brushContext->getExitToolCommand());

    if (argData.isFlagSet(kFlushFlag))
        setResult(brushContext->getFlushCache());

    if (argData.isFlagSet(kInvertFreezeFlag))
        setResult(brushContext->getInvertFreeze());

    if (argData.isFlagSet(kInvertPullFlag))
        setResult(brushContext->getInvertPull());

    if (argData.isFlagSet(kLineWidthFlag))
        setResult(brushContext->getLineWidth());

    if (argData.isFlagSet(kMessageFlag))
        setResult(brushContext->getMessage());

    if (argData.isFlagSet(kPlaneAngleFlag))
        setResult(brushContext->getPlaneAngle());

    if (argData.isFlagSet(kRelaxFlag))
        setResult(brushContext->getRelax());

    if (argData.isFlagSet(kSizeFlag))
        setResult(brushContext->getSize());

    if (argData.isFlagSet(kSmoothFlag))
        setResult(brushContext->getSmooth());

    if (argData.isFlagSet(kSmoothMeshFlag))
        setResult(brushContext->getSmoothMesh());

    if (argData.isFlagSet(kStrengthFlag))
        setResult(brushContext->getStrength());

    if (argData.isFlagSet(kSymmetryFlag))
        setResult(brushContext->getSymmetry());

    if (argData.isFlagSet(kSymmetryAxisFlag))
        setResult(brushContext->getSymmetryAxis());

    if (argData.isFlagSet(kToleranceFlag))
        setResult(brushContext->getTolerance());

    if (argData.isFlagSet(kTypeFlag))
        setResult(brushContext->getType());

    if (argData.isFlagSet(kUsePlaneFlag))
        setResult(brushContext->getUsePlane());

    if (argData.isFlagSet(kVolumeFlag))
        setResult(brushContext->getVolume());

    return MStatus::kSuccess;
}

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
