#!/bin/bash

FRAMES=""
if [[ "$2" != "" && "$2" != "all" ]] ; then
    # Turn number of frames into seconds, assuming 30 fps
    FRAMES=" -t $(( $2 / 30 )).$(( ($2 * 10 / 30)%10 ))"

    >&2  echo "$FRAMES"
fi

QUALITY=0 # Best quality, big files
if [[ "$3" ]]; then
    QUALITY=$3
fi

ffmpeg -r 30 $FRAMES -vsync 0 -i $1 -f mjpeg -qscale:video ${QUALITY} -
