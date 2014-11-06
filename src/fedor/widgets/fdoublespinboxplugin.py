from PyQt4.QtDesigner import QPyDesignerCustomWidgetPlugin
from PyQt4.QtGui import QIcon
from fdoublespinbox import *


class FDoubleSpinBoxWidgetPlugin(QPyDesignerCustomWidgetPlugin):
    def __init__(self, parent=None):
        QPyDesignerCustomWidgetPlugin.__init__(self)
 
    def name(self):
        return 'FDoubleSpinBox'
 
    def group(self):
        return 'Fedor'
 
    def icon(self):
        return QIcon()
 
    def isContainer(self):
        return False
 
    def includeFile(self):
        return 'widgets.fdoublespinbox'
 
    def toolTip(self):
        return 'a double spinbox adapted to BINP IC control system'
 
    def whatsThis(self):
        return 'a double spinbox adapted to BINP IC control system'
 
    def createWidget(self, parent):
        return FDoubleSpinBox(parent)