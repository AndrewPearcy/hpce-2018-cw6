#!/bin/bash

# NOTE: This needs a recent version of ffmpeg. You may want to update via:
#   sudo add-apt-repository ppa:jonathonf/ffmpeg-4
#   sudo apt update
#   sudo apt upgrade ffmpeg

INPUT=$1
OUTPUT=$2

echo ffmpeg -i $1 -filter:v "minterpolate='mi_mode=mci:mc_mode=aobmc:vsbmc=1:fps=${FPS}'" ${OUTPUT}
ffmpeg -r 1 -i $1 -filter:v "minterpolate='mi_mode=mci:mc_mode=aobmc:me_mode=bidir:vsbmc=1:fps=30'" ${OUTPUT}