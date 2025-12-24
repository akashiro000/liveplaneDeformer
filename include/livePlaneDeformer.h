#ifndef LIVE_PLANE_DEFORMER_H
#define LIVE_PLANE_DEFORMER_H

#include <maya/MPxDeformerNode.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MDagPath.h>
#include <maya/MBoundingBox.h>

class LivePlaneDeformer : public MPxDeformerNode
{
public:
    LivePlaneDeformer();
    virtual ~LivePlaneDeformer();

    static void* creator();
    static MStatus initialize();

    // Deformation function
    virtual MStatus deform(MDataBlock& block,
                          MItGeometry& iter,
                          const MMatrix& mat,
                          unsigned int multiIndex);

    static const MTypeId id;
    static const MString typeName;

    // Attributes
    static MObject aTargetMesh;      // Target mesh to conform to
    static MObject aOffset;          // Offset distance from target surface (normal direction)
    static MObject aOffsetY;         // Offset distance in Y axis (up direction)
    static MObject aDivisions;       // Lattice divisions (resolution)
    static MObject aEnvelope;        // Deformer envelope (inherited but we'll use it)

private:
    MStatus getClosestPointWithNormal(const MPoint& point,
                                      const MFnMesh& targetMesh,
                                      MPoint& closestPoint,
                                      MVector& normal);

    // Lattice-based deformation helpers
    void createLattice(const MBoundingBox& bbox,
                      int divisions,
                      MPointArray& latticePoints);

    void deformLattice(const MPointArray& originalLattice,
                      MFnMesh& targetMesh,
                      double offsetValue,
                      double offsetYValue,
                      MPointArray& deformedLattice);

    MPoint deformPointByLattice(const MPoint& point,
                               const MBoundingBox& bbox,
                               int divisions,
                               const MPointArray& originalLattice,
                               const MPointArray& deformedLattice);
};

#endif // LIVE_PLANE_DEFORMER_H
