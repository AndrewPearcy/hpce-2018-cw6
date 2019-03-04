#!/bin/bash

#################################################
## Initial setup and parameters

# Abort the script if things fail
set -euxo pipefail

# Input video file
SRC="$1"

# Events associated with the input file
EVENTS="$2"

# Allows us to specify the number of frames to convert
FRAMES=""
if [[ $# -ge 3 ]] ; then
    FRAMES="$3"
fi

# Frame-rate of spiking compared to traditional
# We assume the traditional camera is 30FPS, and the
# spiking one is effectively 3000FPS
OVERSAMPLE=100
if [[ $# -ge 4 ]] ; then
    OVERSAMPLE="$4"
fi

VIS=0
if [[ $# -ge 5 ]] ; then
    if [[ "$5" == "1" ]] ; then
        VIS=1
    fi
fi

# Get the root of the input file
BASE="${SRC%.*}"

# Create temporary dir for intermediate files
mkdir -p $BASE/w
WORKING="${BASE}/w"


#####################################################################
## Actual processing

# Convert to the working mjpeg copy that tools can work with
scripts/video_to_mjpeg.sh  ${SRC} "${FRAMES}"  > ${WORKING}/input.mjpeg

if [[ "$EVENTS" == "" || "$EVENTS" == "random" ]] ; then
    EVENTS="${WORKING}/objects.events"
    bin/generate_object_stream < ${WORKING}/input.mjpeg > ${EVENTS}
fi
cp $EVENTS $BASE.true.events


# Create version with events
bin/frame_insert_objects  ${EVENTS}  < ${WORKING}/input.mjpeg  > ${WORKING}/objects.mjpeg

if [[ $VIS == 1 ]]; then
    # Visualise where the events are, based on true locations
    bin/frame_highlight_objects ${EVENTS}  < ${WORKING}/objects.mjpeg  > ${WORKING}/objects-true.mjpeg
fi

# Reference detected version, without subsampling or spiking filter
bin/frame_to_object_stream  < ${WORKING}/objects.mjpeg  > ${BASE}.ref.events

if [[ $VIS == 1 ]]; then
    # Visualise where the detected events are in the unfiltered version
    bin/frame_highlight_objects ${BASE}.ref.events  < ${WORKING}/objects.mjpeg  > ${WORKING}/objects-ref-detected.mjpeg
fi

# Convert the objects to spikes and classify it
bin/frame_to_spikes < ${WORKING}/objects.mjpeg > ${WORKING}/objects-spiking.spikes
# Then back into frames
bin/spikes_to_frame < ${WORKING}/objects-spiking.spikes > ${WORKING}/objects-spiking.mjpeg
# And then do the detection
bin/frame_to_object_stream < ${WORKING}/objects-spiking.mjpeg > ${BASE}.spiking.events

if [[ $VIS == 1 ]] ; then
    # Visualise where the spiking version thought the events were
    bin/frame_highlight_objects ${BASE}.spiking.events < ${WORKING}/objects-spiking.mjpeg > ${WORKING}/objects-spiking-detected.mjpeg
fi

# Sub-sample the objects to frames and classify it
bin/frame_subsample ${OVERSAMPLE} < ${WORKING}/objects.mjpeg > ${WORKING}/objects-trad.mjpeg
# ... then do the detection
bin/frame_to_object_stream < ${WORKING}/objects-trad.mjpeg > ${WORKING}/objects-trad-detected-tmp.events
# ... and then stretch time to reflect that this stream is much slower than the original
bin/object_stream_stretch ${OVERSAMPLE} < ${WORKING}/objects-trad-detected-tmp.events > ${BASE}.trad.events

if [[ $VIS == 1 ]] ; then
    # Visualise where the traditional version thought the events were
    bin/frame_highlight_objects ${BASE}.trad.events < ${WORKING}/objects.mjpeg > ${WORKING}/objects-trad-detected.mjpeg
fi


# Produce final visualisation comparing all four
if [[ $VIS == 1 ]] ; then
    ffmpeg -y -i ${WORKING}/objects-true.mjpeg               -i ${WORKING}/objects-ref-detected.mjpeg \
              -i ${WORKING}/objects-spiking-detected.mjpeg   -i ${WORKING}/objects-trad-detected.mjpeg \
           -filter_complex "[0:v][1:v]hstack=inputs=2[top];[2:v][3:v]hstack=inputs=2[bottom];[top][bottom]vstack=inputs=2[v]" -map "[v]" \
           ${WORKING}/objects-detected-all.mp4
fi

