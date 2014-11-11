# the basics
import cxchan
import middlechan
import agregators

import middlerun

import sim_ctl
import osc
import phaseshifter
import phasemetr

import syn

# classes translations from modules
# main system-level class
cxchan = cxchan.cxchan

# middleware classes
middleChan = middlechan.middleChan
middleChanPoly = middlechan.middleChanPoly
middleChanRO = middlechan.middleChanRO
middleChanPolyRO = middlechan.middleChanPolyRO
middleChanEpics = middlechan.middleChanEpics

# middleware run control
middleRunCtl = middlerun.middleRunCtl


#agregators classes
middleSummer = agregators.middleSummer
maskAgregatorEpics = agregators.maskAgregatorEpics

IE4 = syn.IE4

adc200 = osc.adc200

phasemetr = phasemetr.phasemetr

PhaseShifterB = phaseshifter.PhaseShifterB

sim_ctl = sim_ctl.sim_ctl
