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
    std::map<uint32_t, Buffer> _buffs;
    SerialInterface _sif;

    DevSelector devSelector = NULL;


    bool _view_msgs = true;
    bool _view_hex = true;
    bool _view_details = true;

    void render_dockspace();
    void render_menubar();
    void render_actionbar();
    void render_msgs();
    void render_hex();
    void render_details();
};


#endif //STC_SNIFFER_H
