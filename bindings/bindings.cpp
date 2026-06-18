#include <pybind11/pybind11.h>

#include "ope/black_scholes.hpp"

namespace py = pybind11;

PYBIND11_MODULE(ope, m) {
    m.doc() = "Options Pricing Engine - C++ core exposed to Python";

    py::enum_<ope::OptionType>(m, "OptionType")
        .value("Call", ope::OptionType::Call)
        .value("Put", ope::OptionType::Put);

    py::class_<ope::BSInputs>(m, "BSInputs")
        .def(py::init<>())
        // Keyword-constructible: BSInputs(spot=..., strike=..., ...)
        .def(py::init([](double spot, double strike, double rate, double dividend,
                         double vol, double maturity, ope::OptionType type) {
                 return ope::BSInputs{spot, strike, rate, dividend, vol, maturity, type};
             }),
             py::arg("spot"), py::arg("strike"), py::arg("rate"),
             py::arg("dividend") = 0.0, py::arg("vol") = 0.2,
             py::arg("maturity") = 1.0, py::arg("type") = ope::OptionType::Call)
        .def_readwrite("spot", &ope::BSInputs::spot)
        .def_readwrite("strike", &ope::BSInputs::strike)
        .def_readwrite("rate", &ope::BSInputs::rate)
        .def_readwrite("dividend", &ope::BSInputs::dividend)
        .def_readwrite("vol", &ope::BSInputs::vol)
        .def_readwrite("maturity", &ope::BSInputs::maturity)
        .def_readwrite("type", &ope::BSInputs::type);

    py::class_<ope::Greeks>(m, "Greeks")
        .def_readonly("price", &ope::Greeks::price)
        .def_readonly("delta", &ope::Greeks::delta)
        .def_readonly("gamma", &ope::Greeks::gamma)
        .def_readonly("vega", &ope::Greeks::vega)
        .def_readonly("theta", &ope::Greeks::theta)
        .def_readonly("rho", &ope::Greeks::rho)
        .def("__repr__", [](const ope::Greeks& g) {
            return "<Greeks price=" + std::to_string(g.price) +
                   " delta=" + std::to_string(g.delta) +
                   " gamma=" + std::to_string(g.gamma) +
                   " vega=" + std::to_string(g.vega) +
                   " theta=" + std::to_string(g.theta) +
                   " rho=" + std::to_string(g.rho) + ">";
        });

    m.def("black_scholes", &ope::black_scholes, py::arg("inputs"),
          "Closed-form Black-Scholes price and Greeks for a European option.");
}
