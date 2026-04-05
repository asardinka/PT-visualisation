#pragma once
#include <imgui.h>
#include <string>
#include "odeSystem.hpp"

struct UIState {
    char   bufF[256] = "x-x*y";
    char   bufG[256] = "x*y-y";
    double x0        = 0.75;
    double y0        = 0.75;
    float  speed     = 1.f;
    bool   playing      = false;
    bool   buildPressed = false;
    bool   showTrajectory = true;
    std::string errorMsg;
};

inline void drawUI(UIState& ui, OdeSystem& sys) {
    ImGui::SetNextWindowPos({ 10, 10 }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 270, 0 }, ImGuiCond_Once);
    ImGui::Begin("Phase Portrait");

    // ── Equations ────────────────────────────────────────────────────────
    ImGui::Text("dx/dt =");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##f", ui.bufF, sizeof(ui.bufF));

    ImGui::Text("dy/dt =");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##g", ui.bufG, sizeof(ui.bufG));

    ImGui::TextDisabled("Tip: use * for multiply (x*y)");

    // ── Initial point ─────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("Initial point:");
    ImGui::SetNextItemWidth(115);
    ImGui::InputDouble("x0", &ui.x0, 0.1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(115);
    ImGui::InputDouble("y0", &ui.y0, 0.1);

    // ── Speed ─────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("Speed:");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##speed", &ui.speed, 0.1f, 10.f);

    // ── Buttons ───────────────────────────────────────────────────────────
    ImGui::Spacing();
    if (ImGui::Button("Build", { -1, 0 }))
        ui.buildPressed = true;

    ImGui::Spacing();
    if (ui.playing) {
        if (ImGui::Button("Pause", { -1, 0 })) ui.playing = false;
    } else {
        if (ImGui::Button("Play",  { -1, 0 })) ui.playing = true;
    }

    ImGui::Spacing();
    ImGui::Checkbox("Show trajectory", &ui.showTrajectory);

    // ── Error ─────────────────────────────────────────────────────────────
    if (!ui.errorMsg.empty()) {
        ImGui::Spacing();
        ImGui::TextColored({ 1, 0.3f, 0.3f, 1 }, "%s", ui.errorMsg.c_str());
    }

    // ── Equilibria ────────────────────────────────────────────────────────
    if (!sys.equilibria.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Equilibria:");
        for (auto& ep : sys.equilibria) {
            ImVec4 col = ep.stable ? ImVec4(0.2f, 1.f, 0.2f, 1.f)
                                   : ImVec4(1.f, 0.4f, 0.4f, 1.f);
            ImGui::TextColored(col, "(%.2f, %.2f)  %s",
                               ep.x, ep.y, ep.type.c_str());
        }
    }

    ImGui::End();
}