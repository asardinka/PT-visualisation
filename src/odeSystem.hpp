#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct EquilibriumPoint {
    double x, y;
    std::string type;
    bool stable;
};

class OdeSystem {
public:
    std::vector<EquilibriumPoint> equilibria;
    std::vector<std::pair<double, double>> trajectory;

    OdeSystem();
    ~OdeSystem();
    OdeSystem(OdeSystem&&) noexcept;
    OdeSystem& operator=(OdeSystem&&) noexcept;
    OdeSystem(const OdeSystem&) = delete;
    OdeSystem& operator=(const OdeSystem&) = delete;

    bool compile(const std::string& sf, const std::string& sg);
    bool valid() const;
    std::pair<double, double> eval(double x, double y, double t = 0);
    std::pair<double, double> step(double x, double y, double t, double dt);
    /// maxWorldStep > 0: ограничивает шаг по полю (макс. смещение за шаг в фазовых координатах),
    /// чтобы на экране сегменты не превышали ~maxWorldStep * zoom пикселей.
    void integrate(double x0, double y0, double dt = 0.01, int steps = 5000,
                   double maxWorldStep = 0.0);
    void solve(double xmin = -5, double xmax = 5, double ymin = -5, double ymax = 5,
               double gridStep = 1.0);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};