#!/bin/bash

pyformat -r -a -i -j 4 ./
isort -rc --atomic .
