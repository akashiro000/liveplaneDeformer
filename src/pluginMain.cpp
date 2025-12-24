#include "livePlaneDeformer.h"
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

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
