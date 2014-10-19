
# 1:c1 2:c2
define(`MAGX_C100',
       `"_all_code;
         getchan $1; cmp_if_eq 0; ret 0;
         getchan $1; getchan $2; div; sub 1; abs; mul 100; ret"')

# 1:c1 2:c2 3:c_s
define(`MAGX_C100_OPR',
       `"_all_code;
         getchan $3; cmp_if_eq 0; ret 0;
         getchan $1; cmp_if_eq 0; ret 0;
         getchan $1; getchan $2; div; sub 1; abs; mul 100; ret"')

# 1:c1 2:c2 3:c_s
define(`MAGX_CFWEIRD100',
       `"_all_code;
         getchan $3; cmp_if_eq 0; ret 0;
         getchan $1; cmp_if_eq 0; ret 0;
         getchan $1; getchan $2; div; sub 1; abs; mul 100;
         dup; div 1.0 ifelse(comment: "1.0" is 1%); trunc; weird;
         ret"')

define(`RGSWCH',
       `knob $1 $2 choicebs nolabel items:"#T$3\b\blit=red\t$4\b\blit=green" \
               r:$5 w:"_all_code; qryval; putchan $7; \
               push 1; qryval; sub; putchan $6;"')

# 1:id 2:label 3:tip 4:*_src-base 5:max 6:step
define(`MAGX_XCH300_LINE',
       `knob/groupable $1_set $2   text dpyfmt:"%6.1f" r:"$4_set" \
                alwdrange:0-$5 options:nounits
        disp           $1_mes $2_m text dpyfmt:"%6.1f" r:"$4_mes" \
                normrange:0-1 yelwrange:0-2 disprange:0-$5 \
                c:MAGX_C100_OPR($4_set, $4_mes, $4_opr)
        minmax         $1_dvn ""   text dpyfmt:"%6.3f" r:"$4_dvn"
        disp           $1_opr ""   led  shape=circle   r:"$4_opr"
        RGSWCH(-$1_swc, "", 0, 1, -$1_swc@r0, -$1_swc@w1, -$1_swc@w0)
        container -$1_ctl "..." subwin str3:$2": extended controls" \
                base:-$1_ctl content:1 {
            container "" "" grid nodecor 3 content:3 {
                container v3h "VCh300" grid noshadow,nocoltitles,norowtitles \
                        base:v3h param3:4 content:eval(4+3) {
                    disp   opr ""   led  shape=circle   r:opr
                    RGSWCH(onoff, "", Off, On, onoff@r0, onoff@w1, onoff@w0)
                    disp   v3h_state "I_S" text dpyfmt:"%3.0f" r:v3h_state
                    button rst       "R"   button              r:rst
                    container ctl "Control" grid notitle,noshadow,norowtitles \
                            3 base:ctl content:3 {
                        knob dac    "Set, A"    dpyfmt:"%7.1f" r:dac    alwdrange:0-$5
                        knob dacspd "MaxSpd, A" dpyfmt:"%7.1f" r:dacspd disprange:-5000-+5000
                        disp daccur "Cur, A"    dpyfmt:"%7.1f" r:daccur disprange:-2500-+2500
                    }
                    noop - - hseparator layinfo:horz=fill
                    container "" "" grid nodecor 3 content:3 {
                        container "mes" "Measurements" grid noshadow,nocoltitles \
                                base:mes content:5 {
                            disp dms "DAC control"         text - "A" "%9.1f" r:dms
                            disp sts "Stab. shunt (DCCT2)" text - "A" "%9.1f" r:sts
                            disp mes "Meas. shunt (DCCT1)" text - "A" "%9.1f" r:mes
                            disp u1  "U1"                  text - "V" "%9.1f" r:u1
                            disp u2  "U2"                  text - "V" "%9.6f" r:u2
                        }
                        noop - - vseparator layinfo:vert=fill
                        disp ilk "Ilk" led "shape=circle,color=red"   r:ilk  yelwrange:-0.1-+0.1
                    }
                }
                noop - - vseparator layinfo:vert=fill
                container c40d16 "XCANADC40+XCANDAC16" grid noshadow,nocoltitles,norowtitles \
                        base:c40d16 content:3 {
                    container dac "DAC" grid notitle,noshadow,norowtitles \
                            3 base:dac content:3 {
                        disp dac    "Set, V"      text dpyfmt:"%7.4f" r:dac
                        disp dacspd "MaxSpd, V/s" text dpyfmt:"%7.4f" r:dacspd
                        disp daccur "Cur, V"      text dpyfmt:"%7.4f" r:daccur
                    }
                    noop - - hseparator layinfo:horz=fill
                    container ":" "" grid nodecor \
                            3 content:3 {
                        container mes "ADC, V" grid noshadow,nocoltitles \
                                base:mes content:5 {
                            disp adc0 "+0"   dpyfmt:"%9.6f" r:adc0 disprange:-10-+10
                            disp adc1 "+1"   dpyfmt:"%9.6f" r:adc1 disprange:-10-+10
                            disp adc2 "+2"   dpyfmt:"%9.6f" r:adc2 disprange:-10-+10
                            disp adc3 "+3"   dpyfmt:"%9.6f" r:adc3 disprange:-10-+10
                            disp adc4 "+4"   dpyfmt:"%9.6f" r:adc4 disprange:-10-+10
                        }
                        noop - - vseparator layinfo:vert=fill
                        container io "I/O" grid noshadow,norowtitles,transposed \
                                str1:"DAC\tADC" \
                                base:io param1:2 content:4 {
                            disp i0 "" led shape=circle            r:i0
                            disp i1 "" led shape=circle            r:i1
                            disp o0 "" led shape=circle,color=blue r:o0
                            disp o1 "" led shape=circle,color=blue r:o1
                        }
                    }
                }
            }
        }
        ')


define(`MAGX_X1K_XCDAC20_ILK_LINE',
       `disp $1 $2 led "shape=circle,color=red"   r:$1  yelwrange:-0.1-+0.1
        disp -  "" led "shape=circle,color=amber" r:$1_c')

# 1:id 2:label 3:tip 4:*_src-base 5:max 6:step
define(`MAGX_X1K_XCDAC20_LINE',
       `knob/groupable $1_set $2   text dpyfmt:"%6.1f" r:"$4_set" \
                alwdrange:0-$5 options:nounits
        disp           $1_mes $2_m text dpyfmt:"%6.1f" r:"$4_mes" \
                normrange:0-1 yelwrange:0-2 disprange:0-$5 \
                c:MAGX_C100_OPR($4_set, $4_mes, $4_opr)
        minmax         $1_dvn ""   text dpyfmt:"%6.3f" r:"$4_dvn"
        disp           $1_opr ""   led  shape=circle   r:"$4_opr"
        RGSWCH(-$1_swc, "", 0, 1, -$1_swc@r0, -$1_swc@w1, -$1_swc@w0)
        container -$1_ctl "..." subwin str3:$2": extended controls" \
                base:-$1_ctl content:1 {
            container "" "" grid nodecor 3 content:3 {
                container v1k "V1000" grid noshadow,nocoltitles,norowtitles \
                        base:v1k param3:4 content:eval(4+3) {
                    disp   opr ""   led  shape=circle   r:opr
                    RGSWCH(onoff, "", Off, On, onoff@r0, onoff@w1, onoff@w0)
                    disp   v1k_state "I_S" text dpyfmt:"%3.0f" r:v1k_state
                    button rst       "R"   button              r:rst
                    container ctl "Control" grid notitle,noshadow,norowtitles \
                            3 base:ctl content:3 {
                        knob dac    "Set, A"    dpyfmt:"%7.1f" r:dac    alwdrange:0-2500
                        knob dacspd "MaxSpd, A" dpyfmt:"%7.1f" r:dacspd disprange:-2500-+2500
                        disp daccur "Cur, A"    dpyfmt:"%7.1f" r:daccur disprange:-2500-+2500
                    }
                    noop - - hseparator layinfo:horz=fill
                    container "" "" grid nodecor 3 content:3 {
                        container "mes" "Measurements" grid noshadow,nocoltitles \
                                base:mes content:8 {
                            disp current1m    "Current1M"    text - "A" "%9.1f" r:current1m 
                            disp voltage2m    "Voltage2M"    text - "A" "%9.1f" r:voltage2m  
                            disp current2m    "Current2M"    text - "A" "%9.1f" r:current2m  
                            disp outvoltage1m "OutVoltage1M" text - "V" "%9.1f" r:outvoltage1m
                            disp outvoltage2m "OutVoltage2M" text - "V" "%9.6f" r:outvoltage2m
                            disp adcdac       "DAC@ADC"      text - "A" "%9.1f" r:adcdac       disprange:-2500-+2500
                            disp 0v           "0V"           text - "V" "%9.6f" r:0v           disprange:-10-+10
                            disp 10v          "10V"          text - "V" "%9.6f" r:10v          disprange:-10-+10
                        }
                        noop - - vseparator layinfo:vert=fill
                        container "ilk" "Interlocks" grid \
                                noshadow,nocoltitles,norowtitles \
                                2 base:ilk content:10 {
                            MAGX_X1K_XCDAC20_ILK_LINE("overheat", "Overheating source")
                            MAGX_X1K_XCDAC20_ILK_LINE("emergsht", "Emergency shutdown")
                            MAGX_X1K_XCDAC20_ILK_LINE("currprot", "Current protection")
                            MAGX_X1K_XCDAC20_ILK_LINE("loadovrh", "Load overheat/water")
                            button rst "Reset" button r:rst
                            button rci "R"     button r:rci
                        }
                    }
                }
                noop - - vseparator layinfo:vert=fill
                container adc "X-CDAC20" grid noshadow,nocoltitles,norowtitles \
                        base:xcdac20 content:3 {
                    container dac "DAC" grid notitle,noshadow,norowtitles \
                            3 base:dac content:3 {
                        disp dac    "Set, V"      text dpyfmt:"%7.4f" r:dac
                        disp dacspd "MaxSpd, V/s" text dpyfmt:"%7.4f" r:dacspd
                        disp daccur "Cur, V"      text dpyfmt:"%7.4f" r:daccur
                    }
                    noop - - hseparator layinfo:horz=fill
                    container ":" "" grid nodecor \
                            3 content:3 {
                        container mes "ADC, V" grid noshadow,nocoltitles \
                                base:mes content:8 {
                            disp adc0 "0"   dpyfmt:"%9.6f" r:adc0 disprange:-10-+10
                            disp adc1 "1"   dpyfmt:"%9.6f" r:adc1 disprange:-10-+10
                            disp adc2 "2"   dpyfmt:"%9.6f" r:adc2 disprange:-10-+10
                            disp adc3 "3"   dpyfmt:"%9.6f" r:adc3 disprange:-10-+10
                            disp adc4 "4"   dpyfmt:"%9.6f" r:adc4 disprange:-10-+10
                            disp adc5 "DAC" dpyfmt:"%9.6f" r:adc5 disprange:-10-+10
                            disp adc6 "0V"  dpyfmt:"%9.6f" r:adc6 disprange:-10-+10
                            disp adc7 "10V" dpyfmt:"%9.6f" r:adc7 disprange:-10-+10
                        }
                        noop - - vseparator layinfo:vert=fill
                        container io "I/O" grid noshadow,norowtitles,transposed \
                                str2:"0\t1\t2\t3\t4\t5\t6\t7" \
                                base:io param1:8 content:16 {
                            disp i0 "" led shape=circle            r:i0
                            disp i1 "" led shape=circle            r:i1
                            disp i2 "" led shape=circle            r:i2
                            disp i3 "" led shape=circle            r:i3
                            disp i4 "" led shape=circle            r:i4
                            disp i5 "" led shape=circle            r:i5
                            disp i6 "" led shape=circle            r:i6
                            disp i7 "" led shape=circle            r:i7
                            disp o0 "" led shape=circle,color=blue r:o0
                            disp o1 "" led shape=circle,color=blue r:o1
                            disp o2 "" led shape=circle,color=blue r:o2
                            disp o3 "" led shape=circle,color=blue r:o3
                            disp o4 "" led shape=circle,color=blue r:o4
                            disp o5 "" led shape=circle,color=blue r:o5
                            disp o6 "" led shape=circle,color=blue r:o6
                            disp o7 "" led shape=circle,color=blue r:o7
                        }
                    }
                }
            }
        }
        ')


define(`MAGX_IST_XCDAC20_ILK_LINE',
       `disp $1 $2 led "shape=circle,color=red"   r:$1  yelwrange:-0.1-+0.1
        disp -  "" led "shape=circle,color=amber" r:$1_c')

# 1:id 2:label 3:tip 4:*_src-base 5:max 6:step 7:weird_dcct2 8:reversable
define(`MAGX_IST_XCDAC20_LINE',
       `knob/groupable $1_set $2   text dpyfmt:"%6.1f" r:"$4_set" \
                alwdrange:0-$5 options:nounits
        disp           $1_mes $2_m text dpyfmt:"%6.1f" r:"$4_mes" \
                normrange:0-1 yelwrange:0-2 disprange:0-$5 \
                c:MAGX_C100_OPR($4_set, $4_mes, $4_opr)
        minmax         $1_dvn ""   text dpyfmt:"%6.3f" r:"$4_dvn"
        disp           $1_opr ""   led  shape=circle   r:"$4_opr" \
                c:"_all_code; getchan -$4_ctl.xcdac20.io.o1; getchan $4_opr;
                  sub; dup; weird; ret"
        RGSWCH(-$1_swc, "", 0, 1, -$1_swc@r0, -$1_swc@w1, -$1_swc@w0)
        container -$1_ctl "..." subwin str3:$2": extended controls" \
                base:-$1_ctl content:1 {
            container "" "" grid nodecor 3 content:3 {
                container ist "IST" grid noshadow,nocoltitles,norowtitles \
                        base:ist param3:4 content:eval(4+4+1) {
                    disp   opr ""   led  shape=circle   r:opr
                    RGSWCH(onoff, "", Off, On, onoff@r0, onoff@w1, onoff@w0)
                    disp   ist_state "I_S" text dpyfmt:"%3.0f" r:ist_state
                    button rst       "R"   button              r:rst
                    container ctl "Control" grid notitle,noshadow,norowtitles \
                            3 base:ctl content:3 {
                        knob dac    "Set, A"    dpyfmt:"%7.1f" r:dac    alwdrange:0-2500
                        knob dacspd "MaxSpd, A" dpyfmt:"%7.1f" r:dacspd disprange:-2500-+2500
                        disp daccur "Cur, A"    dpyfmt:"%7.1f" r:daccur disprange:-2500-+2500
                    }
                    noop - - hseparator layinfo:horz=fill
                    container "" "" grid nodecor 3 content:3 {
                        container "mes" "Measurements" grid noshadow,nocoltitles \
                                base:mes content:8 {
                            disp dcct1  "DCCT1"   text - "A" "%9.1f" r:dcct1   disprange:-2500-+2500
                            disp dcct2  "DCCT2"   text - "A" "%9.1f" r:dcct2   disprange:-2500-+2500 \
                                    ifelse($7, 1, c:MAGX_CFWEIRD100(dcct1,dcct2,:ctl.daccur))
                            disp dacmes "DAC"     text - "A" "%9.1f" r:dacmes  disprange:-2500-+2500
                            disp u2     "U2"      text - "V" "%9.1f" r:u2      disprange:-10-+10
                            disp adc4   "ADC4"    text - "V" "%9.6f" r:adc4    disprange:-10-+10
                            disp adcdac "DAC@ADC" text - "A" "%9.1f" r:adcdac  disprange:-2500-+2500
                            disp 0v     "0V"      text - "V" "%9.6f" r:0v      disprange:-10-+10
                            disp 10v    "10V"     text - "V" "%9.6f" r:10v     disprange:-10-+10
                        }
                        noop - - vseparator layinfo:vert=fill
                        container "ilk" "Interlocks" grid \
                                noshadow,nocoltitles,norowtitles \
                                2 base:ilk content:16 {
                            button rst "Reset" button r:rst
                            button rci "R"     button r:rci
                            MAGX_IST_XCDAC20_ILK_LINE("out",     "Out Prot")
                            MAGX_IST_XCDAC20_ILK_LINE("water",   "Water")
                            MAGX_IST_XCDAC20_ILK_LINE("imax",    "Imax")
                            MAGX_IST_XCDAC20_ILK_LINE("umax",    "Umax")
                            MAGX_IST_XCDAC20_ILK_LINE("battery", "Battery")
                            MAGX_IST_XCDAC20_ILK_LINE("phase",   "Phase")
                            MAGX_IST_XCDAC20_ILK_LINE("temp",    "Tempr")
                        }
                    }
                    noop - - hseparator layinfo:horz=fill
                    container "" "" grid nodecor 4 base::-srvc content:4 {
                        button calb    "Clbr DAC" button r:calb
                        knob   dc_mode "Dig.corr" onoff  r:dc_mode
                        disp   dc_val  ""         text dpyfmt:"%-8.0f" r:dc_val
                        disp   sign    "S:"       text withlabel dpyfmt:"%+2.0f" r:sign
                    }
                }
                noop - - vseparator layinfo:vert=fill
                container adc "X-CDAC20" grid noshadow,nocoltitles,norowtitles \
                        base:xcdac20 content:3 {
                    container dac "DAC" grid notitle,noshadow,norowtitles \
                            3 base:dac content:3 {
                        disp dac    "Set, V"      text dpyfmt:"%7.4f" r:dac
                        disp dacspd "MaxSpd, V/s" text dpyfmt:"%7.4f" r:dacspd
                        disp daccur "Cur, V"      text dpyfmt:"%7.4f" r:daccur
                    }
                    noop - - hseparator layinfo:horz=fill
                    container ":" "" grid nodecor \
                            3 content:3 {
                        container mes "ADC, V" grid noshadow,nocoltitles \
                                base:mes content:8 {
                            disp adc0 "0"   dpyfmt:"%9.6f" r:adc0 disprange:-10-+10
                            disp adc1 "1"   dpyfmt:"%9.6f" r:adc1 disprange:-10-+10
                            disp adc2 "2"   dpyfmt:"%9.6f" r:adc2 disprange:-10-+10
                            disp adc3 "3"   dpyfmt:"%9.6f" r:adc3 disprange:-10-+10
                            disp adc4 "4"   dpyfmt:"%9.6f" r:adc4 disprange:-10-+10
                            disp adc5 "DAC" dpyfmt:"%9.6f" r:adc5 disprange:-10-+10
                            disp adc6 "0V"  dpyfmt:"%9.6f" r:adc6 disprange:-10-+10
                            disp adc7 "10V" dpyfmt:"%9.6f" r:adc7 disprange:-10-+10
                        }
                        noop - - vseparator layinfo:vert=fill
                        container io "I/O" grid noshadow,norowtitles,transposed \
                                str2:"0\t1\t2\t3\t4\t5\t6\t7" \
                                base:io param1:8 content:16 {
                            disp i0 "" led shape=circle            r:i0
                            disp i1 "" led shape=circle            r:i1
                            disp i2 "" led shape=circle            r:i2
                            disp i3 "" led shape=circle            r:i3
                            disp i4 "" led shape=circle            r:i4
                            disp i5 "" led shape=circle            r:i5
                            disp i6 "" led shape=circle            r:i6
                            disp i7 "" led shape=circle            r:i7
                            disp o0 "" led shape=circle,color=blue r:o0
                            disp o1 "" led shape=circle,color=blue r:o1
                            disp o2 "" led shape=circle,color=blue r:o2
                            disp o3 "" led shape=circle,color=blue r:o3
                            disp o4 "" led shape=circle,color=blue r:o4
                            disp o5 "" led shape=circle,color=blue r:o5
                            disp o6 "" led shape=circle,color=blue r:o6
                            disp o7 "" led shape=circle,color=blue r:o7
                        }
                    }
                }
            }
        }
        ')


define(`MAGX_XOR4016_LINE',
       `knob/groupable look:text r:$1_set alwdrange:-10000-+10000 step:100 \
                options:nounits
        disp           look:text r:$1_mes normrange:0-5 yelwrange:0-7 \
                c:MAGX_C100($1_set, $1_mes)
        disp           look:text r:$1_u   disprange:-50-+50')

define(`MAGX_COR208_LINE',
       `knob/groupable look:text r:$1_set options:nounits
        disp           look:text r:$1_mes
        disp           look:text r:$1_u   yelwrange:-1-+24')
