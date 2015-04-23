#!/bin/bash

source ~/Develop/venv/system/bin/activate
cd ../../

isort -rc --atomic -e -w 79 -af -m2 -cs -tc ./
pyformat -r -a -i -j 4  ./
yapf -ri ./


deactivate
