#ifndef STC_DEVSELECTOR_H
#define STC_DEVSELECTOR_H


#include "SerialInterface.h"
#include "Kikan/renderer/stdRenderer/StdRenderer.h"

class DevSelector {
public:
    DevSelector(SerialInterface* sIF, uint64_t* timestamp);
    ~DevSelector();

    void render(Kikan::StdRenderer* renderer);

private:
    void render_dev_selectable(const char* dev);

    uint64_t* _timestamp;

    SerialInterface* _sif;
    bool _err_popup = false;
    int _errno = 0;
};


#endif //STC_DEVSELECTOR_H
