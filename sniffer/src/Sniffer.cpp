#include <sstream>
#include <dirent.h>
#include <iomanip>
#include <glob.h>
#include "Sniffer.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Kikan/renderer/stdRenderer/Camera.h"
#include "imgui/imgui_internal.h"
#include "IconFontAwesome/IconsFontAwesome5.h"

Sniffer::Sniffer() {
    Kikan::Engine::init();
    _engine = Kikan::Engine::Kikan();

    running = true;

    _renderer = (Kikan::StdRenderer*)_engine->getRenderer();
    Kikan::Camera camera;
    _renderer->mvp = camera.matrix();
    _renderer->overrideRender(this);

    // Setup ImGUI
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(((Kikan::StdRenderer*)_engine->getRenderer())->getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 430");


    // Setup Icon Font
    io.Fonts->AddFontDefault();

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    std::string fontFile("assets/");
    fontFile += FONT_ICON_FILE_NAME_FAS;
    io.Fonts->AddFontFromFileTTF( fontFile.c_str(), 11.0f, &icons_config, icons_ranges );
}

Sniffer::~Sniffer() {
    delete _engine;
}

void Sniffer::update() {
    running = _engine->shouldRun();
    _engine->update();
}

void Sniffer::preRender(Kikan::StdRenderer *renderer, double dt) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(_sif.getFD() != -1){
        uint8_t* data = nullptr;
        uint32_t src = 0;
        uint32_t len = 0;
        if((data = _sif.sIFread(&src, &len)) != nullptr){
            if(_buffs.count(src) == 0)
                _buffs[src] = new Buffer;

            if(!_paused)
                _buffs[src]->addData(data, len);
        }

        for (auto buffer : _buffs) {
            std::vector<Message*> msgs;
            buffer.second->parseMsgs(&msgs);
            for(auto* msg : msgs){
                struct timespec spec;
                clock_gettime(CLOCK_REALTIME, &spec);
                uint64_t timestamp = ((spec.tv_nsec/ 1000000) + (spec.tv_sec * 1000)) - _start_timestamp;

                _packets.push_back(new PacketInfo(src, timestamp, msg));
            }
        }
        render_dockspace();
    }
    else{
        render_dev();

        if(_sif.getFD() != -1){
            load_profiles();
        }
    }
}

void Sniffer::postRender(Kikan::StdRenderer *renderer, double dt) {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Sniffer::render_dockspace() {
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);



    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    render_main();

    ImGui::End();
}

void Sniffer::render_main(){
    render_menubar();
    render_actionbar();
    if(_view_msgs)
        render_msgs();
    if(_view_hex)
        render_hex();
    if(_view_details)
        render_details();
    if(_view_buffers)
        render_buffers();
    render_profiles();

    //bool show_demo_window = true;
    //ImGui::ShowDemoWindow(&show_demo_window);
}

void Sniffer::render_menubar() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    if(ImGui::BeginViewportSideBar("##MainStatusBar", NULL, ImGuiDir_Up, height, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Profiles", nullptr, &_view_profiles);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Messages", nullptr, &_view_msgs);
                ImGui::MenuItem("Hex", nullptr, &_view_hex);
                ImGui::MenuItem("Details", nullptr, &_view_details);
                ImGui::MenuItem("Buffers", nullptr, &_view_buffers);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
}

void Sniffer::render_actionbar() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight() * 1.5;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7, 7));
    if(ImGui::BeginViewportSideBar("##SecondaryMenuBar", NULL, ImGuiDir_Up, height, window_flags)) {

        if (ImGui::BeginMenuBar()) {

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
            _paused = _paused != ImGui::Button(_paused ? ICON_FA_PLAY : ICON_FA_PAUSE, ImVec2(20, 20));

            if(ImGui::Button(ICON_FA_REDO_ALT, ImVec2(20, 20))){
                reset();
            }

            ImGui::Separator();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
            ImGui::SetNextItemWidth(200);
            std::vector<std::string> profiles;
            uint32_t n = 0, pos = 0;
            for(const auto& pair : _profiles){
                profiles.push_back(pair.first);
                if(pair.first == _profile_str)
                    pos = n;
                n++;
            }

            if (ImGui::BeginCombo("Profiles##ProSel", _profile_str.c_str()))
            {
                for (int i = 0; i < profiles.size(); i++)
                {
                    if (ImGui::Selectable(profiles[i].c_str(), i == pos)){
                        _profile_str = profiles[i];
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleVar();
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
    ImGui::PopStyleVar();
}

void Sniffer::render_msgs() {
    ImGui::Begin("Messages");

    if(ImGui::IsWindowFocused()){
        if(_engine->getInput()->keyXPressed(Kikan::Key::DOWN) && _sel_msg < _packets.size() - 1)
            _sel_msg++;
        else if(_engine->getInput()->keyXPressed(Kikan::Key::UP) && _sel_msg > 0)
            _sel_msg--;
    }

    ImGui::Text("No");
    ImGui::SameLine(50);
    ImGui::Text("Src");
    ImGui::SameLine(100);
    ImGui::Text("Time");
    ImGui::SameLine(200);
    ImGui::Text("Value");


    ImGui::Separator();
    // Rows
    for(int i = 0; i < _packets.size(); i++){
        auto* packetInfo = _packets[i];
        auto* msg = packetInfo->msg;

        bool isSelected = ImGui::Selectable((std::to_string(i) + "##Row" + std::to_string(i)).c_str(), _sel_msg == i);
        ImGui::SameLine(50);
        ImGui::Selectable((std::to_string(packetInfo->src) + "##Row" + std::to_string(i)).c_str());
        ImGui::SameLine(100);
        ImGui::Selectable((std::to_string(packetInfo->timestamp) + "##Row" + std::to_string(i)).c_str());
        ImGui::SameLine(200);

        char buf[256];
        sprintf(buf, "ID: %d(0x%X) Len: %u(0x%X) TCN: %u(0x%X)", msg->hdr.id, msg->hdr.id, msg->hdr.len, msg->hdr.len, msg->hdr.tcn, msg->hdr.tcn);
        std::stringstream ss;
        ss << buf;
        if(msg->hdr.tcf & TCF_SYN)
            ss << " SYN";
        else if(msg->hdr.tcf & TCF_ACK)
            ss << " ACK";
        if(msg->hdr.tcf & TCF_RST)
            ss << " RST";

        ss << "##Row" << i;

        ImGui::Selectable(ss.str().c_str());

        if (isSelected) {
            _sel_msg = i;
        }
    }

    ImGui::End();
}

char to_ascii(uint8_t val){
    if(val > 31 && val < 127)
        return (char)val;

    return '.';
}

void format_hex(char* str, uint32_t addr, uint8_t *data, uint32_t len){
    uint8_t* tmp = (uint8_t*)malloc(0x10);
    memset(tmp, 0, 0x10);
    memcpy(tmp, data, len);

    sprintf(str, "%04X | %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X  |%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c|",
            addr,
            tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7],
            tmp[8], tmp[9], tmp[10], tmp[11], tmp[12], tmp[13], tmp[14], tmp[15],
            to_ascii(tmp[0]), to_ascii(tmp[1]), to_ascii(tmp[2]), to_ascii(tmp[3]), to_ascii(tmp[4]), to_ascii(tmp[5]),
            to_ascii(tmp[6]),
            to_ascii(tmp[7]),
            to_ascii(tmp[8]), to_ascii(tmp[9]), to_ascii(tmp[10]), to_ascii(tmp[11]), to_ascii(tmp[12]),
            to_ascii(tmp[13]),
            to_ascii(tmp[14]),
            to_ascii(tmp[15]));

    free(tmp);
}

void Sniffer::render_hex() {
    ImGui::Begin("Hex View");

    if(_packets.empty()){
        ImGui::End();
        return;
    }

    Message* msg = _packets[_sel_msg]->msg;
    uint32_t addr = 0;
    char str[128];

    ImGui::Text("Magic Number");
    format_hex(str, addr, (uint8_t*)&msg->magic, 4);
    ImGui::Text("%s", str);
    ImGui::NewLine();
    addr += 0x10;

    ImGui::Text("Header");
    format_hex(str, addr, (uint8_t*)&msg->hdr, sizeof(Header));
    ImGui::Text("%s", str);
    ImGui::NewLine();
    addr += 0x10;

    ImGui::Text("Body");
    for (int i = 0; i < msg->hdr.len; i += 16) {
        format_hex(str, addr, msg->bdy.data + i, std::min(16, msg->hdr.len - i));
        ImGui::Text("%s", str);
        addr += 0x10;
    }

    ImGui::End();
}

void Sniffer::render_details() {
    ImGui::Begin("Details");

    if(_packets.empty()){
        ImGui::End();
        return;
    }
    Message* msg = _packets[_sel_msg]->msg;

    char item[64];
    sprintf(item, "Magic Number: 0x%04X", msg->magic);
    if (ImGui::TreeNode(item)) {
        ImGui::Text("\t0x%04X (%d)", msg->magic, msg->magic);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Header")) {
        sprintf(item, "ID: %d###id", msg->hdr.id);
        if (ImGui::TreeNode(item)) {
            ImGui::Text("\t0x%02X (%d)", msg->hdr.id, msg->hdr.id);
            ImGui::TreePop();
        }

        sprintf(item, "Length: %d###len", msg->hdr.len);
        if (ImGui::TreeNode(item)) {
            ImGui::Text("\t0x%02X (%d)", msg->hdr.len, msg->hdr.len);
            ImGui::TreePop();
        }

        sprintf(item, "TCN: %u###tcn", msg->hdr.tcn);
        if (ImGui::TreeNode(item)) {
            ImGui::Text("\t0x%04X (%u)", msg->hdr.tcn, msg->hdr.tcn);
            ImGui::TreePop();
        }

        sprintf(item, "TCF: %d###tcf", msg->hdr.tcf);
        if (ImGui::TreeNode(item)) {
            if(msg->hdr.tcf & TCF_SYN)
                ImGui::Text("\t.......1 SYN");
            else if(msg->hdr.tcf & TCF_ACK)
                ImGui::Text("\t......1. ACK");
            if(msg->hdr.tcf & TCF_RST)
                ImGui::Text("\t.....1.. RST");

            ImGui::Text("\t0x%01X (%d)", msg->hdr.tcf, msg->hdr.tcf);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Body")) {

        if(_profiles.count(_profile_str) != 0){
            if(_profiles[_profile_str]->types.count(msg->hdr.id) != 0){
                std::vector<ProfileType*> types = *_profiles[_profile_str]->types[msg->hdr.id];
                uint32_t off = 0;
                for(int i = 0; i < types.size(); i++){
                    if(off + types[i]->getType() >= msg->hdr.len)
                        break;

                    switch (types[i]->getType()) {
                        case 0:
                            ImGui::Text("%s (%s): 0x%02X(%u)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(msg->bdy.data + off), *(msg->bdy.data + off));
                            break;
                        case 1:
                            ImGui::Text("%s (%s): 0x%04X(%u)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(uint16_t*)(msg->bdy.data + off), *(uint16_t*)(msg->bdy.data + off));
                            break;
                        case 2:
                            ImGui::Text("%s (%s): 0x%08X(%u)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(uint32_t*)(msg->bdy.data + off), *(uint32_t*)(msg->bdy.data + off));
                            break;
                        case 3:
                            ImGui::Text("%s (%s): 0x%16lX(%lu)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(uint64_t*)(msg->bdy.data + off), *(uint64_t*)(msg->bdy.data + off));
                            break;
                        case 4:
                            ImGui::Text("%s (%s): 0x%02X(%d)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(int8_t*)(msg->bdy.data + off), *(int8_t*)(msg->bdy.data + off));
                            break;
                        case 5:
                            ImGui::Text("%s (%s): 0x%04X(%d)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(int16_t*)(msg->bdy.data + off), *(int16_t*)(msg->bdy.data + off));
                            break;
                        case 6:
                            ImGui::Text("%s (%s): 0x%08X(%d)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(int32_t*)(msg->bdy.data + off), *(int32_t*)(msg->bdy.data + off));
                            break;
                        case 7:
                            ImGui::Text("%s (%s): 0x%16lX(%ld)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(int64_t*)(msg->bdy.data + off), *(int64_t*)(msg->bdy.data + off));
                            break;

                        case 8:
                            ImGui::Text("%s (%s): 0x%08X(%f)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(uint32_t*)(msg->bdy.data + off), *(float*)(msg->bdy.data + off));
                            break;
                        case 9:
                            ImGui::Text("%s (%s): 0x%16lX(%lf)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(uint64_t*)(msg->bdy.data + off), *(double*)(msg->bdy.data + off));
                            break;
                        case 10:
                            ImGui::Text("%s (%s): 0x%02X(%c)", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], *(msg->bdy.data + off), *(char*)(msg->bdy.data + off));
                            break;

                        case 11:{
                            char str[types[i]->getLen() + 1];
                            strncpy(str, (const char*)(msg->bdy.data + off), types[i]->getLen());
                            str[types[i]->getLen()] = '\0';
                            ImGui::Text("%s (%s): %s", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], str);
                            break;
                        }

                        case 12:{
                            std::stringstream ss;
                            for (int j = 0; j < types[i]->getLen(); ++j) {
                                ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << *(msg->bdy.data + off + j) << " ";
                            }
                            ImGui::Text("%s (%s): %s", types[i]->getName().c_str(), ProfileType::typeStr[types[i]->getType()], ss.str().c_str());
                        }
                    }
                    off += types[i]->getType();
                }
            }
            else{
                ImGui::Text("Id(%d) not in profile %s", msg->hdr.id, _profile_str.c_str());
            }
        }
        else{
            ImGui::Text("No Profile selected");
        }


        ImGui::TreePop();
    }

    ImGui::End();
}

void Sniffer::reset_start_timestamp() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    _start_timestamp = (spec.tv_nsec / 1000000) + (spec.tv_sec * 1000);
}

void Sniffer::render_buffers() {
    ImGui::Begin("Buffers");

    std::vector<std::string> srcs;
    for (auto pair : _buffs) {
        srcs.push_back(std::to_string(pair.first));
    }

    if(srcs.empty()){
        ImGui::End();
        return;
    }

    if (ImGui::BeginCombo("Buffer Source", srcs[_sel_buf_src].c_str()))
    {
        for (int n = 0; n < srcs.size(); n++)
        {
            if (ImGui::Selectable(srcs[n].c_str(), _sel_buf_src == n))
                _sel_buf_src = n;

            //if (is_selected)
            //    ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    ImGui::Text("Buffer");
    Buffer* buf = _buffs[std::stoi(srcs[_sel_buf_src])];
    uint32_t size = buf->size();
    uint32_t addr = 0;
    char str[128];
    uint8_t* data = (uint8_t*)malloc(size);
    buf->getData(0, data, size);

    for (int i = 0; i < size; i += 16) {
        format_hex(str, addr, data + i, std::min(16U, size - i));
        ImGui::Text("%s", str);
        addr += 0x10;
    }

    free(data);

    ImGui::End();
}

void Sniffer::reset() {
    _sel_msg = 0;
    _sel_buf_src = 0;

    for(auto pair : _buffs){
        delete pair.second;
    }
    _buffs.clear();

    for (auto* packet : _packets) {
        delete packet->msg->bdy.data;
        delete packet->msg;
        delete packet;
    }
    _packets.clear();

    reset_start_timestamp();
}

void Sniffer::render_profiles() {
    if(_view_profiles)
        ImGui::OpenPopup("Profiles");
    if (ImGui::BeginPopupModal("Profiles", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        std::vector<std::string> names;
        for (auto pair : _profiles)
            names.push_back(pair.first);
        if(names.empty())
            names.push_back(" ");
        _sel_profile_str = names[_sel_profile];
        if (ImGui::BeginCombo("Profiles    ", names[_sel_profile].c_str()))
        {
            for (int n = 0; n < names.size(); n++)
            {
                if (ImGui::Selectable(names[n].c_str(), _sel_profile == n)){
                    _sel_profile = n;
                    if(_sel_profile_str != names[n]){
                        _sel_profile_i = 0;
                        _sel_profile_type = 0;
                    }
                    _sel_profile_str = names[n];
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine(310);
        if(ImGui::Button(ICON_FA_PLUS "##Profile")){
            memset(_new_profile, 0, 24);
            _view_profiles_add = true;
            _view_profiles = false;
        }
        ImGui::SameLine(335);
        if(ImGui::Button(ICON_FA_MINUS "##Profile")){
            _view_profiles_rm = names[_sel_profile] != " ";
            _view_profiles = false;
        }

        ImGui::Spacing();

        ImGui::Separator();
        std::vector<std::string> ids;
        if(_profiles.count(_sel_profile_str) != 0)
        {
            for (auto pair : _profiles[_sel_profile_str]->types)
                ids.push_back(std::to_string(pair.first));
        }
        if(ids.empty())
        {
            if(ImGui::BeginCombo("ID", "")){
                ImGui::EndCombo();
            }
        }
        else {
            _sel_profile_id = stoi(ids[_sel_profile_i]);
            if (ImGui::BeginCombo("ID", ids[_sel_profile_i].c_str()))
            {
                for (int n = 0; n < ids.size(); n++)
                {
                    if (ImGui::Selectable(ids[n].c_str())){
                        _sel_profile_i = n;
                        _sel_profile_id = stoi(ids[n]);
                    }
                }
                ImGui::EndCombo();
            }
        }

        if(_profiles.count(_sel_profile_str) != 0){
            ImGui::SameLine(310);
            if(ImGui::Button(ICON_FA_PLUS "##ID")){
                memset(_new_id, 0, 24);
                _new_id[0] = '0';
                _view_profiles_id_add = true;
                _view_profiles = false;
            }
            ImGui::SameLine(335);
            if(ImGui::Button(ICON_FA_MINUS "##ID")){
                _view_profiles_id_rm = !ids.empty();
                _view_profiles = false;
            }
        }



        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(0, 260), true);
        if (ImGui::BeginTable("Types", 1, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
        {
            if(!ids.empty()) {
                std::vector<ProfileType*> types = *_profiles[_sel_profile_str]->types[_sel_profile_id];
                for (int n = 0; n < types.size(); n++) {
                    ImGui::TableNextColumn();

                    std::stringstream ss;
                    ss << types[n]->getName() << "  ";
                    for(int i = 0; i < 30 - types[n]->getName().size(); i++)
                        ss << " ";
                    ss << ProfileType::typeStr[types[n]->getType()];
                    ss << "(" << types[n]->getLen() << ")";
                    if (ImGui::Selectable(ss.str().c_str(), n == _sel_profile_type)) {
                        _sel_profile_type = n;
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        if(!ids.empty()){
            ImGui::SetCursorPosX(310);
            if(ImGui::Button(ICON_FA_PLUS "##Type")){
                memset(_new_type, 0 ,24);
                memset(_new_type_len, 0 ,24);
                _new_type_len[0] = '1';
                _sel_profile_new_type = 0;
                _view_profiles_type_add = true;
                _view_profiles = false;
            }
            ImGui::SameLine(335);
            if(ImGui::Button(ICON_FA_MINUS "##Type")){
                std::vector<ProfileType*> types = *_profiles[_sel_profile_str]->types[_sel_profile_id];
                _view_profiles_type_rm = types.size() > _sel_profile_type;
                _view_profiles = false;
            }
        }


        ImGui::Separator();
        float width = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX(width / 2 - 100);
        if (ImGui::Button("Close", ImVec2(200, 0))) {
            _view_profiles = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    render_profile_add();
    render_profile_rm();
    render_profile_id_add();
    render_profile_id_rm();
    render_profile_type_add();
    render_profile_type_rm();
}

void Sniffer::render_profile_add() {
    if(_view_profiles_add)
        ImGui::OpenPopup("Add Profile");
    if (ImGui::BeginPopupModal("Add Profile", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("##EnterText", _new_profile, 24);

        std::string str(_new_profile);
        bool valid = true;
        if(str.empty()){
            valid = false;
            ImGui::Text("Name can't be empty");
        }
        else if(_profiles.count(str) != 0){
            valid = false;
            ImGui::Text("Profile already exists");
        }

        ImGui::Separator();
        if(ImGui::Button("Confirm", ImVec2(100, 0))){
            if(valid){
                _profiles[_new_profile] = new Profile;
                _view_profiles_add = false;
                _view_profiles = true;
                save_profiles();
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(100, 0))){
            _view_profiles_add = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_profile_rm() {
    if(_view_profiles_rm)
        ImGui::OpenPopup("Remove Profile");
    if (ImGui::BeginPopupModal("Remove Profile", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string text = "Are you sure you want to remove ";
        text += _sel_profile_str;
        ImGui::Text("%s", text.c_str());
        float width = ImGui::CalcTextSize(text.c_str()).x;
        if(ImGui::Button("Confirm", ImVec2(width / 2, 0))){
            if(_profiles[_sel_profile_str])
                delete _profiles[_sel_profile_str];
            _profiles.erase(_sel_profile_str);
            _sel_profile = 0;
            _view_profiles_rm = false;
            _view_profiles = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(width / 2, 0))){
            _view_profiles_rm = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_profile_id_add() {
    if(_view_profiles_id_add)
        ImGui::OpenPopup("Add ID");
    if (ImGui::BeginPopupModal("Add ID", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("##EnterText", _new_id, 24, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsDecimal);

        std::string str(_new_id);
        uint32_t id = 0;
        if(!str.empty())
            id = std::stoi(_new_id);
        bool valid = true;
        if(str.empty()){
            valid = false;
            ImGui::Text("ID can't be empty");
        }
        else if(_profiles[_sel_profile_str]->types.count(id) != 0){
            valid = false;
            ImGui::Text("ID already exists");
        }

        ImGui::Separator();
        if(ImGui::Button("Confirm", ImVec2(100, 0))){
            if(valid){
                _profiles[_sel_profile_str]->types[id] = new std::vector<ProfileType*>();
                _view_profiles_id_add = false;
                _view_profiles = true;
                save_profiles();
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(100, 0))){
            _view_profiles_id_add = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_profile_id_rm() {
    if(_view_profiles_id_rm)
        ImGui::OpenPopup("Remove ID");
    if (ImGui::BeginPopupModal("Remove ID", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string text = "Are you sure you want to remove ";
        text += std::to_string(_sel_profile_id);
        ImGui::Text("%s", text.c_str());
        float width = ImGui::CalcTextSize(text.c_str()).x;
        if(ImGui::Button("Confirm", ImVec2(width / 2, 0))){
            if(_profiles[_sel_profile_str]->types[_sel_profile_id])
                delete _profiles[_sel_profile_str]->types[_sel_profile_id];
            _profiles[_sel_profile_str]->types.erase(_sel_profile_id);
            _view_profiles_id_rm = false;
            _view_profiles = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(width / 2, 0))){
            _view_profiles_id_rm = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_profile_type_add() {
    if(_view_profiles_type_add)
        ImGui::OpenPopup("Add Type");
    if (ImGui::BeginPopupModal("Add Type", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name  ##EnterText", _new_type, 24);

        std::string str(_new_type);
        bool valid = true;
        if(str.empty()){
            valid = false;
            ImGui::Text("ID can't be empty");
        }

        if (ImGui::BeginCombo("Type", ProfileType::typeStr[_sel_profile_new_type]))
        {
            for (int n = 0; n < ProfileType::typeStr.size(); n++)
            {
                if (ImGui::Selectable(ProfileType::typeStr[n])){
                    _sel_profile_new_type = n;
                }
            }
            ImGui::EndCombo();
        }

        uint32_t len = 1;
        if(_sel_profile_new_type > 10){
            ImGui::InputText("Length##EnterText", _new_type_len, 24, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsDecimal);
            std::string lenStr(_new_type_len);

            if(lenStr.empty()){
                valid = false;
                ImGui::Text("Length can't be empty");
            }else{
                len = std::stoi(_new_type_len);

                if(len < 1){
                    valid = false;
                    ImGui::Text("Length must be > 0");
                }
            }
        }

        ImGui::Separator();
        if(ImGui::Button("Confirm", ImVec2(100, 0))){
            if(valid){
                _profiles[_sel_profile_str]->types[_sel_profile_id]->push_back(new ProfileType(_sel_profile_new_type, str, len));
                _view_profiles_type_add = false;
                _view_profiles = true;
                save_profiles();
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(100, 0))){
            _view_profiles_type_add = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_profile_type_rm() {
    if(_view_profiles_type_rm)
        ImGui::OpenPopup("Remove Type");
    if (ImGui::BeginPopupModal("Remove Type", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::vector<ProfileType*>* types = _profiles[_sel_profile_str]->types[_sel_profile_id];

        std::string text = "Are you sure you want to remove ";
        text += (*types)[_sel_profile_type]->getName();
        ImGui::Text("%s", text.c_str());
        float width = ImGui::CalcTextSize(text.c_str()).x;
        if(ImGui::Button("Confirm", ImVec2(width / 2, 0))){

            delete (*types)[_sel_profile_type];
           types->erase(types->begin() + _sel_profile_type);

            _view_profiles_type_rm = false;
            _view_profiles = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(width / 2, 0))){
            _view_profiles_type_rm = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}

void Sniffer::save_profiles() {
    for (const auto& pair : _profiles) {
        Profile::save(pair.first, pair.second);
    }
}

void Sniffer::load_profiles() {
    DIR *dir;
    struct dirent *entry;

    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }
    std::stringstream ss;
    ss << home_dir << "/STCSniffer/Profiles/";
    dir = opendir(ss.str().c_str());

    if (dir == NULL) {
        printf("Error could not open %s: %s\n", ss.str().c_str(), strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".stc_profile") != NULL) {

            std::stringstream path;
            path << ss.str() << entry->d_name;

            Profile* profile = Profile::load(path.str());
            if(profile){
                char* name = strtok(entry->d_name, ".");
                _profiles[std::string(name)] = profile;
            }
        }
    }


}

void Sniffer::render_dev() {
    float textHeight = ImGui::GetTextLineHeight() * 5;

    ImGui::SetNextWindowSize(ImVec2(_renderer->getWidth(), textHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::Begin("Device List Header", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    float textWidth = ImGui::CalcTextSize("Select the device to listen to:").x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetTextLineHeight() * 2);
    ImGui::Text("Select the device to listen to:");

    ImGui::SetNextWindowSize(ImVec2(_renderer->getWidth(), _renderer->getHeight() - textHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, textHeight), ImGuiCond_Always);
    ImGui::Begin("Device List", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    glob_t glob_result;
    glob("/dev/ttyUSB*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_sel(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    glob("/dev/serial*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_sel(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    glob("/dev/UART*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_sel(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    ImGui::End();
    ImGui::End();

    if(_dev_err_popup){
        ImGui::OpenPopup("Error Opening Device");
    }
    if (ImGui::BeginPopupModal("Error Opening Device", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        std::ostringstream oss;
        oss << "Error opening " << _sif.getDev() << ": " << strerror(_dev_errno);

        ImGui::Text("%s", oss.str().c_str());

        float textWidth = ImGui::CalcTextSize(oss.str().c_str()).x;
        ImGui::SetCursorPosX(textWidth * 0.25f);

        if (ImGui::Button("OK", ImVec2(textWidth * .5f, 0))) {
            _dev_err_popup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Sniffer::render_dev_sel(const char* dev) {
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX(windowWidth * 0.25f);
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::Selectable(dev, false, ImGuiSelectableFlags_None, ImVec2(windowWidth * .5f, ImGui::GetTextLineHeight() * 2.f));
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
        if(_sif.sIFopen(dev) == -1){
            _dev_errno = errno;
            printf("Error opening %s: %s\n", dev, strerror(errno));
            _dev_err_popup = true;
        }
        printf("%s\n", dev);

        reset_start_timestamp();
    }
}


