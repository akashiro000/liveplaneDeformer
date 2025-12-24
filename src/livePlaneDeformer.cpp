#include "livePlaneDeformer.h"
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MFnMeshData.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>

// Type ID - You should generate a unique ID for production use
// You can get one from Autodesk or use a random number in development
const MTypeId LivePlaneDeformer::id(0x00080900);
const MString LivePlaneDeformer::typeName("livePlaneDeformer");

// Attributes
MObject LivePlaneDeformer::aTargetMesh;
MObject LivePlaneDeformer::aOffset;

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

    // Offset attribute (distance from target surface)
    aOffset = nAttr.create("offset", "off", MFnNumericData::kDouble, 0.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(-10.0);
    nAttr.setMax(10.0);
    status = addAttribute(aOffset);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Attribute affects
    status = attributeAffects(aTargetMesh, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aOffset, outputGeom);
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

    // Get offset value
    MDataHandle offsetHandle = block.inputValue(aOffset, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    double offsetValue = offsetHandle.asDouble();

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

    // Iterate through all points in the geometry
    for (; !iter.isDone(); iter.next())
    {
        MPoint point = iter.position();

        // Find closest point on target mesh
        MPoint closestPoint;
        MVector normal;
        status = getClosestPoint(point, targetMeshFn, closestPoint, normal);

        if (status == MS::kSuccess)
        {
            // Apply offset along normal
            MPoint newPoint = closestPoint + (normal * offsetValue);

            // Blend with original position using envelope
            newPoint = point + ((newPoint - point) * env);

            // Set the new position
            iter.setPosition(newPoint);
        }
    }

    return MS::kSuccess;
}

MStatus LivePlaneDeformer::getClosestPoint(const MPoint& point,
                                            const MFnMesh& targetMesh,
                                            MPoint& closestPoint,
                                            MVector& normal)
{
    MStatus status;

    // Use MMeshIntersector for efficient closest point queries
    MMeshIntersector intersector;
    status = intersector.create(targetMesh.object(), targetMesh.dagPath().inclusiveMatrix());
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPointOnMesh pointOnMesh;
    status = intersector.getClosestPoint(point, pointOnMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    closestPoint = pointOnMesh.getPoint();
    normal = pointOnMesh.getNormal();

    return MS::kSuccess;
}
