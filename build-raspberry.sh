#! /bin/bash

ninja -C builddir-raspi &&

rsync -av builddir-raspi/sfizz-tui lanpi-2:/home/alarm/sfizz-tui/build