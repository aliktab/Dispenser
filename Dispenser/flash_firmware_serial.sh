#!/usr/bin/env bash

cd $(dirname "$0")

if [ ! -f firmware.bin ]; then
  particle compile photon project.properties src --saveTo firmware.bin
fi

particle flash --serial firmware.bin
rm firmware.bin
