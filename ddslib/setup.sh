#!/bin/bash

set -e

if [[ -e lib1 || -h lib1 ]]; then rm -f lib1; fi
if [[ -e lib2 || -h lib2 ]]; then rm -f lib2; fi

ln -s $1/repos/mynewt-dds-micro/libs/dds/src            lib1
ln -s $1/repos/mynewt-dds-micro/apps/dds_mgr/src/common lib2
