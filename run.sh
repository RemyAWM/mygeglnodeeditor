#!/bin/bash


export PKG_CONFIG_PATH=/opt/babl/lib/pkgconfig:/opt/gegl-03/lib/pkgconfig:/opt/gimp-git/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/opt/babl/lib:/opt/gegl-03/lib:/opt/gimp-git/lib:$LD_LIBRARY_PATH

./Default/myGeglApp