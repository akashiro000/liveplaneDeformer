"""
livePlaneDeformer Tool - Production UI
Professional tool for managing livePlaneDeformer nodes in Maya
"""

import maya.cmds as cmds
import maya.api.OpenMaya as om


class LivePlaneDeformerTool:
    """Main tool class for livePlaneDeformer management"""

    WINDOW_NAME = "livePlaneDeformerToolUI"
    WINDOW_TITLE = "Live Plane Deformer Tool"

    def __init__(self):
        self.current_deformer = None
        self.selection_job = None

    def show(self):
        """Show the tool window"""
        # Delete existing window
        if cmds.window(self.WINDOW_NAME, exists=True):
            cmds.deleteUI(self.WINDOW_NAME)

        # Create window
        self.window = cmds.window(
            self.WINDOW_NAME,
            title=self.WINDOW_TITLE,
            widthHeight=(400, 500),
            sizeable=True
        )

        # Main layout
        main_layout = cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        # Create tab layout
        tabs = cmds.tabLayout(innerMarginWidth=5, innerMarginHeight=5)

        # Tab 1: Setup
        self._create_setup_tab()

        # Tab 2: Parameters
        self._create_parameters_tab()

        # Tab 3: Info
        self._create_info_tab()

        cmds.tabLayout(tabs, edit=True, tabLabel=(
            (self.setup_tab, 'Setup'),
            (self.params_tab, 'Parameters'),
            (self.info_tab, 'Info')
        ))

        cmds.showWindow(self.window)

        # Setup selection callback
        self._setup_selection_callback()

        # Update UI with current selection
        self._on_selection_changed()

    def _create_setup_tab(self):
        """Create the setup tab"""
        self.setup_tab = cmds.columnLayout(adjustableColumn=True, rowSpacing=10)

        # Header
        cmds.text(label="Deformer Setup", font="boldLabelFont", height=30)
        cmds.separator(height=10, style="in")

        # Apply Deformer Section
        cmds.frameLayout(label="Apply Deformer", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        cmds.text(label="1. Select source mesh(es)", align="left")
        cmds.button(
            label="Apply livePlaneDeformer",
            height=40,
            backgroundColor=(0.4, 0.6, 0.4),
            command=lambda x: self._apply_deformer()
        )

        cmds.setParent('..')
        cmds.setParent('..')

        # Target Mesh Section
        cmds.frameLayout(label="Target Mesh", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        cmds.rowColumnLayout(numberOfColumns=2, columnWidth=[(1, 300), (2, 80)])
        self.target_mesh_field = cmds.textField(
            editable=False,
            placeholderText="No target mesh connected"
        )
        cmds.button(
            label="Set Target",
            command=lambda x: self._set_target_mesh()
        )
        cmds.setParent('..')

        cmds.button(
            label="Clear Target",
            command=lambda x: self._clear_target_mesh()
        )

        cmds.setParent('..')
        cmds.setParent('..')

        # Current Deformer Info
        cmds.frameLayout(label="Current Deformer", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        self.current_deformer_text = cmds.text(
            label="No deformer selected",
            align="left",
            font="smallPlainLabelFont"
        )

        cmds.button(
            label="Select Deformer Node",
            command=lambda x: self._select_deformer_node()
        )

        cmds.setParent('..')
        cmds.setParent('..')

        cmds.setParent('..')

    def _create_parameters_tab(self):
        """Create the parameters tab"""
        self.params_tab = cmds.scrollLayout(childResizable=True)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=10)

        # Header
        cmds.text(label="Deformer Parameters", font="boldLabelFont", height=30)
        cmds.separator(height=10, style="in")

        # Deformer selector
        cmds.text(label="Active Deformer:", align="left", font="boldLabelFont")
        self.params_deformer_text = cmds.text(
            label="Select a mesh with livePlaneDeformer",
            align="left",
            font="obliqueLabelFont"
        )
        cmds.separator(height=10)

        # Offset Parameters
        cmds.frameLayout(label="Offset Controls", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        self.offset_slider = cmds.floatSliderGrp(
            label="Offset (Normal)",
            field=True,
            minValue=-5.0,
            maxValue=5.0,
            value=0.0,
            step=0.01,
            precision=3,
            changeCommand=self._on_offset_changed
        )

        self.offsetY_slider = cmds.floatSliderGrp(
            label="Offset Y (Up)",
            field=True,
            minValue=-5.0,
            maxValue=5.0,
            value=0.0,
            step=0.01,
            precision=3,
            changeCommand=self._on_offsetY_changed
        )

        cmds.setParent('..')
        cmds.setParent('..')

        # Lattice Parameters
        cmds.frameLayout(label="Lattice Controls", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        self.divisions_slider = cmds.intSliderGrp(
            label="Divisions",
            field=True,
            minValue=2,
            maxValue=20,
            value=5,
            step=1,
            changeCommand=self._on_divisions_changed
        )

        cmds.text(
            label="Higher values = smoother but slower",
            align="left",
            font="smallObliqueLabelFont"
        )

        cmds.setParent('..')
        cmds.setParent('..')

        # Envelope
        cmds.frameLayout(label="Influence", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        self.envelope_slider = cmds.floatSliderGrp(
            label="Envelope",
            field=True,
            minValue=0.0,
            maxValue=1.0,
            value=1.0,
            step=0.01,
            precision=3,
            changeCommand=self._on_envelope_changed
        )

        cmds.setParent('..')
        cmds.setParent('..')

        # Reset button
        cmds.separator(height=10)
        cmds.button(
            label="Reset All Parameters",
            command=lambda x: self._reset_parameters()
        )

        cmds.setParent('..')
        cmds.setParent('..')

    def _create_info_tab(self):
        """Create the info tab"""
        self.info_tab = cmds.scrollLayout(childResizable=True)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=10)

        # Header
        cmds.text(label="About livePlaneDeformer", font="boldLabelFont", height=30)
        cmds.separator(height=10, style="in")

        # Description
        cmds.frameLayout(label="Description", collapsable=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        info_text = (
            "livePlaneDeformer is a Maya deformer plugin that conforms\n"
            "a source mesh to the surface of a target mesh.\n\n"
            "It uses a lattice-based algorithm with trilinear interpolation\n"
            "to provide smooth, shape-preserving deformations."
        )
        cmds.text(label=info_text, align="left", wordWrap=True)

        cmds.setParent('..')
        cmds.setParent('..')

        # Usage
        cmds.frameLayout(label="Usage", collapsable=True, collapse=False)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        usage_text = (
            "1. Select source mesh(es)\n"
            "2. Apply livePlaneDeformer (Setup tab)\n"
            "3. Select target mesh and click 'Set Target'\n"
            "4. Adjust parameters in Parameters tab\n\n"
            "The source mesh will conform to the target mesh surface."
        )
        cmds.text(label=usage_text, align="left", wordWrap=True)

        cmds.setParent('..')
        cmds.setParent('..')

        # Parameters Info
        cmds.frameLayout(label="Parameters", collapsable=True, collapse=True)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5)

        params_text = (
            "• Offset (Normal): Distance along surface normal\n"
            "• Offset Y (Up): Additional offset in Y direction\n"
            "• Divisions: Lattice resolution (2-20)\n"
            "  Higher = smoother but slower\n"
            "• Envelope: Overall deformation influence (0-1)"
        )
        cmds.text(label=params_text, align="left", wordWrap=True)

        cmds.setParent('..')
        cmds.setParent('..')

        # Version
        cmds.separator(height=20)
        cmds.text(label="Version 1.0", align="center", font="smallObliqueLabelFont")

        cmds.setParent('..')
        cmds.setParent('..')

    def _setup_selection_callback(self):
        """Setup callback for selection changes"""
        # Remove old callback if exists
        if self.selection_job:
            if cmds.scriptJob(exists=self.selection_job):
                cmds.scriptJob(kill=self.selection_job, force=True)

        # Create new callback
        self.selection_job = cmds.scriptJob(
            event=["SelectionChanged", self._on_selection_changed],
            parent=self.window
        )

    def _on_selection_changed(self):
        """Called when selection changes"""
        # Find livePlaneDeformer on selected objects
        selected = cmds.ls(selection=True, type='transform')

        if not selected:
            self.current_deformer = None
            self._update_ui()
            return

        # Check first selected object for deformer
        deformer = self._get_deformer_from_mesh(selected[0])

        if deformer:
            self.current_deformer = deformer
            self._update_ui()
            self._load_parameters()

    def _get_deformer_from_mesh(self, mesh):
        """Get livePlaneDeformer from a mesh"""
        try:
            history = cmds.listHistory(mesh, pruneDagObjects=True) or []
            for node in history:
                if cmds.nodeType(node) == 'livePlaneDeformer':
                    return node
        except:
            pass
        return None

    def _apply_deformer(self):
        """Apply livePlaneDeformer to selected meshes"""
        selected = cmds.ls(selection=True, type='transform')

        if not selected:
            cmds.warning("Please select at least one mesh")
            return

        applied_deformers = []

        for mesh in selected:
            try:
                # Check if already has deformer
                existing = self._get_deformer_from_mesh(mesh)
                if existing:
                    cmds.warning(f"{mesh} already has livePlaneDeformer: {existing}")
                    continue

                # Apply deformer
                cmds.select(mesh)
                deformer = cmds.deformer(type='livePlaneDeformer')[0]
                applied_deformers.append(deformer)

                cmds.info(f"Applied {deformer} to {mesh}")

            except Exception as e:
                cmds.warning(f"Failed to apply deformer to {mesh}: {str(e)}")

        if applied_deformers:
            # Select first mesh and update UI
            cmds.select(selected[0])
            self.current_deformer = applied_deformers[0]
            self._update_ui()
            self._load_parameters()

            cmds.confirmDialog(
                title="Success",
                message=f"Applied {len(applied_deformers)} deformer(s)",
                button=["OK"]
            )

    def _set_target_mesh(self):
        """Set target mesh for current deformer"""
        if not self.current_deformer:
            cmds.warning("No active deformer. Please select a mesh with livePlaneDeformer.")
            return

        selected = cmds.ls(selection=True, type='transform')

        if not selected:
            cmds.warning("Please select a target mesh")
            return

        target_mesh = selected[0]

        try:
            # Connect target mesh
            cmds.connectAttr(
                f'{target_mesh}.worldMesh[0]',
                f'{self.current_deformer}.targetMesh',
                force=True
            )

            cmds.info(f"Connected {target_mesh} as target")
            self._update_ui()

        except Exception as e:
            cmds.warning(f"Failed to set target mesh: {str(e)}")

    def _clear_target_mesh(self):
        """Clear target mesh connection"""
        if not self.current_deformer:
            return

        try:
            connections = cmds.listConnections(
                f'{self.current_deformer}.targetMesh',
                source=True,
                destination=False,
                plugs=True
            )

            if connections:
                cmds.disconnectAttr(connections[0], f'{self.current_deformer}.targetMesh')
                cmds.info("Target mesh cleared")
                self._update_ui()

        except Exception as e:
            cmds.warning(f"Failed to clear target: {str(e)}")

    def _select_deformer_node(self):
        """Select the deformer node in the outliner"""
        if self.current_deformer:
            cmds.select(self.current_deformer, replace=True)

    def _update_ui(self):
        """Update UI based on current deformer"""
        if self.current_deformer and cmds.objExists(self.current_deformer):
            # Update deformer name displays
            cmds.text(self.current_deformer_text, edit=True,
                     label=f"Active: {self.current_deformer}")
            cmds.text(self.params_deformer_text, edit=True,
                     label=f"{self.current_deformer}")

            # Update target mesh field
            connections = cmds.listConnections(
                f'{self.current_deformer}.targetMesh',
                source=True,
                destination=False
            )

            if connections:
                cmds.textField(self.target_mesh_field, edit=True,
                              text=connections[0])
            else:
                cmds.textField(self.target_mesh_field, edit=True,
                              text="")

        else:
            cmds.text(self.current_deformer_text, edit=True,
                     label="No deformer selected")
            cmds.text(self.params_deformer_text, edit=True,
                     label="Select a mesh with livePlaneDeformer")
            cmds.textField(self.target_mesh_field, edit=True, text="")

    def _load_parameters(self):
        """Load parameters from current deformer"""
        if not self.current_deformer or not cmds.objExists(self.current_deformer):
            return

        try:
            # Load values
            offset = cmds.getAttr(f'{self.current_deformer}.offset')
            offsetY = cmds.getAttr(f'{self.current_deformer}.offsetY')
            divisions = cmds.getAttr(f'{self.current_deformer}.divisions')
            envelope = cmds.getAttr(f'{self.current_deformer}.envelope')

            # Update sliders
            cmds.floatSliderGrp(self.offset_slider, edit=True, value=offset)
            cmds.floatSliderGrp(self.offsetY_slider, edit=True, value=offsetY)
            cmds.intSliderGrp(self.divisions_slider, edit=True, value=divisions)
            cmds.floatSliderGrp(self.envelope_slider, edit=True, value=envelope)

        except Exception as e:
            cmds.warning(f"Failed to load parameters: {str(e)}")

    def _on_offset_changed(self, value):
        """Callback for offset slider"""
        if self.current_deformer and cmds.objExists(self.current_deformer):
            cmds.setAttr(f'{self.current_deformer}.offset', value)

    def _on_offsetY_changed(self, value):
        """Callback for offsetY slider"""
        if self.current_deformer and cmds.objExists(self.current_deformer):
            cmds.setAttr(f'{self.current_deformer}.offsetY', value)

    def _on_divisions_changed(self, value):
        """Callback for divisions slider"""
        if self.current_deformer and cmds.objExists(self.current_deformer):
            cmds.setAttr(f'{self.current_deformer}.divisions', value)

    def _on_envelope_changed(self, value):
        """Callback for envelope slider"""
        if self.current_deformer and cmds.objExists(self.current_deformer):
            cmds.setAttr(f'{self.current_deformer}.envelope', value)

    def _reset_parameters(self):
        """Reset all parameters to default values"""
        if not self.current_deformer or not cmds.objExists(self.current_deformer):
            cmds.warning("No active deformer")
            return

        result = cmds.confirmDialog(
            title="Reset Parameters",
            message="Reset all parameters to default values?",
            button=["Yes", "No"],
            defaultButton="Yes",
            cancelButton="No"
        )

        if result == "Yes":
            cmds.setAttr(f'{self.current_deformer}.offset', 0.0)
            cmds.setAttr(f'{self.current_deformer}.offsetY', 0.0)
            cmds.setAttr(f'{self.current_deformer}.divisions', 5)
            cmds.setAttr(f'{self.current_deformer}.envelope', 1.0)

            self._load_parameters()
            cmds.info("Parameters reset to defaults")


# Global instance
_tool_instance = None


def show():
    """Show the tool window"""
    global _tool_instance
    _tool_instance = LivePlaneDeformerTool()
    _tool_instance.show()


# For testing
if __name__ == '__main__':
    show()
