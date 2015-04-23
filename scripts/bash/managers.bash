#!/bin/bash

source ~/Develop/venv/journal/bin/activate

for i in {1..5}
do
   python ./bin/manager.py &
done