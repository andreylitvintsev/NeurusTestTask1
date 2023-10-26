#!/usr/bin/bash

#export GST_DEBUG="*:2"

SCRIPT=$(realpath -s "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

gst-launch-1.0 \
--gst-plugin-path=$SCRIPTPATH \
uridecodebin uri=https://gstreamer.freedesktop.org/data/media/large/matrix.avi \
! videoconvert ! video/x-raw, format=BGR \
! yolofilter \
    draw=true \
    cfg=$SCRIPTPATH/yolov3.cfg \
    weights=$SCRIPTPATH/yolov3.weights \
    classes=$SCRIPTPATH/coco.names \
! videoconvert ! video/x-raw, format=I420 \
! fpsdisplaysink
