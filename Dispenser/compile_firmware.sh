#!/usr/bin/env bash

cd $(dirname "$0")

if [ -e "firmware.bin" ]; then
  rm firmware.bin
fi

particle compile photon project.properties src --saveTo firmware.bin
