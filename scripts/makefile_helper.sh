#!/bin/bash

# This creates upfront targets so that bash autocompletion works

for i in media/*.mp4 ; do
    f=$(basename ${i} .mp4);
    echo "working/${f}.mjpeg : ";
    echo ""

    echo "working/${f}.mjpeg.play :";
    echo ""
done

for i in src/*.cpp ; do
    f=$(basename ${i} .cpp);
    echo "bin/${f} : ";
    echo ""
done