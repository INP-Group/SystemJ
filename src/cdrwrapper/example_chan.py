#############################################
############### USE EXAMPLE #################
#############################################
if __name__ == "__main__":
    print "Example starts"

    # Special QT import, needs to make qt libs visible for Cdrlib
    import DLFCN
    old_dlopen_flags = sys.getdlopenflags( )
    sys.setdlopenflags( old_dlopen_flags | DLFCN.RTLD_GLOBAL )
    from PyQt4 import QtCore, QtGui
    sys.setdlopenflags( old_dlopen_flags )

    # QT init
    application = QtGui.QApplication(sys.argv)
    myapp = QtGui.QMainWindow(None)
    myapp.show()

    # Definig simple callback python callable
    def test_cb_py(handle, val, params):
        print "Python Callback:", handle, val, params
        return 0

    # CDR
    wrapper = cdr()
    wrapper.RegisterSimpleChan("test", callback, None) # Not None third parameter needs to be tested
    # Needs to be tested:
    # wrapper.CdrGetSimpleChanVal(10)
    # wrapper.CdrSetSimpleChanVal(10, 12.1)

    print "Example goes to main loop"
    # QT main loop
    sys.exit(application.exec_())
