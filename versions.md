**3.1.1 (2021-03-29) - Update**
* Added support for Maya 2022.

**3.1.1 (2020-08-24)**
* Changed the settings for controlling the undersampling from environment to Maya preference variables (SHAPESBrushUndersamplingAdjust and SHAPESBrushUndersamplingPull).

**3.1.0 (2020-08-21)**
* Re-introduced support for the legacy viewport.
* Re-introduced that defining the blend-to mesh can be performed through selection order.
* Added two environment variables to control the undersampling while adjusting the brush and using the pull/push brush. (SHAPES_BRUSH_UNDERSAMPLING_ADJUST=2, SHAPES_BRUSH_UNDERSAMPLING_PULL=3)
* Replaced the command-line installer by the drag-and-drop installer.

**3.0.1 (2020-06-05)**
* Fixed: When using a component selection the brush action is not restricted and the selection gets cleared.
* Fixed: Script error when unloading the plug-in.

**3.0.0 (2020-03-05)**
* Initial open source release.
* Added support for Maya 2020.
* Added a menu item to the Maya modify menu.
* Removed support for the legacy viewport. The brush circle doesn't display.
* Changed the style of the brush circle and the strength value.
* General improvements to the surface detection. Working with the brush depth is now more reliable.
* The symmetry tolerance value is set to 0 by default which enables automatic tolerance detection. This is based on the average edge length of the mesh.
* General tool command improvements.
