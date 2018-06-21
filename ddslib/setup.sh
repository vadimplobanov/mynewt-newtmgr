#!/bin/bash

set -e

if [[ -e lib1 ]]; then rm lib1; fi
if [[ -e lib2 ]]; then rm lib2; fi

ln -s $1/repos/mynewt-dds-micro/libs/dds/src            lib1
ln -s $1/repos/mynewt-dds-micro/apps/dds_mgr/src/common lib2
