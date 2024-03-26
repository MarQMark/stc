#ifndef STC_SNIFFER_H
#define STC_SNIFFER_H

#include "Kikan/renderer/stdRenderer/StdRenderer.h"
#include "Kikan/Engine.h"
#include "Buffer.h"
#include "SerialInterface.h"
#include "DevSelector.h"

struct PacketInfo{
    PacketInfo() = default;
    PacketInfo(uint32_t src, uint64_t timestamp, Message* msg){
        this->src = src;
        this->timestamp = timestamp;
        this->msg = msg;
    };

    uint32_t src;
    uint64_t timestamp;
    Message* msg;
};

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

    std::map<uint32_t, Buffer*> _buffs;
    std::vector<PacketInfo*> _packets;
    SerialInterface _sif;

    void reset();

    bool _paused = false;

    uint64_t _start_timestamp = 0;
    void reset_start_timestamp();

    uint32_t _sel_msg = 0;
    uint32_t _sel_buf_src = 0;

    DevSelector* _devSelector;

    bool _view_msgs = true;
    bool _view_hex = true;
    bool _view_details = true;
    bool _view_buffers = true;

    void render_dockspace();
    void render_main();
    void render_menubar();
    void render_actionbar();
    void render_msgs();
    void render_hex();
    void render_details();
    void render_buffers();
};


#endif //STC_SNIFFER_H
