from PyQt4.QtDesigner import QPyDesignerCustomWidgetPlugin

from fspinbox import *


class FSpinBoxWidgetPlugin(QPyDesignerCustomWidgetPlugin):
    def __init__(self, parent=None):
        QPyDesignerCustomWidgetPlugin.__init__(self)

    def name(self):
        return 'FSpinBox'

    def group(self):
        return 'Fedor'

    def icon(self):
        return QIcon()

    def isContainer(self):
        return False

    def includeFile(self):
        return 'widgets.fspinbox'

    def toolTip(self):
        return 'a spinbox adapted to BINP IC control system'

    def whatsThis(self):
        return 'a spinbox adapted to BINP IC control system'

    def createWidget(self, parent):
        return FSpinBox(parent)
