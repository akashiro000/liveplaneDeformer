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
#include <maya/MMeshIntersector.h>

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
    static MObject aOffset;          // Offset distance from target surface
    static MObject aEnvelope;        // Deformer envelope (inherited but we'll use it)

private:
    MStatus getClosestPoint(const MPoint& point,
                           const MFnMesh& targetMesh,
                           MPoint& closestPoint,
                           MVector& normal);
};

#endif // LIVE_PLANE_DEFORMER_H
