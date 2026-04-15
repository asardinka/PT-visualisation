#include <vector>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "src/odeSystem.hpp"
#include "src/canvas.hpp"
#include "src/ui.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1200, 800 }), "Phase Portrait");
    window.setFramerateLimit(60);
    (void)ImGui::SFML::Init(window);
    ImGui::GetIO().IniFilename = nullptr;

    OdeSystem sys;
    Canvas    canvas;
    UIState   ui;

    canvas.init(window.getSize());

    sys.compile(ui.bufF, ui.bufG);
    sys.solve();
    sys.integrate(ui.x0, ui.y0, 0.01, 5000,
        static_cast<double>(3.5f) / static_cast<double>(canvas.zoom));
    std::vector<std::pair<double, double>> trajectoryDisplay = sys.trajectory;

    double tx = ui.x0, ty = ui.y0, t = 0.0;
    float  lastTrajZoom = canvas.zoom;
    bool   wasPlaying   = false;

    sf::Clock clock;
    while (window.isOpen()) {

        while (auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            canvas.handleEvent(*event);

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* r = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect area({ 0, 0 },
                    { static_cast<float>(r->size.x),
                      static_cast<float>(r->size.y) });
                window.setView(sf::View(area));
                canvas.resize(r->size);
            }
        }

        if (ui.buildPressed) {
            if (sys.compile(ui.bufF, ui.bufG)) {
                sys.solve();
                ui.playing = false;
                tx = ui.x0; ty = ui.y0; t = 0.0;
                sys.integrate(tx, ty, 0.01, 5000,
                    static_cast<double>(3.5f) / static_cast<double>(canvas.zoom));
                trajectoryDisplay = sys.trajectory;
                lastTrajZoom = canvas.zoom;
                ui.errorMsg.clear();
            } else {
                ui.errorMsg = "Parse error!";
            }
            ui.buildPressed = false;
        }

        if (!ui.playing && sys.valid()) {
            const float z = canvas.zoom;
            if (z > lastTrajZoom * 1.08f || z < lastTrajZoom / 1.08f) {
                sys.integrate(ui.x0, ui.y0, 0.01, 5000,
                    static_cast<double>(3.5f) / static_cast<double>(z));
                trajectoryDisplay = sys.trajectory;
                tx = ui.x0;
                ty = ui.y0;
                t  = 0.0;
                lastTrajZoom = z;
            }
        }

        if (ui.playing && sys.valid()) {
            if (!wasPlaying && t == 0.0) {
                sys.trajectory.clear();
                sys.trajectory.push_back({ ui.x0, ui.y0 });
                tx = ui.x0;
                ty = ui.y0;
            }
            const double playDt = 0.01 * static_cast<double>(ui.speed);
            auto [nx, ny] = sys.step(tx, ty, t, playDt);
            tx = nx; ty = ny;
            t += playDt;
            sys.trajectory.push_back({ tx, ty });
        }
        wasPlaying = ui.playing;

        window.clear(sf::Color(20, 20, 20));
        canvas.drawGrid(window);
        canvas.drawVectorField(window, sys);
        if (ui.showTrajectory)
            canvas.drawTrajectory(window, trajectoryDisplay);
        canvas.drawEquilibria(window, sys);
        if (sys.valid())
            canvas.drawPhaseMarker(window, tx, ty);

        ImGui::SFML::Update(window, clock.restart());
        canvas.drawGridLabels();
        drawUI(ui, sys);
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
