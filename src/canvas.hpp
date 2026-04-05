#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include "odeSystem.hpp"

struct Canvas {
    static constexpr float kMinPxPerWorld = 1e-4f;
    static constexpr float kMaxPxPerWorld = 1e6f;

    float zoom = 50.0f;
    sf::Vector2f offset;
    sf::Vector2u windowSize;

private:
    bool         dragging  = false;
    sf::Vector2i lastMouse;

    struct GridGeom {
        float  W, H;
        double gpx, startX, startY, wx0, wy0, worldStep;
    };

    GridGeom gridGeom() const {
        float W = static_cast<float>(windowSize.x);
        float H = static_cast<float>(windowSize.y);
        float gridPx = zoom;
        while (gridPx < 40.f) gridPx *= 2.f;
        while (gridPx > 80.f) gridPx /= 2.f;

        const double gpx = static_cast<double>(gridPx);
        const double ox  = static_cast<double>(offset.x);
        const double oy  = static_cast<double>(offset.y);
        const double z   = static_cast<double>(zoom);

        double startX = std::fmod(ox, gpx);
        if (startX < 0) startX += gpx;
        double startY = std::fmod(oy, gpx);
        if (startY < 0) startY += gpx;

        const double worldPerPx = 1.0 / z;
        const double wx0        = (startX - ox) * worldPerPx;
        const double wy0        = (oy - startY) * worldPerPx;
        const double worldStep  = gpx * worldPerPx;

        return { W, H, gpx, startX, startY, wx0, wy0, worldStep };
    }

public:
    void init(sf::Vector2u size) {
        windowSize = size;
        offset = { size.x / 2.0f, size.y / 2.0f };
        zoom     = std::clamp(zoom, kMinPxPerWorld, kMaxPxPerWorld);
    }

    void resize(sf::Vector2u size) {
        offset.x += (static_cast<float>(size.x) - static_cast<float>(windowSize.x)) / 2.0f;
        offset.y += (static_cast<float>(size.y) - static_cast<float>(windowSize.y)) / 2.0f;
        windowSize = size;
    }

    sf::Vector2f toScreen(double wx, double wy) const {
        return {
            offset.x + static_cast<float>(wx) * zoom,
            offset.y - static_cast<float>(wy) * zoom
        };
    }

    sf::Vector2f toWorld(float sx, float sy) const {
        return {
            (sx - offset.x) / zoom,
            (offset.y - sy) / zoom
        };
    }

    void handleEvent(const sf::Event& event) {
        if (ImGui::GetIO().WantCaptureMouse) return;

        if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            constexpr float kStepPerDelta = 1.1f;
            float           d             = std::clamp(scroll->delta, -30.f, 30.f);
            float           mult          = std::pow(kStepPerDelta, d);
            sf::Vector2f    mouse(static_cast<float>(scroll->position.x),
                                  static_cast<float>(scroll->position.y));
            float oldZ = zoom;
            float newZ = std::clamp(oldZ * mult, kMinPxPerWorld, kMaxPxPerWorld);
            if (newZ == oldZ)
                return;
            float scale = newZ / oldZ;
            offset      = mouse + (offset - mouse) * scale;
            zoom        = newZ;
        }
        if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (press->button == sf::Mouse::Button::Left) {
                dragging  = true;
                lastMouse = { press->position.x, press->position.y };
            }
        }
        if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (release->button == sf::Mouse::Button::Left)
                dragging = false;
        }
        if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
            if (dragging) {
                sf::Vector2i cur(move->position.x, move->position.y);
                offset   += sf::Vector2f(static_cast<float>(cur.x - lastMouse.x),
                                         static_cast<float>(cur.y - lastMouse.y));
                lastMouse = cur;
            }
        }
    }

    // ── Grid ────────────────────────────────────────────────────────────────
    void drawGrid(sf::RenderWindow& window) {
        const GridGeom g = gridGeom();
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        sf::Color       gridCol(55, 55, 55);

        const double Wd = static_cast<double>(g.W);
        const double Hd = static_cast<double>(g.H);

        for (int i = 0;; ++i) {
            double xd = g.startX + static_cast<double>(i) * g.gpx;
            if (xd >= Wd) break;
            float x = static_cast<float>(xd);
            lines.append({ {x, 0}, gridCol });
            lines.append({ {x, g.H}, gridCol });
        }

        for (int j = 0;; ++j) {
            double yd = g.startY + static_cast<double>(j) * g.gpx;
            if (yd >= Hd) break;
            float y = static_cast<float>(yd);
            lines.append({ {0, y}, gridCol });
            lines.append({ {g.W, y}, gridCol });
        }

        sf::Color axisCol(180, 180, 180);
        lines.append({ {0,        offset.y}, axisCol });
        lines.append({ {g.W,      offset.y}, axisCol });
        lines.append({ {offset.x, 0},        axisCol });
        lines.append({ {offset.x, g.H},    axisCol });

        window.draw(lines);
    }

    /// Вызывать после ImGui::SFML::Update (нужен активный кадр ImGui и шрифт по умолчанию).
    void drawGridLabels() {
        const GridGeom g = gridGeom();
        ImDrawList*      dl = ImGui::GetBackgroundDrawList();
        const float      labelMargin = 6.f;
        const ImU32      col = IM_COL32(160, 160, 160, 255);
        char             buf[32];

        const double Wd = static_cast<double>(g.W);
        const double Hd = static_cast<double>(g.H);

        for (int i = 0;; ++i) {
            double xd = g.startX + static_cast<double>(i) * g.gpx;
            if (xd >= Wd) break;
            double wx = g.wx0 + static_cast<double>(i) * g.worldStep;
            formatWorldValueAxisX(buf, sizeof buf, wx, g.worldStep);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float  x  = static_cast<float>(xd);
            dl->AddText({ x - ts.x * 0.5f, g.H - labelMargin - ts.y }, col, buf);
        }

        for (int j = 0;; ++j) {
            double yd = g.startY + static_cast<double>(j) * g.gpx;
            if (yd >= Hd) break;
            double wy = g.wy0 - static_cast<double>(j) * g.worldStep;
            formatWorldValueAxisY(buf, sizeof buf, wy, g.worldStep);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float  y  = static_cast<float>(yd);
            dl->AddText({ g.W - labelMargin - ts.x, y - ts.y * 0.5f }, col, buf);
        }
    }

    // ── Vector field ────────────────────────────────────────────────────────
    void drawVectorField(sf::RenderWindow& window, OdeSystem& sys) {
        if (!sys.valid()) return;

        float W = static_cast<float>(windowSize.x);
        float H = static_cast<float>(windowSize.y);
        auto  wMin = toWorld(0, H);
        auto  wMax = toWorld(W, 0);

        const float worldW = wMax.x - wMin.x;
        const float worldH = wMax.y - wMin.y;
        if (worldW <= 0 || worldH <= 0) return;

        // Шаг сетки в мире так, чтобы на экране было ~sampleSpacingPx между точками;
        // убран нижний порог 0.3 — он давал 2–3 гигантские стрелки при сильном zoom.
        constexpr float sampleSpacingPx = 11.f;
        constexpr int   maxSamplesAxis  = 80;
        constexpr float arrowLenPx      = 6.5f;

        float step = sampleSpacingPx / zoom;
        const float stepFloor =
            std::max(worldW / static_cast<float>(maxSamplesAxis),
                     worldH / static_cast<float>(maxSamplesAxis));
        step = std::max(step, stepFloor);

        const float minSpan = std::min(worldW, worldH);
        if (step * 2.f > minSpan)
            step = std::max(minSpan / 3.f, 1e-12f);

        float arrowLen = std::min(arrowLenPx / zoom, step * 0.36f);

        const double w0x = static_cast<double>(wMin.x);
        const double w0y = static_cast<double>(wMin.y);
        const double w1x = static_cast<double>(wMax.x);
        const double w1y = static_cast<double>(wMax.y);
        const double st  = static_cast<double>(step);

        const int nxi = std::max(1, static_cast<int>(std::ceil((w1x - w0x) / st)) + 1);
        const int nyi = std::max(1, static_cast<int>(std::ceil((w1y - w0y) / st)) + 1);

        sf::VertexArray lines(sf::PrimitiveType::Lines);

        for (int iy = 0; iy < nyi; ++iy) {
            double wy = w0y + static_cast<double>(iy) * st;
            if (wy > w1y + st * 1e-6) break;
            for (int ix = 0; ix < nxi; ++ix) {
                double wx = w0x + static_cast<double>(ix) * st;
                if (wx > w1x + st * 1e-6) break;

                auto [fx, fy] = sys.eval(wx, wy);
                float len = std::sqrt(static_cast<float>(fx * fx + fy * fy));
                if (len < 1e-12f) continue;

                const float al = arrowLen;
                float nxf = static_cast<float>(fx) / len * al;
                float nyf = static_cast<float>(fy) / len * al;

                sf::Vector2f start = toScreen(wx, wy);
                sf::Vector2f end   = toScreen(wx + static_cast<double>(nxf),
                                               wy + static_cast<double>(nyf));

                sf::Color col = speedColor(len);
                lines.append({ start, col });
                lines.append({ end,   col });
            }
        }
        window.draw(lines);
    }

    // ── Trajectory ──────────────────────────────────────────────────────────
    void drawTrajectory(sf::RenderWindow& window,
                        const std::vector<std::pair<double, double>>& path) {
        if (path.empty()) return;

        sf::VertexArray va(sf::PrimitiveType::LineStrip);
        for (auto& [x, y] : path)
            va.append({ toScreen(x, y), sf::Color::Cyan });
        window.draw(va);
    }

    /// Текущая точка на фазовой плоскости (начальная после Build, движущаяся при Play).
    void drawPhaseMarker(sf::RenderWindow& window, double wx, double wy) {
        sf::Vector2f pos = toScreen(wx, wy);

        sf::CircleShape dot(7.f);
        dot.setOrigin({ 7.f, 7.f });
        dot.setPosition(pos);
        dot.setFillColor(sf::Color(255, 220, 60));
        dot.setOutlineColor(sf::Color::White);
        dot.setOutlineThickness(1.5f);
        window.draw(dot);
    }

    // ── Equilibrium points ──────────────────────────────────────────────────
    void drawEquilibria(sf::RenderWindow& window, OdeSystem& sys) {
        for (auto& ep : sys.equilibria) {
            sf::Vector2f pos = toScreen(ep.x, ep.y);

            sf::CircleShape circle(6.f);
            circle.setOrigin({ 6.f, 6.f });
            circle.setPosition(pos);
            circle.setFillColor(ep.stable ? sf::Color(0, 210, 0)
                                           : sf::Color(210, 50, 50));
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(1.f);
            window.draw(circle);
        }
    }

private:
    static double snapWorldToGrid(double v, double worldStep) {
        if (worldStep > 0 && std::isfinite(v)) {
            const double q  = v / worldStep;
            const double rq = std::round(q);
            if (std::fabs(q - rq) < 1e-6)
                v = rq * worldStep;
        }
        if (std::fabs(v) < 1e-9 * std::max(worldStep, 1.0))
            v = 0;
        return v;
    }

    static void formatWorldValueAxisX(char* buf, std::size_t n, double v, double worldStep) {
        v = snapWorldToGrid(v, worldStep);
        std::snprintf(buf, n, "%.3f", v);
    }

    static void formatWorldValueAxisY(char* buf, std::size_t n, double v, double worldStep) {
        v = snapWorldToGrid(v, worldStep);
        std::snprintf(buf, n, "%.6g", v);
    }

    static sf::Color speedColor(float speed) {
        float t = std::min(speed / 3.0f, 1.0f);
        return sf::Color(
            static_cast<uint8_t>(t * 255),
            static_cast<uint8_t>((1.f - t) * 200),
            80
        );
    }
};