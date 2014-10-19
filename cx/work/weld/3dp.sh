#!/bin/zsh

setalloff()
{
	~/pult/bin/cdrclient weldcc \
		pause 100 \
		set sync.bfp.ctl.work=0 \
		set weld.movements.astop=1 \
		set weld.movements.bstop=1 \
		set weld.movements.rstop=1 \
		pause 1000
}

onint()
{
	echo INTERRUPT
	setalloff
}
trap onint INT

######################################################################

VB=32		# deg/s
HEATS=3		# rotations for preheat
ROUNDS=20	#
WIRESPD=5.0	# wire speed, mm/s
#DELTAT=2000
DELTADEG=30
ILENSDYN=0.6310
ILENSDYNINCR=0.001379
BEAMCUR=6.00
BEAMCURDECR=0.25
BEAMCURDECRRNOUNS=8
BEAMLOW=4.5
VERTSHIFT=0.6
VERTSPEED=0.5

while test -n "$1"
do               
        echo $1  
	export $1        
        shift    
done             
TROUND=$[360.0/$VB]
####VERTSPEED=$[$VERTSHIFT/($TROUND)]
DELTAT=$[( ($VERTSHIFT)/($VERTSPEED) + ($DELTADEG)*1.0/($VB) )*1000]

if true
then
~/pult/bin/cdrclient weldcc \
	set weld.movements.a-smc8-subwin.controls.start_frq=$VERTSPEED \
	set weld.movements.a-smc8-subwin.controls.final_frq=$VERTSPEED \
	set weld.movements.a-smc8-subwin.controls.accel=$VERTSPEED \
	set weld.movements.anumsteps=-$VERTSHIFT \
	\
	set weld.movements.b-smc8-subwin.controls.start_frq=$VB \
	set weld.movements.b-smc8-subwin.controls.final_frq=$VB \
	set weld.movements.b-smc8-subwin.controls.accel=$VB \
	set weld.movements.bnumsteps=$[(1+$HEATS+$ROUNDS)*360] \
	\
	set weld.movements.r-485-subwin.controls.config.vel.min_vel=$WIRESPD \
	set weld.movements.r-485-subwin.controls.config.vel.max_vel=$WIRESPD \
	set weld.movements.r-485-subwin.controls.config.accel=$WIRESPD \
	set weld.movements.rnumsteps=-$[$WIRESPD*$TROUND*$ROUNDS] \
        \
        set gunctl.stab.i_cat_bot_s=$BEAMCUR \
        \
	pause 1000 \
	\
	set weld.movements.bgo=1 \
	pause $[$TROUND*1000] \
	set sync.bfp.ctl.work=1 \
	pause $[$TROUND*$HEATS*1000] \
	set weld.movements.rgo=1 \
	pause $[$TROUND*1000-$DELTAT] \
	$(perl -e 'foreach $x (0 ..'$[$ROUNDS-1]'){$bc='$BEAMCUR'-$x*'$BEAMCURDECR'; ($bc < '$BEAMLOW') && ($bc='$BEAMLOW'); $i='$ILENSDYN'+$x*'$ILENSDYNINCR'; print "set gunctl.stab.i_cat_bot_s=$bc "; print "set weld.movements.ago=1 set magsys.is.i_ld_s=$i pause '$[$TROUND*1000]' ";}')
else
~/pult/bin/cdrclient weldcc \
	set weld.movements.a-smc8-subwin.controls.start_frq=$VERTSPEED \
	set weld.movements.a-smc8-subwin.controls.final_frq=$VERTSPEED \
	set weld.movements.a-smc8-subwin.controls.accel=$VERTSPEED \
	set weld.movements.anumsteps=-$[VERTSHIFT*$ROUNDS] \
	\
	set weld.movements.b-smc8-subwin.controls.start_frq=$VB \
	set weld.movements.b-smc8-subwin.controls.final_frq=$VB \
	set weld.movements.b-smc8-subwin.controls.accel=$VB \
	set weld.movements.bnumsteps=$[(1+$HEATS+$ROUNDS)*360] \
	\
	set weld.movements.r-485-subwin.controls.config.vel.min_vel=$WIRESPD \
	set weld.movements.r-485-subwin.controls.config.vel.max_vel=$WIRESPD \
	set weld.movements.r-485-subwin.controls.config.accel=$WIRESPD \
	set weld.movements.rnumsteps=-$[$WIRESPD*$TROUND*$ROUNDS] \
	pause 1000 \
	\
	set weld.movements.bgo=1 \
	pause $[$TROUND*1000] \
	set sync.bfp.ctl.work=1 \
	pause $[$TROUND*$HEATS*1000] \
	set weld.movements.rgo=1 set weld.movements.ago=1 \
	pause $[$TROUND*1000] \
	$(perl -e 'foreach $x (0 ..'$[$ROUNDS-1]'){$i='$ILENSDYN'+$x*0.001; print "set magsys.is.i_ld_s=$i pause '$[$TROUND*1000]' ";}')
fi

echo FINISH
setalloff
