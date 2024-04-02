#ifndef STC_SNIFFER_H
#define STC_SNIFFER_H

#include <utility>

#include "Kikan/renderer/stdRenderer/StdRenderer.h"
#include "Kikan/Engine.h"
#include "Buffer.h"
#include "SerialInterface.h"
#include "Profile.h"

struct DeviceInfo{
    explicit DeviceInfo(const std::string& path) {
        name = path.substr(5, path.size() - 5);
        this->path = path;
    }
    ~DeviceInfo() {
        delete sIf;
    }

    int open(){
        if(sIf)
            return 1;

        sIf = new SerialInterface;
        if(sIf->sIFopen(path.c_str()) == -1){
            delete sIf;
            sIf = nullptr;
            return -1;
        }

        return 1;
    }

    void close(){
        if(sIf){
            delete sIf;
            sIf = nullptr;
        }
    }

    uint8_t* read(std::string* src, uint32_t* len){
        if(muxed){
            uint32_t muxSrc = 0;
            uint8_t* data = sIf->sIFread(&muxSrc, len);
            *src = std::string(name + ":" + std::to_string(muxSrc));
            return data;
        }
        else{
            *src = std::string(name);
            return sIf->sIFread(len);
        }
    }

    bool exists = true;
    bool muxed = false;
    std::string name;
    std::string path;
    SerialInterface* sIf = nullptr;
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

    std::map<std::string, DeviceInfo*> _devs;
    std::map<std::string, Buffer*> _buffs;
    std::vector<PacketInfo*> _packets;

    void reset();

    bool _paused = false;
    std::string _profile_str;

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


    bool _dev_err_popup = false;
    int _dev_errno = 0;
    std::string _dev_error;

    bool _dev_sel = true;
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

    void render_dev();
    void render_dev_sel(DeviceInfo* dev);

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

    void save_profiles();
    void load_profiles();
};


#endif //STC_SNIFFER_H
