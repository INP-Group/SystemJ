from PyQt4.QtDesigner import QPyDesignerCustomWidgetPlugin
from PyQt4.QtGui import QIcon
from fcombobox import *
 
class FComboBoxWidgetPlugin(QPyDesignerCustomWidgetPlugin):
    def __init__(self, parent=None):
        QPyDesignerCustomWidgetPlugin.__init__(self)
 
    def name(self):
        return 'FComboBox'
 
    def group(self):
        return 'Fedor'
 
    def icon(self):
        return QIcon()
 
    def isContainer(self):
        return False
 
    def includeFile(self):
        return 'widgets.fcombobox'

    def toolTip(self):
        return 'a combobox adapted to BINP IC control system'
 
    def whatsThis(self):
        return 'a combobox adapted to BINP IC control system'
 
    def createWidget(self, parent):
        return FComboBox(parent)