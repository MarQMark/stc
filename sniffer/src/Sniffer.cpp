#include "Sniffer.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Kikan/renderer/stdRenderer/Camera.h"

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

    devSelector = DevSelector(&_sif);
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
        uint32_t src;
        uint32_t len;
        if((data = _sif.sIFread(&src, &len)) != nullptr){
            printf("recv data %d\n", len);

            _buffs[src].addData(data, len);
            _buffs[src].parseMsgs();
        }
        render_dockspace();
    }
    else{
        devSelector.render(renderer);
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

    render_menubar();
    if(_view_msgs)
        render_msgs();
    if(_view_hex)
        render_hex();
    if(_view_details)
        render_details();

    bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::End();
}

void Sniffer::render_menubar() {
    if(ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {

        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Messages", nullptr, &_view_msgs);
            ImGui::MenuItem("Hex", nullptr, &_view_hex);
            ImGui::MenuItem("Details", nullptr, &_view_details);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Sniffer::render_actionbar() {

}

struct RowData {
    std::string src;
    std::string time;
    std::string value;
};

void Sniffer::render_msgs() {
    ImGui::Begin("Messages");

    ImGui::Text("Src");
    ImGui::SameLine(150);
    ImGui::Text("Time");
    ImGui::SameLine(300);
    ImGui::Text("Value");

    std::vector<RowData> tableData = {
            {"Source 1", "10:00", "100"},
            {"Source 2", "10:15", "150"},
            {"Source 3", "10:30", "200"}
    };

    ImGui::Separator();
    // Rows
    for (size_t i = 0; i < tableData.size(); ++i) {
        bool isSelected = ImGui::Selectable((tableData[i].src + "##Row" + std::to_string(i)).c_str());
        ImGui::SameLine(150);
        ImGui::Selectable((tableData[i].time + "##Row" + std::to_string(i)).c_str());
        ImGui::SameLine(300);
        ImGui::Selectable((tableData[i].value + "##Row" + std::to_string(i)).c_str());

        // Handle row selection logic here
        if (isSelected) {
            // Handle the selection of the entire row
            // For example, you can store the selected row index or perform some action
            // This code simply prints the selected row index to the console
            std::cout << "Selected row index: " << i << std::endl;
        }
    }

    ImGui::End();
}

void Sniffer::render_hex() {

}

void Sniffer::render_details() {

}


