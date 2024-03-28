#ifndef STC_SNIFFER_H
#define STC_SNIFFER_H

#include "Kikan/renderer/stdRenderer/StdRenderer.h"
#include "Kikan/Engine.h"
#include "Buffer.h"
#include "SerialInterface.h"
#include "DevSelector.h"
#include "Profile.h"

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

    uint32_t _sel_profile = 0;
    char _new_profile[24];
    char _new_id[24] = {"0"};
    char _new_type[24] = {""};
    char _new_type_len[24] = {"1"};
    std::string _sel_profile_str = " ";
    uint32_t _sel_profile_i = 0;
    uint32_t _sel_profile_id = 0;
    uint32_t _sel_profile_new_type = 0;
    uint32_t _sel_profile_type = 0;
    std::map<std::string, Profile*> _profiles;


    DevSelector* _devSelector;

    bool _view_msgs = true;
    bool _view_hex = true;
    bool _view_details = true;
    bool _view_buffers = true;
    bool _view_profiles = false;
    bool _view_profiles_add = false;
    bool _view_profiles_rm = false;
    bool _view_profiles_id_add = false;
    bool _view_profiles_id_rm = false;
    bool _view_profiles_type_add = false;
    bool _view_profiles_type_rm = false;

    void render_dockspace();
    void render_main();
    void render_menubar();
    void render_actionbar();
    void render_msgs();
    void render_hex();
    void render_details();
    void render_buffers();
    void render_profiles();
    void render_profile_add();
    void render_profile_rm();
    void render_profile_id_add();
    void render_profile_id_rm();
    void render_profile_type_add();
    void render_profile_type_rm();
};


#endif //STC_SNIFFER_H
