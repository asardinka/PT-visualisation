#include "odeSystem.hpp"

#include <algorithm>
#include <cmath>
#include <Eigen/Eigenvalues>
#include <exprtk.hpp>

using namespace std;

struct OdeSystem::Impl {
    double _x = 0, _y = 0, _t = 0;
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double>   expr_f, expr_g;
    exprtk::parser<double>       parser;
    bool _valid = false;

    bool compile(const string& sf, const string& sg) {
        symbol_table.clear();
        symbol_table.add_variable("x", _x);
        symbol_table.add_variable("y", _y);
        symbol_table.add_variable("t", _t);
        symbol_table.add_constants();
        symbol_table.add_constant("e", exp(1.0));

        expr_f.register_symbol_table(symbol_table);
        expr_g.register_symbol_table(symbol_table);

        _valid = parser.compile(sf, expr_f) && parser.compile(sg, expr_g);
        return _valid;
    }

    bool valid() const { return _valid; }

    pair<double, double> eval(double x, double y, double t = 0) {
        _x = x; _y = y; _t = t;
        return { expr_f.value(), expr_g.value() };
    }

    pair<double, double> step(double x, double y, double t, double dt) {
        auto [k1x, k1y] = eval(x, y, t);
        auto [k2x, k2y] = eval(x + dt/2*k1x, y + dt/2*k1y, t + dt/2);
        auto [k3x, k3y] = eval(x + dt/2*k2x, y + dt/2*k2y, t + dt/2);
        auto [k4x, k4y] = eval(x + dt*k3x,   y + dt*k3y,   t + dt);
        return {
            x + dt/6 * (k1x + 2*k2x + 2*k3x + k4x),
            y + dt/6 * (k1y + 2*k2y + 2*k3y + k4y)
        };
    }

    Eigen::Matrix2d jacobian(double x, double y) {
        const double h = 1e-6;
        Eigen::Matrix2d J;
        J(0,0) = (eval(x+h,y).first  - eval(x-h,y).first)  / (2*h);
        J(0,1) = (eval(x,y+h).first  - eval(x,y-h).first)  / (2*h);
        J(1,0) = (eval(x+h,y).second - eval(x-h,y).second) / (2*h);
        J(1,1) = (eval(x,y+h).second - eval(x,y-h).second) / (2*h);
        return J;
    }

    EquilibriumPoint classify(double x, double y) {
        auto ev = Eigen::EigenSolver<Eigen::Matrix2d>(jacobian(x, y)).eigenvalues();
        double re1 = ev[0].real(), re2 = ev[1].real();
        double im1 = ev[0].imag();

        EquilibriumPoint ep;
        ep.x = x; ep.y = y;

        if (abs(im1) > 1e-6) {
            if (abs(re1) < 1e-6) {
                ep.type = "Center";       ep.stable = true;
            } else if (re1 < 0) {
                ep.type = "Stable focus"; ep.stable = true;
            } else {
                ep.type = "Unstable focus"; ep.stable = false;
            }
        } else if (re1 * re2 < 0) {
            ep.type = "Saddle";           ep.stable = false;
        } else if (re1 < 0) {
            ep.type = "Stable node";      ep.stable = true;
        } else {
            ep.type = "Unstable node";    ep.stable = false;
        }
        return ep;
    }
};

OdeSystem::OdeSystem()  : impl(make_unique<Impl>()) {}
OdeSystem::~OdeSystem() = default;
OdeSystem::OdeSystem(OdeSystem&&) noexcept = default;
OdeSystem& OdeSystem::operator=(OdeSystem&&) noexcept = default;

bool OdeSystem::compile(const string& sf, const string& sg) { return impl->compile(sf, sg); }
bool OdeSystem::valid()  const { return impl->valid(); }
pair<double,double> OdeSystem::eval(double x, double y, double t) { return impl->eval(x,y,t); }
pair<double,double> OdeSystem::step(double x, double y, double t, double dt) { return impl->step(x,y,t,dt); }

void OdeSystem::integrate(double x0, double y0, double dt, int steps, double maxWorldStep) {
    trajectory.clear();
    if (maxWorldStep > 0)
        steps = max(steps, 20000);
    double x = x0, y = y0, t = 0;
    for (int i = 0; i < steps; i++) {
        trajectory.push_back({x, y});
        double stepDt = dt;
        if (maxWorldStep > 0) {
            auto [fx, fy] = impl->eval(x, y, t);
            double speed = hypot(fx, fy);
            if (speed > 1e-12)
                stepDt = min(stepDt, maxWorldStep / speed);
            stepDt = max(stepDt, 1e-10);
        }
        auto [nx, ny] = impl->step(x, y, t, stepDt);
        x = nx;
        y = ny;
        t += stepDt;
        if (abs(x) > 100 || abs(y) > 100) break;
    }
}

void OdeSystem::solve(double xmin, double xmax, double ymin, double ymax, double gridStep) {
    equilibria.clear();
    const double eps = 1e-6;

    for (double x0 = xmin; x0 <= xmax; x0 += gridStep) {
        for (double y0 = ymin; y0 <= ymax; y0 += gridStep) {
            double x = x0, y = y0;

            for (int i = 0; i < 50; i++) {
                auto [fx, fy] = impl->eval(x, y, 0);
                if (abs(fx) < eps && abs(fy) < eps) break;

                auto J = impl->jacobian(x, y);
                double det = J(0,0)*J(1,1) - J(0,1)*J(1,0);
                if (abs(det) < 1e-12) break;

                double dx = -(J(1,1)*fx - J(0,1)*fy) / det;
                double dy = -(-J(1,0)*fx + J(0,0)*fy) / det;
                x += dx; y += dy;
                if (abs(dx) < eps && abs(dy) < eps) break;
            }

            auto [fx, fy] = impl->eval(x, y, 0);
            if (abs(fx) > 1e-5 || abs(fy) > 1e-5) continue;
            if (abs(x) > xmax*2 || abs(y) > ymax*2) continue;

            bool dup = false;
            for (auto& ep : equilibria)
                if (abs(ep.x-x) < 1e-3 && abs(ep.y-y) < 1e-3) { dup = true; break; }

            if (!dup) equilibria.push_back(impl->classify(x, y));
        }
    }
}
