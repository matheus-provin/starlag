// ============================================================================
//  StarInfo.cpp — formatação das informações de uma estrela (T2.4).
// ============================================================================

#include "render/StarInfo.h"

#include "physics/Constants.h"

#include <cmath>
#include <sstream>

namespace starlag::render {

namespace {

// Distância euclidiana (parsecs) entre duas estrelas.
double distancePc(const data::Star& a, const data::Star& b) {
    const double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

std::string displayName(const data::Star& star) {
    if (star.hasProperName()) return star.proper;

    // Designação Bayer/Flamsteed + constelação (ex.: "Alp Lyr", "3 Lyr").
    std::string desig;
    if (!star.bayer.empty()) desig = star.bayer;
    else if (!star.flam.empty()) desig = star.flam;
    if (!desig.empty() && !star.con.empty()) return desig + " " + star.con;
    if (!desig.empty()) return desig;

    // Fallback: identificador de catálogo.
    if (star.hip != 0) return "HIP " + std::to_string(star.hip);
    if (star.hd != 0) return "HD " + std::to_string(star.hd);
    if (!star.gl.empty()) return star.gl;
    return "Estrela #" + std::to_string(star.id);
}

double luminositySolar(const data::Star& star) {
    if (star.hasLum && star.lum > 0.0) return star.lum;
    // Estimativa pela magnitude absoluta (magnitude bolométrica aproximada):
    // L/L☉ = 10^((M☉ − M) / 2.5), com M☉ ≈ 4.83.
    constexpr double kSolarAbsMag = 4.83;
    return std::pow(10.0, (kSolarAbsMag - star.absmag) / 2.5);
}

std::string formatStarInfo(const data::Star& star, const data::Star* origin) {
    std::ostringstream os;
    os << "=== " << displayName(star) << " ===\n";

    // Identidade / designação.
    if (star.hasProperName() && (!star.bayer.empty() || !star.flam.empty())
        && !star.con.empty()) {
        os << "  Designacao   : ";
        if (!star.bayer.empty()) os << star.bayer << " ";
        else if (!star.flam.empty()) os << star.flam << " ";
        os << star.con << "\n";
    } else if (!star.con.empty()) {
        os << "  Constelacao  : " << star.con << "\n";
    }
    if (!star.spect.empty()) os << "  Tipo espectral: " << star.spect << "\n";

    // Catálogo.
    os << "  Catalogo     : id " << star.id;
    if (star.hip != 0) os << " | HIP " << star.hip;
    if (star.hd != 0) os << " | HD " << star.hd;
    if (!star.gl.empty()) os << " | " << star.gl;
    os << "\n";

    // Distância (do Sol) — o HYG dá a distância heliocêntrica.
    os << "  Distancia    : " << star.distPc << " pc  (" << star.distLy << " anos-luz do Sol)\n";

    // Fotometria.
    os << "  Magnitude    : aparente " << star.mag << " | absoluta " << star.absmag << "\n";
    os << "  Luminosidade : " << luminositySolar(star) << " L(sol)";
    if (!star.hasLum) os << " (estimada de absmag)";
    os << "\n";

    // Distância da origem escolhida (par origem→destino), se fornecida.
    if (origin != nullptr) {
        const double dPc = distancePc(*origin, star);
        const double dLy = dPc * physics::kParsec_ly;
        os << "  --\n";
        os << "  De " << displayName(*origin) << " ate aqui: "
           << dPc << " pc  (" << dLy << " anos-luz)\n";
    }
    return os.str();
}

}  // namespace starlag::render
