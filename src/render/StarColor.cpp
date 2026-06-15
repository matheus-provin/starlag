// ============================================================================
//  StarColor.cpp — implementação da cor de estrelas (T2.2).
// ============================================================================

#include "render/StarColor.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace starlag::render {

namespace {

// Limita x ao intervalo [lo, hi].
double clampd(double x, double lo, double hi) {
    return std::max(lo, std::min(hi, x));
}

}  // namespace

double colorIndexToTemperature(double bv) {
    // Ballesteros (2012): T = 4600 * (1/(0.92·BV+1.7) + 1/(0.92·BV+0.62)).
    // Limitamos BV à faixa estelar para evitar singularidades nos denominadores.
    const double x = clampd(bv, -0.4, 2.0);
    const double a = 0.92 * x;
    return 4600.0 * (1.0 / (a + 1.7) + 1.0 / (a + 0.62));
}

Rgb temperatureToRgb(double kelvin) {
    // Aproximação de corpo negro de Tanner Helland. Trabalha em "centikelvin".
    const double T = clampd(kelvin, 1000.0, 40000.0);
    const double t = T / 100.0;

    double r, g, b;

    // Vermelho.
    if (t <= 66.0) {
        r = 255.0;
    } else {
        r = 329.698727446 * std::pow(t - 60.0, -0.1332047592);
    }

    // Verde.
    if (t <= 66.0) {
        g = 99.4708025861 * std::log(t) - 161.1195681661;
    } else {
        g = 288.1221695283 * std::pow(t - 60.0, -0.0755148492);
    }

    // Azul.
    if (t >= 66.0) {
        b = 255.0;
    } else if (t <= 19.0) {
        b = 0.0;
    } else {
        b = 138.5177312231 * std::log(t - 10.0) - 305.0447927307;
    }

    Rgb c;
    c.r = static_cast<float>(clampd(r, 0.0, 255.0) / 255.0);
    c.g = static_cast<float>(clampd(g, 0.0, 255.0) / 255.0);
    c.b = static_cast<float>(clampd(b, 0.0, 255.0) / 255.0);
    return c;
}

double spectralClassToTemperature(char spectralClass) {
    // Temperatura representativa de cada classe (ponto médio aproximado).
    switch (std::toupper(static_cast<unsigned char>(spectralClass))) {
        case 'O': return 35000.0;  // azul.
        case 'B': return 15000.0;  // azul-branca.
        case 'A': return 8500.0;   // branca.
        case 'F': return 6500.0;   // branco-amarelada.
        case 'G': return 5700.0;   // amarela (Sol).
        case 'K': return 4500.0;   // laranja.
        case 'M': return 3200.0;   // vermelha.
        default:  return 5778.0;   // desconhecida → solar (neutro).
    }
}

Rgb starColor(const data::Star& star) {
    // 1ª escolha: índice de cor B−V (medição direta, mais confiável).
    if (star.hasCi) {
        return temperatureToRgb(colorIndexToTemperature(star.ci));
    }
    // 2ª escolha: classe espectral (1ª letra de `spect`).
    if (!star.spect.empty()) {
        return temperatureToRgb(spectralClassToTemperature(star.spect[0]));
    }
    // Fallback: branco-solar.
    return temperatureToRgb(5778.0);
}

}  // namespace starlag::render
