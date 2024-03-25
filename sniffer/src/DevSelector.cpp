#include <cerrno>
#include <glob.h>
#include <sstream>
#include "DevSelector.h"
#include "imgui/imgui.h"

DevSelector::DevSelector(SerialInterface* sif, uint64_t* timestamp) : _sif(sif), _timestamp(timestamp) {
}

DevSelector::~DevSelector() {
}

void DevSelector::render(Kikan::StdRenderer* renderer) {
    float textHeight = ImGui::GetTextLineHeight() * 5;

    ImGui::SetNextWindowSize(ImVec2(renderer->getWidth(), textHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::Begin("Device List Header", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    float textWidth = ImGui::CalcTextSize("Select the device to listen to:").x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetTextLineHeight() * 2);
    ImGui::Text("Select the device to listen to:");

    ImGui::SetNextWindowSize(ImVec2(renderer->getWidth(), renderer->getHeight() - textHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, textHeight), ImGuiCond_Always);
    ImGui::Begin("Device List", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    glob_t glob_result;
    glob("/dev/ttyUSB*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_selectable(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    glob("/dev/serial*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_selectable(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    glob("/dev/UART*", GLOB_TILDE, NULL, &glob_result);
    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
        render_dev_selectable(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);

    ImGui::End();
    ImGui::End();

    if(_err_popup){
        ImGui::OpenPopup("Error Opening Device");
    }
    if (ImGui::BeginPopupModal("Error Opening Device", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        std::ostringstream oss;
        oss << "Error opening " << _sif->getDev() << ": " << strerror(_errno);

        ImGui::Text("%s", oss.str().c_str());

        float textWidth = ImGui::CalcTextSize(oss.str().c_str()).x;
        ImGui::SetCursorPosX(textWidth * 0.25f);

        if (ImGui::Button("OK", ImVec2(textWidth * .5f, 0))) {
            _err_popup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DevSelector::render_dev_selectable(const char* dev){
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX(windowWidth * 0.25f);
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::Selectable(dev, false, ImGuiSelectableFlags_None, ImVec2(windowWidth * .5f, ImGui::GetTextLineHeight() * 2.f));
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
        if(_sif->sIFopen(dev) == -1){
            _errno = errno;
            printf("Error opening %s: %s\n", dev, strerror(errno));
            _err_popup = true;
        }
        printf("%s\n", dev);

        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        *_timestamp = (spec.tv_nsec / 1000000) + (spec.tv_sec * 1000);
    }
}
