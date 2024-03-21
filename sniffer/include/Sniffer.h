#ifndef STC_SNIFFER_H
#define STC_SNIFFER_H

#include "Kikan/renderer/stdRenderer/StdRenderer.h"
#include "Kikan/Engine.h"
#include "Buffer.h"

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
    Buffer _buff;

    void render_dockspace();
};


#endif //STC_SNIFFER_H
