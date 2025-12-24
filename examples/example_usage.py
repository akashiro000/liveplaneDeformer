"""
livePlaneDeformer Example Usage Script (Python)
This script demonstrates how to use the livePlaneDeformer plugin in Python
"""

import maya.cmds as cmds


def cleanup_scene():
    """Clean up existing objects"""
    objects = ['sourceMesh', 'targetMesh', 'wave1', 'livePlaneDeformerUI']

    for obj in objects:
        if cmds.objExists(obj):
            if cmds.window(obj, exists=True):
                cmds.deleteUI(obj)
            else:
                cmds.delete(obj)


def create_example_scene():
    """Create example scene with livePlaneDeformer"""

    # Clean up
    cleanup_scene()

    # Create a sphere as the source mesh
    source_mesh = cmds.polySphere(n="sourceMesh", r=2, sx=20, sy=20)[0]
    cmds.move(0, 0, 0, source_mesh)

    # Create a plane as the target mesh
    target_mesh = cmds.polyPlane(n="targetMesh", w=10, h=10, sx=30, sy=30)[0]
    cmds.move(0, -2, 0, target_mesh)

    # Add a wave deformer to create surface variation
    cmds.select(target_mesh)
    wave_deformer = cmds.nonLinear(type='wave')[0]
    cmds.setAttr(f'{wave_deformer}.amplitude', 0.5)
    cmds.setAttr(f'{wave_deformer}.wavelength', 2.0)

    # Apply the livePlaneDeformer to the source mesh
    cmds.select(source_mesh)
    deformer_node = cmds.deformer(type='livePlaneDeformer')[0]

    # Connect the target mesh
    cmds.connectAttr(f'{target_mesh}.worldMesh[0]',
                    f'{deformer_node}.targetMesh', force=True)

    # Set initial offset
    cmds.setAttr(f'{deformer_node}.offset', 0.0)

    # Create UI
    create_control_ui(deformer_node, wave_deformer)

    # Frame all objects
    cmds.select(source_mesh, target_mesh)
    cmds.viewFit(all=True)

    print("livePlaneDeformer example setup complete!")
    print(f"Deformer node: {deformer_node}")

    return deformer_node, wave_deformer


def create_control_ui(deformer_node, wave_node):
    """Create a simple UI to control the deformer"""

    if cmds.window('livePlaneDeformerUI', exists=True):
        cmds.deleteUI('livePlaneDeformerUI')

    window = cmds.window('livePlaneDeformerUI',
                        title='Live Plane Deformer Controls',
                        widthHeight=(300, 200))

    cmds.columnLayout(adjustableColumn=True)

    # Deformer controls
    cmds.text(label='Deformer Controls', font='boldLabelFont')
    cmds.separator(height=10)

    cmds.floatSliderGrp('offsetSlider',
                       label='Offset (Normal)',
                       field=True,
                       minValue=-5.0,
                       maxValue=5.0,
                       value=0.0,
                       step=0.01,
                       changeCommand=lambda val: cmds.setAttr(
                           f'{deformer_node}.offset', val))

    cmds.floatSliderGrp('offsetYSlider',
                       label='Offset Y (Up)',
                       field=True,
                       minValue=-5.0,
                       maxValue=5.0,
                       value=0.0,
                       step=0.01,
                       changeCommand=lambda val: cmds.setAttr(
                           f'{deformer_node}.offsetY', val))

    cmds.intSliderGrp('divisionsSlider',
                     label='Lattice Divisions',
                     field=True,
                     minValue=2,
                     maxValue=20,
                     value=5,
                     step=1,
                     changeCommand=lambda val: cmds.setAttr(
                         f'{deformer_node}.divisions', val))

    cmds.floatSliderGrp('envelopeSlider',
                       label='Envelope',
                       field=True,
                       minValue=0.0,
                       maxValue=1.0,
                       value=1.0,
                       step=0.01,
                       changeCommand=lambda val: cmds.setAttr(
                           f'{deformer_node}.envelope', val))

    cmds.separator(height=10)

    # Wave deformer controls
    cmds.text(label='Wave Deformer Controls', font='boldLabelFont')
    cmds.separator(height=10)

    cmds.floatSliderGrp('amplitudeSlider',
                       label='Amplitude',
                       field=True,
                       minValue=0.0,
                       maxValue=2.0,
                       value=0.5,
                       step=0.01,
                       changeCommand=lambda val: cmds.setAttr(
                           f'{wave_node}.amplitude', val))

    cmds.floatSliderGrp('wavelengthSlider',
                       label='Wavelength',
                       field=True,
                       minValue=0.5,
                       maxValue=5.0,
                       value=2.0,
                       step=0.1,
                       changeCommand=lambda val: cmds.setAttr(
                           f'{wave_node}.wavelength', val))

    cmds.showWindow(window)


# Run the example
if __name__ == '__main__':
    create_example_scene()
