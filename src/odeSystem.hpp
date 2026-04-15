#pragma once

#include "expr.hpp"

#include <Eigen/Eigenvalues>
#include <algorithm>
#include <cmath>
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

    OdeSystem() : impl(std::make_unique<Impl>()) {}
    ~OdeSystem() = default;
    OdeSystem(OdeSystem&&) noexcept = default;
    OdeSystem& operator=(OdeSystem&&) noexcept = default;
    OdeSystem(const OdeSystem&) = delete;
    OdeSystem& operator=(const OdeSystem&) = delete;

    bool compile(const std::string& sf, const std::string& sg) { return impl->compile(sf, sg); }
    bool valid() const { return impl->valid(); }
    std::pair<double, double> eval(double x, double y, double t = 0) { return impl->eval(x, y, t); }
    std::pair<double, double> step(double x, double y, double t, double dt) { return impl->step(x, y, t, dt); }

    void integrate(double x0, double y0, double dt = 0.01, int steps = 5000, double maxWorldStep = 0.0) {
        trajectory.clear();
        if (maxWorldStep > 0) {
            steps = std::max(steps, 20000);
        }
        double x = x0;
        double y = y0;
        double t = 0;
        for (int i = 0; i < steps; i++) {
            trajectory.push_back({x, y});
            double stepDt = dt;
            if (maxWorldStep > 0) {
                auto [fx, fy] = impl->eval(x, y, t);
                double speed = std::hypot(fx, fy);
                if (speed > 1e-12) {
                    stepDt = std::min(stepDt, maxWorldStep / speed);
                }
                stepDt = std::max(stepDt, 1e-10);
            }
            auto [nx, ny] = impl->step(x, y, t, stepDt);
            x = nx;
            y = ny;
            t += stepDt;
            if (std::abs(x) > 100 || std::abs(y) > 100) {
                break;
            }
        }
    }
    void solve(double xmin = -5, double xmax = 5, double ymin = -5, double ymax = 5, double gridStep = 1.0) {
        equilibria.clear();
        const double eps = 1e-6;

        for (double x0 = xmin; x0 <= xmax; x0 += gridStep) {
            for (double y0 = ymin; y0 <= ymax; y0 += gridStep) {
                double x = x0;
                double y = y0;

                for (int i = 0; i < 50; i++) {
                    auto [fx, fy] = impl->eval(x, y, 0);
                    if (std::abs(fx) < eps && std::abs(fy) < eps) {
                        break;
                    }

                    auto J = impl->jacobian(x, y);
                    double det = J(0, 0) * J(1, 1) - J(0, 1) * J(1, 0);
                    if (std::abs(det) < 1e-12) {
                        break;
                    }

                    double dx = -(J(1, 1) * fx - J(0, 1) * fy) / det;
                    double dy = -(-J(1, 0) * fx + J(0, 0) * fy) / det;
                    x += dx;
                    y += dy;
                    if (std::abs(dx) < eps && std::abs(dy) < eps) {
                        break;
                    }
                }

                auto [fx, fy] = impl->eval(x, y, 0);
                if (std::abs(fx) > 1e-5 || std::abs(fy) > 1e-5) {
                    continue;
                }
                if (std::abs(x) > xmax * 2 || std::abs(y) > ymax * 2) {
                    continue;
                }

                bool dup = false;
                for (auto& ep : equilibria) {
                    if (std::abs(ep.x - x) < 1e-3 && std::abs(ep.y - y) < 1e-3) {
                        dup = true;
                        break;
                    }
                }

                if (!dup) {
                    equilibria.push_back(impl->classify(x, y));
                }
            }
        }
    }

private:
    struct Impl {
        ExprEngine engine;

        bool compile(const std::string& sf, const std::string& sg) {
            return engine.compile(sf, sg);
        }

        bool valid() const { return engine.valid(); }

        std::pair<double, double> eval(double x, double y, double t = 0) {
            return engine.eval(x, y, t);
        }

        std::pair<double, double> step(double x, double y, double t, double dt) {
            auto [k1x, k1y] = eval(x, y, t);
            auto [k2x, k2y] = eval(x + dt / 2 * k1x, y + dt / 2 * k1y, t + dt / 2);
            auto [k3x, k3y] = eval(x + dt / 2 * k2x, y + dt / 2 * k2y, t + dt / 2);
            auto [k4x, k4y] = eval(x + dt * k3x, y + dt * k3y, t + dt);
            return {
                x + dt / 6 * (k1x + 2 * k2x + 2 * k3x + k4x),
                y + dt / 6 * (k1y + 2 * k2y + 2 * k3y + k4y)};
        }

        Eigen::Matrix2d jacobian(double x, double y) {
            const double h = 1e-6;
            Eigen::Matrix2d J;
            J(0, 0) = (eval(x + h, y).first - eval(x - h, y).first) / (2 * h);
            J(0, 1) = (eval(x, y + h).first - eval(x, y - h).first) / (2 * h);
            J(1, 0) = (eval(x + h, y).second - eval(x - h, y).second) / (2 * h);
            J(1, 1) = (eval(x, y + h).second - eval(x, y - h).second) / (2 * h);
            return J;
        }

        EquilibriumPoint classify(double x, double y) {
            auto ev = Eigen::EigenSolver<Eigen::Matrix2d>(jacobian(x, y)).eigenvalues();
            double re1 = ev[0].real();
            double re2 = ev[1].real();
            double im1 = ev[0].imag();

            EquilibriumPoint ep;
            ep.x = x;
            ep.y = y;

            if (std::abs(im1) > 1e-6) {
                if (std::abs(re1) < 1e-6) {
                    ep.type = "Center";
                    ep.stable = true;
                } else if (re1 < 0) {
                    ep.type = "Stable focus";
                    ep.stable = true;
                } else {
                    ep.type = "Unstable focus";
                    ep.stable = false;
                }
            } else if (re1 * re2 < 0) {
                ep.type = "Saddle";
                ep.stable = false;
            } else if (re1 < 0) {
                ep.type = "Stable node";
                ep.stable = true;
            } else {
                ep.type = "Unstable node";
                ep.stable = false;
            }
            return ep;
        }
    };
    std::unique_ptr<Impl> impl;
};