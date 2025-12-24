#include "livePlaneDeformer.h"
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>

// Lattice helper functions implementation
void LivePlaneDeformer::createLattice(const MBoundingBox& bbox,
                                       int divisions,
                                       MPointArray& latticePoints)
{
    latticePoints.clear();

    MPoint minPt = bbox.min();
    MPoint maxPt = bbox.max();

    // Create a 3D grid of points
    for (int i = 0; i <= divisions; i++)
    {
        for (int j = 0; j <= divisions; j++)
        {
            for (int k = 0; k <= divisions; k++)
            {
                double u = (double)i / divisions;
                double v = (double)j / divisions;
                double w = (double)k / divisions;

                MPoint pt(
                    minPt.x + u * (maxPt.x - minPt.x),
                    minPt.y + v * (maxPt.y - minPt.y),
                    minPt.z + w * (maxPt.z - minPt.z)
                );

                latticePoints.append(pt);
            }
        }
    }
}

void LivePlaneDeformer::deformLattice(const MPointArray& originalLattice,
                                       MFnMesh& targetMesh,
                                       double offsetValue,
                                       double offsetYValue,
                                       MPointArray& deformedLattice)
{
    deformedLattice.clear();
    deformedLattice.setLength(originalLattice.length());

    // Deform each lattice point
    for (unsigned int i = 0; i < originalLattice.length(); i++)
    {
        MPoint latticePoint = originalLattice[i];

        // Ray cast downward
        MFloatPoint raySource(latticePoint.x, latticePoint.y + 1000.0, latticePoint.z);
        MFloatPoint rayDirection(0.0f, -1.0f, 0.0f);

        MFloatPointArray hitPoints;
        MIntArray hitFaces;
        MStatus status;

        bool hit = targetMesh.allIntersections(
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

        MPoint deformedPoint = latticePoint;

        if (hit && hitPoints.length() > 0)
        {
            // Project to surface
            deformedPoint = MPoint(hitPoints[0]);

            // Get interpolated normal
            MVector normal;
            status = targetMesh.getClosestNormal(deformedPoint, normal, MSpace::kWorld);
            if (status == MS::kSuccess)
            {
                normal.normalize();
                // Apply offsets
                deformedPoint = deformedPoint + (normal * offsetValue);
            }

            deformedPoint.y += offsetYValue;
        }
        else
        {
            // Fall back to closest point
            MPoint closestPoint;
            MVector normal;
            status = getClosestPointWithNormal(latticePoint, targetMesh, closestPoint, normal);
            if (status == MS::kSuccess)
            {
                deformedPoint = closestPoint + (normal * offsetValue);
                deformedPoint.y += offsetYValue;
            }
        }

        deformedLattice[i] = deformedPoint;
    }
}

MPoint LivePlaneDeformer::deformPointByLattice(const MPoint& point,
                                                 const MBoundingBox& bbox,
                                                 int divisions,
                                                 const MPointArray& originalLattice,
                                                 const MPointArray& deformedLattice)
{
    MPoint minPt = bbox.min();
    MPoint maxPt = bbox.max();

    // Calculate normalized position (0-1) within bounding box
    double u = (point.x - minPt.x) / (maxPt.x - minPt.x);
    double v = (point.y - minPt.y) / (maxPt.y - minPt.y);
    double w = (point.z - minPt.z) / (maxPt.z - minPt.z);

    // Clamp to [0, 1]
    u = (u < 0.0) ? 0.0 : (u > 1.0) ? 1.0 : u;
    v = (v < 0.0) ? 0.0 : (v > 1.0) ? 1.0 : v;
    w = (w < 0.0) ? 0.0 : (w > 1.0) ? 1.0 : w;

    // Find the lattice cell containing this point
    double cellU = u * divisions;
    double cellV = v * divisions;
    double cellW = w * divisions;

    int i0 = (int)cellU;
    int j0 = (int)cellV;
    int k0 = (int)cellW;

    int i1 = (i0 < divisions) ? i0 + 1 : i0;
    int j1 = (j0 < divisions) ? j0 + 1 : j0;
    int k1 = (k0 < divisions) ? k0 + 1 : k0;

    // Local coordinates within the cell (0-1)
    double tu = cellU - i0;
    double tv = cellV - j0;
    double tw = cellW - k0;

    // Trilinear interpolation
    // Get the 8 corner points of the cell
    int div1 = divisions + 1;
    int div2 = div1 * div1;

    int idx000 = i0 * div2 + j0 * div1 + k0;
    int idx001 = i0 * div2 + j0 * div1 + k1;
    int idx010 = i0 * div2 + j1 * div1 + k0;
    int idx011 = i0 * div2 + j1 * div1 + k1;
    int idx100 = i1 * div2 + j0 * div1 + k0;
    int idx101 = i1 * div2 + j0 * div1 + k1;
    int idx110 = i1 * div2 + j1 * div1 + k0;
    int idx111 = i1 * div2 + j1 * div1 + k1;

    // Get displacement vectors
    MVector disp000 = deformedLattice[idx000] - originalLattice[idx000];
    MVector disp001 = deformedLattice[idx001] - originalLattice[idx001];
    MVector disp010 = deformedLattice[idx010] - originalLattice[idx010];
    MVector disp011 = deformedLattice[idx011] - originalLattice[idx011];
    MVector disp100 = deformedLattice[idx100] - originalLattice[idx100];
    MVector disp101 = deformedLattice[idx101] - originalLattice[idx101];
    MVector disp110 = deformedLattice[idx110] - originalLattice[idx110];
    MVector disp111 = deformedLattice[idx111] - originalLattice[idx111];

    // Trilinear interpolation of displacement
    MVector disp00 = disp000 * (1.0 - tw) + disp001 * tw;
    MVector disp01 = disp010 * (1.0 - tw) + disp011 * tw;
    MVector disp10 = disp100 * (1.0 - tw) + disp101 * tw;
    MVector disp11 = disp110 * (1.0 - tw) + disp111 * tw;

    MVector disp0 = disp00 * (1.0 - tv) + disp01 * tv;
    MVector disp1 = disp10 * (1.0 - tv) + disp11 * tv;

    MVector finalDisp = disp0 * (1.0 - tu) + disp1 * tu;

    // Apply displacement to original point
    return point + finalDisp;
}
