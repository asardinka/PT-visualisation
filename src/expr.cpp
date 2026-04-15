#include "expr.hpp"

#include <cmath>
#include <exprtk.hpp>

struct ExprEngine::Impl {
    double x = 0.0;
    double y = 0.0;
    double t = 0.0;
    bool is_valid = false;

    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expr_f;
    exprtk::expression<double> expr_g;
    exprtk::parser<double> parser;
};

ExprEngine::ExprEngine() = default;
ExprEngine::~ExprEngine() = default;
ExprEngine::ExprEngine(ExprEngine&&) noexcept = default;
ExprEngine& ExprEngine::operator=(ExprEngine&&) noexcept = default;

bool ExprEngine::compile(const std::string& sf, const std::string& sg) {
    if (!impl) {
        impl = std::make_unique<Impl>();
    }

    impl->symbol_table.clear();
    impl->symbol_table.add_variable("x", impl->x);
    impl->symbol_table.add_variable("y", impl->y);
    impl->symbol_table.add_variable("t", impl->t);
    impl->symbol_table.add_constants();
    impl->symbol_table.add_constant("e", std::exp(1.0));

    impl->expr_f.register_symbol_table(impl->symbol_table);
    impl->expr_g.register_symbol_table(impl->symbol_table);

    impl->is_valid =
        impl->parser.compile(sf, impl->expr_f) &&
        impl->parser.compile(sg, impl->expr_g);
    return impl->is_valid;
}

bool ExprEngine::valid() const {
    return impl && impl->is_valid;
}

std::pair<double, double> ExprEngine::eval(double x, double y, double t) {
    impl->x = x;
    impl->y = y;
    impl->t = t;
    return {impl->expr_f.value(), impl->expr_g.value()};
}
