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
#include <vector>

// Type ID - You should generate a unique ID for production use
// You can get one from Autodesk or use a random number in development
const MTypeId LivePlaneDeformer::id(0x00080900);
const MString LivePlaneDeformer::typeName("livePlaneDeformer");

// Attributes
MObject LivePlaneDeformer::aTargetMesh;
MObject LivePlaneDeformer::aOffset;
MObject LivePlaneDeformer::aDivisionsU;
MObject LivePlaneDeformer::aDivisionsV;
MObject LivePlaneDeformer::aDivisionsW;

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

    // Divisions U attribute (lattice resolution in X direction)
    aDivisionsU = nAttr.create("divisionsU", "du", MFnNumericData::kInt, 3, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(2);
    nAttr.setMax(20);
    status = addAttribute(aDivisionsU);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Divisions V attribute (lattice resolution in Z direction)
    aDivisionsV = nAttr.create("divisionsV", "dv", MFnNumericData::kInt, 3, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(2);
    nAttr.setMax(20);
    status = addAttribute(aDivisionsV);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Divisions W attribute (lattice resolution in Y direction)
    aDivisionsW = nAttr.create("divisionsW", "dw", MFnNumericData::kInt, 3, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    nAttr.setKeyable(true);
    nAttr.setMin(2);
    nAttr.setMax(20);
    status = addAttribute(aDivisionsW);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Attribute affects
    status = attributeAffects(aTargetMesh, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aOffset, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aDivisionsU, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aDivisionsV, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(aDivisionsW, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus LivePlaneDeformer::deform(MDataBlock& block,
                                   MItGeometry& iter,
                                   const MMatrix& mat,
                                   unsigned int multiIndex)
{
    MStatus status;

    // Get envelope value
    MDataHandle envelopeHandle = block.inputValue(envelope, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    float env = envelopeHandle.asFloat();

    if (env == 0.0f)
        return MS::kSuccess;

    // Get attributes
    MDataHandle offsetHandle = block.inputValue(aOffset, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    double offsetValue = offsetHandle.asDouble();

    MDataHandle divisionsUHandle = block.inputValue(aDivisionsU, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    int divU = divisionsUHandle.asInt();

    MDataHandle divisionsVHandle = block.inputValue(aDivisionsV, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    int divV = divisionsVHandle.asInt();

    MDataHandle divisionsWHandle = block.inputValue(aDivisionsW, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    int divW = divisionsWHandle.asInt();

    // Get target mesh
    MDataHandle targetMeshHandle = block.inputValue(aTargetMesh, &status);
    if (status != MS::kSuccess || targetMeshHandle.type() == MFnData::kInvalid)
        return MS::kSuccess;

    MObject targetMeshObj = targetMeshHandle.asMesh();
    if (targetMeshObj.isNull())
        return MS::kSuccess;

    MFnMesh targetMeshFn(targetMeshObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Get transformation matrices
    MMatrix matrix = mat;
    MMatrix inverseMatrix = matrix.inverse();

    // Calculate bounding box in local space
    MBoundingBox bbox;
    for (; !iter.isDone(); iter.next())
    {
        bbox.expand(iter.position());
    }
    iter.reset();

    // Create 3D lattice
    MPointArray originalLattice;
    createLattice(bbox, divU, divV, divW, originalLattice);

    // Deform lattice using Y-axis raycast projection
    // First pass: Calculate Y displacement for bottom layer only (w = 0)
    // Then apply same displacement to all layers to preserve thickness

    int strideU = 1;
    int strideV = (divU + 1);
    int strideW = (divU + 1) * (divV + 1);

    // Array to store displacement vectors for each XZ position
    std::vector<MVector> displacements(strideW, MVector(0.0, 0.0, 0.0));

    // Transform local Y axis to world space (need this for raycast direction)
    MVector localYAxis(0.0, 1.0, 0.0);
    MVector worldYAxis = localYAxis.transformAsNormal(matrix);
    worldYAxis.normalize();

    // Calculate displacement for bottom layer (w = 0) to place it on surface
    for (int v = 0; v <= divV; v++)
    {
        for (int u = 0; u <= divU; u++)
        {
            int bottomIndex = 0 * strideW + v * strideV + u * strideU;

            MPoint localPt = originalLattice[bottomIndex];
            MPoint worldPt = localPt * matrix;

            // Ray starts from far along +Y axis in object space
            MPoint rayStart = worldPt + (worldYAxis * 1000.0);
            MVector rayDir = -worldYAxis;  // Cast in -Y direction in object space

            MFloatPoint raySource(rayStart.x, rayStart.y, rayStart.z);
            MFloatPoint rayDirection(rayDir.x, rayDir.y, rayDir.z);

            MFloatPointArray hitPoints;
            MIntArray hitFaces;

            bool hit = targetMeshFn.allIntersections(
                raySource,
                rayDirection,
                NULL, NULL,
                false,
                MSpace::kWorld,
                10000.0f,
                false,
                NULL,
                false,
                hitPoints,
                NULL,
                &hitFaces,
                NULL, NULL, NULL, NULL,
                &status
            );

            MVector displacement(0.0, 0.0, 0.0);

            if (hit && hitPoints.length() > 0)
            {
                // Find closest hit (nearest to ray start)
                int closestHitIndex = 0;
                double minDist = rayStart.distanceTo(MPoint(hitPoints[0]));
                for (unsigned int j = 1; j < hitPoints.length(); j++)
                {
                    double dist = rayStart.distanceTo(MPoint(hitPoints[j]));
                    if (dist < minDist)
                    {
                        minDist = dist;
                        closestHitIndex = j;
                    }
                }

                MPoint hitPoint = MPoint(hitPoints[closestHitIndex]);

                // Get surface normal and apply offset
                MVector surfaceNormal;
                status = targetMeshFn.getClosestNormal(hitPoint, surfaceNormal, MSpace::kWorld);
                if (status == MS::kSuccess)
                {
                    surfaceNormal.normalize();
                    hitPoint = hitPoint + (surfaceNormal * offsetValue);
                }

                // Calculate displacement vector (not just Y component!)
                displacement = hitPoint - worldPt;
            }

            // Store displacement for this XZ position
            int xzIndex = v * strideV + u * strideU;
            displacements[xzIndex] = displacement;
        }
    }

    // Second pass: Apply displacement to all layers
    MPointArray deformedLattice;
    deformedLattice.setLength(originalLattice.length());

    for (int w = 0; w <= divW; w++)
    {
        for (int v = 0; v <= divV; v++)
        {
            for (int u = 0; u <= divU; u++)
            {
                int idx = w * strideW + v * strideV + u * strideU;
                int xzIndex = v * strideV + u * strideU;

                MPoint localPt = originalLattice[idx];
                MPoint worldPt = localPt * matrix;

                // Apply displacement vector uniformly to all layers
                worldPt = worldPt + displacements[xzIndex];

                // Convert back to local space
                deformedLattice[idx] = worldPt * inverseMatrix;
            }
        }
    }

    // Apply FFD to each vertex
    for (; !iter.isDone(); iter.next())
    {
        MPoint localPt = iter.position();

        // Deform using trilinear interpolation
        MPoint deformedPt = deformPointByLattice(localPt, bbox, divU, divV, divW,
                                                  originalLattice, deformedLattice);

        // Blend with envelope
        MPoint finalPt = localPt + ((deformedPt - localPt) * env);

        iter.setPosition(finalPt);
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
