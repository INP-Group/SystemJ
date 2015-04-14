#!/bin/bash

source ~/Develop/venv/system/bin/activate

isort -rc --atomic -e -w 79 -af -sl -cs -tc ./
pyformat -r -a -i -j 4  ./

deactivate
