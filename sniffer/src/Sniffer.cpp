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

    _devSelector = new DevSelector(&_sif, &_start_timestamp);

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
    delete _devSelector;
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
        _devSelector->render(renderer);
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

    bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);
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
    ImGui::SameLine(300);
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
        ImGui::SameLine(300);

        char buf[256];
        sprintf(buf, "0x%08X  0x%04X 0x%04X 0x%08X 0x%02X", msg->magic, msg->hdr.id, msg->hdr.len, msg->hdr.tcn, msg->hdr.tcf);
        ImGui::Selectable((std::string(buf) + "##Row" + std::to_string(i)).c_str());

        if (isSelected) {
            _sel_msg = i;
        }
    }

    ImGui::End();
}

char to_aasci(uint8_t val){
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
            to_aasci(tmp[0]), to_aasci(tmp[1]), to_aasci(tmp[2]), to_aasci(tmp[3]), to_aasci(tmp[4]), to_aasci(tmp[5]), to_aasci(tmp[6]), to_aasci(tmp[7]),
            to_aasci(tmp[8]), to_aasci(tmp[9]), to_aasci(tmp[10]), to_aasci(tmp[11]), to_aasci(tmp[12]), to_aasci(tmp[13]), to_aasci(tmp[14]), to_aasci(tmp[15]));

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
        format_hex(str, addr, (uint8_t*)&msg->bdy + i, std::min(16, msg->hdr.len - i));
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
            ImGui::Text("\t0x%01X (%d)", msg->hdr.tcf, msg->hdr.tcf);
            // TODO: Add meaning
            ImGui::TreePop();
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
        if (ImGui::BeginCombo("Profile List", names[_sel_profile].c_str()))
        {
            for (int n = 0; n < names.size(); n++)
            {
                if (ImGui::Selectable(names[n].c_str(), _sel_profile == n)){
                    _sel_profile = n;
                    _sel_profile_str = names[n];
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine(310);
        if(ImGui::Button(ICON_FA_PLUS)){
            _view_profiles_add = true;
            _view_profiles = false;
        }
        ImGui::SameLine(335);
        if(ImGui::Button(ICON_FA_MINUS)){
            _view_profiles_rm = names[_sel_profile] != " ";
            _view_profiles = false;
        }

        if (ImGui::Button("Close")) {
            _view_profiles = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    if(_view_profiles_add)
        ImGui::OpenPopup("Profiles Add");
    if (ImGui::BeginPopupModal("Profiles Add", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        char name[256] = {"test"};
        ImGui::InputText("##Enter Text", name, 256);
        if(ImGui::Button("Confirm")){
            _profiles[name] = new Profile;
            _view_profiles_add = false;
            _view_profiles = true;
        }
        if(ImGui::Button("Cancel")){
            _view_profiles_add = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }

    if(_view_profiles_rm)
        ImGui::OpenPopup("Profiles Remove");
    if (ImGui::BeginPopupModal("Profiles Remove", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to remove %s", _sel_profile_str.c_str());
        if(ImGui::Button("Confirm")){
            if(_profiles[_sel_profile_str])
                delete _profiles[_sel_profile_str];
            _profiles.erase(_sel_profile_str);
            _sel_profile_str = " ";
            _sel_profile = 0;
            _view_profiles_rm = false;
            _view_profiles = true;
        }
        if(ImGui::Button("Cancel")){
            _view_profiles_rm = false;
            _view_profiles = true;
        }
        ImGui::EndPopup();
    }
}


