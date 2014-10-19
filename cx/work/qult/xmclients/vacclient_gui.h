#ifndef __VACCLIENT_GUI_H
#define __VACCLIENT_GUI_H


enum {
    cmSwitchView  = 4,
    cmSwitchScale = 5,
};

XhWindow  CreateApplicationWindow(int argc, char *argv[]);
void      RunApplication         (void);


#endif /* __VACCLIENT_GUI_H */
