#!/usr/bin/env python
"""
Simple gvxr scene setup for manual visualization of Behelit model.
"""
import os
import numpy as np
import matplotlib.pyplot as plt
from gvxrPython3 import gvxr

gvxr.createOpenGLContext()
gvxr.setWindowBackGroundColour(1.0, 1.0, 1.0)
gvxr.setZoom(2500.0)

model_stl = os.path.join("data", "Behelit.stl")
gvxr.loadMeshFile("Model", model_stl, "mm")
gvxr.moveToCentre("Model")

# M = np.array(gvxr.getNodeWorldTransformationMatrix("Model")).reshape((4, 4)).T
# cx, cy, cz = M[:3, 3]
# gvxr.translateNode("Model", -cx, -cy, -cz, "mm")
# gvxr.rotateNode("Model", 90.0, 0.0, 1.0, 0.0)
# gvxr.rotateNode("Model", 270.0, 1.0, 0.0, 0.0)
# gvxr.translateNode("Model", cx, cy, cz, "mm")

gvxr.setCompound("Model", "Ca10(PO4)6(OH)2")
gvxr.setDensity("Model", 1.8, "g/cm3")
gvxr.setColour("Model", 1.0, 0.2, 0.2, 1.0)

gvxr.useParallelBeam()
gvxr.setSourcePosition(-200.0, 0.0, 0.0, "mm")
gvxr.setDetectorPosition(200.0, 0.0, 0.0, "mm")
gvxr.setDetectorUpVector(0, 1, 0)
gvxr.setDetectorNumberOfPixels(2048, 2048)
gvxr.setDetectorPixelSize(0.05, 0.05, "mm")

photon_energy_keV = 25.0
gvxr.setMonoChromatic(photon_energy_keV, "keV", 1e10)

gvxr.displayScene()
gvxr.renderLoop()
