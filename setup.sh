#!/bin/bash

export COMPUTE_ROOT=$HOME/computeShaderAlgorithms

# Requires mesa 24.x library for OpenGL 4.3+
export MESA_GL_VERSION_OVERRIDE=4.3
export MESA_GLSL_VERSION_OVERRIDE=430

mkdir build
echo "Setup done"
