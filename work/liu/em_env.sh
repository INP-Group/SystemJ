#!/bin/sh

export CX_TRANSLATE_RACK0_44=:10
for i in 1 2 3 4 5 6 7 8
do
    export CX_TRANSLATE_RACK${i}_43=:1$i
done
export CX_TRANSLATE_RACK0_45=:19
export CX_TRANSLATE_RACK0_46=:20
