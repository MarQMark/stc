#ifndef STC_SNIFFER_H
#define STC_SNIFFER_H

#include "Kikan/renderer/stdRenderer/StdRenderer.h"
#include "Kikan/Engine.h"
#include "Buffer.h"
#include "SerialInterface.h"
#include "DevSelector.h"

class Sniffer : Kikan::StdRenderer::Override{
public:
    Sniffer();
    ~Sniffer();

    void update();
    bool running;

private:
    void preRender(Kikan::StdRenderer* renderer, double dt) override;
    void postRender(Kikan::StdRenderer* renderer, double dt) override;

    Kikan::Engine* _engine;
    Kikan::StdRenderer* _renderer;
    Buffer _buff;
    SerialInterface _sif;

    DevSelector devSelector = NULL;

    bool _dev_error_popup = false;

    void render_dockspace();
};


#endif //STC_SNIFFER_H
