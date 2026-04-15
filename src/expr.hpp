#pragma once

#include <memory>
#include <string>
#include <utility>

class ExprEngine {
public:
    ExprEngine();
    ~ExprEngine();
    ExprEngine(ExprEngine&&) noexcept;
    ExprEngine& operator=(ExprEngine&&) noexcept;
    ExprEngine(const ExprEngine&) = delete;
    ExprEngine& operator=(const ExprEngine&) = delete;

    bool compile(const std::string& sf, const std::string& sg);
    bool valid() const;
    std::pair<double, double> eval(double x, double y, double t = 0.0);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
