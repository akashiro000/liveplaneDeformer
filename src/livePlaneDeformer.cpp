#include "livePlaneDeformer.h"
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MFnMeshData.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>

// Type ID - You should generate a unique ID for production use
// You can get one from Autodesk or use a random number in development
const MTypeId LivePlaneDeformer::id(0x00080900);
const MString LivePlaneDeformer::typeName("livePlaneDeformer");

// Attributes
MObject LivePlaneDeformer::aTargetMesh;
MObject LivePlaneDeformer::aOffset;
MObject LivePlaneDeformer::aOffsetY;
MObject LivePlaneDeformer::aDivisions;

LivePlaneDeformer::LivePlaneDeformer()
{
}

LivePlaneDeformer::~LivePlaneDeformer()
{
}

void* LivePlaneDeformer::creator()
{
    return new LivePlaneDeformer();
}

MStatus LivePlaneDeformer::initialize()
{
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Target mesh attribute (input mesh to conform to)
    aTargetMesh = tAttr.create("targetMesh", "tm", MFnData::kMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    tAttr.setStorable(false);
    tAttr.setConnectable(true);
    status = addAttribute(aTargetMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Offset attribute (distance from target surface along normal)
    aOffset = nAttr.create("offset", "off", MFnNumericData::kDouble, 0.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(-10.0);
    nAttr.setMax(10.0);
    status = addAttribute(aOffset);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Offset Y attribute (distance in Y axis)
    aOffsetY = nAttr.create("offsetY", "ofy", MFnNumericData::kDouble, 0.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(-10.0);
    nAttr.setMax(10.0);
    status = addAttribute(aOffsetY);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Divisions attribute (lattice resolution)
    aDivisions = nAttr.create("divisions", "div", MFnNumericData::kInt, 5, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(2);
    nAttr.setMax(20);
    status = addAttribute(aDivisions);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Attribute affects
    status = attributeAffects(aTargetMesh, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aOffset, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aOffsetY, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aDivisions, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus LivePlaneDeformer::deform(MDataBlock& block,
                                   MItGeometry& iter,
                                   const MMatrix& mat,
                                   unsigned int multiIndex)
{
    MStatus status;

    // Get envelope value (controls overall deformation amount)
    MDataHandle envelopeHandle = block.inputValue(envelope, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    float env = envelopeHandle.asFloat();

    // Early exit if envelope is zero
    if (env == 0.0f)
        return MS::kSuccess;

    // Get offset values
    MDataHandle offsetHandle = block.inputValue(aOffset, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    double offsetValue = offsetHandle.asDouble();

    MDataHandle offsetYHandle = block.inputValue(aOffsetY, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    double offsetYValue = offsetYHandle.asDouble();

    MDataHandle divisionsHandle = block.inputValue(aDivisions, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    int divisions = divisionsHandle.asInt();

    // Get target mesh
    MDataHandle targetMeshHandle = block.inputValue(aTargetMesh, &status);
    if (status != MS::kSuccess || targetMeshHandle.type() == MFnData::kInvalid)
    {
        // No target mesh connected, just return
        return MS::kSuccess;
    }

    MObject targetMeshObj = targetMeshHandle.asMesh();
    if (targetMeshObj.isNull())
    {
        return MS::kSuccess;
    }

    MFnMesh targetMeshFn(targetMeshObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Get the matrix to transform points to world space
    MMatrix matrix = mat;
    MMatrix inverseMatrix = matrix.inverse();

    // Calculate bounding box of the geometry in world space
    // First pass: calculate bounding box
    MBoundingBox bbox;
    for (; !iter.isDone(); iter.next())
    {
        MPoint localPt = iter.position();
        MPoint worldPt = localPt * matrix;
        bbox.expand(worldPt);
    }

    // Reset iterator for second pass
    iter.reset();

    // Create lattice structure
    MPointArray originalLattice;
    createLattice(bbox, divisions, originalLattice);

    // Deform the lattice based on target mesh
    MPointArray deformedLattice;
    deformLattice(originalLattice, targetMeshFn, offsetValue, offsetYValue, deformedLattice);

    // Second pass: deform each vertex based on the lattice deformation
    for (; !iter.isDone(); iter.next())
    {
        // Get point in local space and transform to world space
        MPoint localPoint = iter.position();
        MPoint worldPoint = localPoint * matrix;

        // Deform point using lattice
        MPoint deformedWorldPoint = deformPointByLattice(
            worldPoint,
            bbox,
            divisions,
            originalLattice,
            deformedLattice
        );

        // Blend with original position using envelope (in world space)
        MPoint finalWorldPoint = worldPoint + ((deformedWorldPoint - worldPoint) * env);

        // Transform back to local space
        MPoint finalLocalPoint = finalWorldPoint * inverseMatrix;

        // Set the new position in local space
        iter.setPosition(finalLocalPoint);
    }

    return MS::kSuccess;
}

MStatus LivePlaneDeformer::getClosestPointWithNormal(const MPoint& point,
                                                      const MFnMesh& targetMesh,
                                                      MPoint& closestPoint,
                                                      MVector& normal)
{
    MStatus status;

    // Get closest point on the mesh surface
    int closestPolygon;
    status = targetMesh.getClosestPoint(point, closestPoint, MSpace::kWorld, &closestPolygon);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Get the interpolated normal at the closest point using getClosestNormal
    // This provides smooth normal interpolation across the surface
    status = targetMesh.getClosestNormal(point, normal, MSpace::kWorld);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Normalize the normal vector
    normal.normalize();

    return MS::kSuccess;
}

// Plugin initialization
MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "YourName", "1.0", "Any");

    status = plugin.registerNode(
        LivePlaneDeformer::typeName,
        LivePlaneDeformer::id,
        LivePlaneDeformer::creator,
        LivePlaneDeformer::initialize,
        MPxNode::kDeformerNode
    );

    if (!status)
    {
        status.perror("registerNode");
        return status;
    }

    MGlobal::displayInfo("livePlaneDeformer plugin loaded successfully");

    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterNode(LivePlaneDeformer::id);

    if (!status)
    {
        status.perror("deregisterNode");
        return status;
    }

    MGlobal::displayInfo("livePlaneDeformer plugin unloaded successfully");

    return status;
}
