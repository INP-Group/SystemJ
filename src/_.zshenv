
export PATH=$PATH:$HOME
export PATH=$PATH:"/home/femanov/control_system/qult/chlclients:/home/femanov/python/bin:/home/femanov/epics/3.15/bin/linux-x86_64"
export PATH=$PATH:"/home/femanov/python/pymiddle"

export PYTHONPATH=${PYTHONPATH}:"$HOME/control_system/cx/src/lib/4PyQt:$HOME/python/cothread"
export PYTHONPATH=${PYTHONPATH}:"$HOME/python:$HOME/python/lib64/python2.6/site-packages"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$HOME/control_system/cx/src/lib/4PyQt":"$HOME/epics/3.15/lib/linux-x86_64":"$HOME/python/npextras"

export PYQTDESIGNERPATH=$PYQTDESIGNERPATH:"$HOME/python/widgets"


export CHLCLIENTS_PATH="$HOME/control_system/qult/chlclients"


export EPICS_BASE=/$HOME/epics/3.15
export EPICS_HOST_ARCH=linux-x86_64

export EPICS_CA_AUTO_ADDR_LIST=NO
export EPICS_CA_ADDR_LIST="192.168.182.101 192.168.182.103"




#for elegant start
export RPN_DEFNS=/home/femanov/defns.rpn

