#!/bin/bash

HOME_FOLDER="/home/warmonger/Dropbox/Study/Diploma/Diploma_cx"
WORK_CX_FOLDER=$HOME_FOLDER/work
CONFIG_FOLDER=$WORK_CX_FOLDER/cx/exports/configs
SBIN_FOLDER=$WORK_CX_FOLDER/cx/exports/sbin
QULT_FOLDER=$WORK_CX_FOLDER/qult/configs

cd $WORK_CX_FOLDER/cx/src/lib/4PyQt

export CHLCLIENTS_PATH=$WORK_CX_FOLDER/qult/chlclients; export CX_TRANSLATE_LINAC1_34=:34; python ./cdr_wrapper.py



# export CHLCLIENTS_PATH=/home/warmonger/Dropbox/Study/Diploma/Diploma_cx/work/qult/chlclients; export CX_TRANSLATE_LINAC1_34=:34; ./linmagx

