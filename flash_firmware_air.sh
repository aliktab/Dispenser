#!/usr/bin/env bash


cd $(dirname "$0")


flash_device()
{
  if [ ! -f firmware.bin ] || [ ! -e $2 ]; then
    echo 'compile '$1
    particle compile photon project.properties src --saveTo firmware.bin
  fi

  if [ -f firmware.bin ]; then
    echo 'flash '$1

    for dev_id in $(cat devices.lst)
    do
      if [[ $dev_id = '#'* ]]; then
        echo '  skip '$dev_id
        continue
      fi

      echo '  flash '$dev_id
      curl -X PUT -F file=@firmware.bin "https://api.particle.io/v1/devices/"$dev_id"?access_token=5236d3b07439604edc4ed5556bc573a86b6918c5"
      echo ' '
    done

    rm firmware.bin

  else

    echo There is no firmware.bin file

  fi
}


flash_device_cd()
{
  cd $1

  flash_device $1 $2

  cd ..
}


for arg in $@
do
  case $arg in
    '-c')  c_flg='true' ;;
  esac
done

flash_device_cd Dispenser $c_flg
