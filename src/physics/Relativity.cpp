// ============================================================================
//  Relativity.cpp — implementação do núcleo de dilatação temporal (T3.1).
// ============================================================================

#include "physics/Relativity.h"
#include "physics/Constants.h"

#include <algorithm>  // std::clamp
#include <cmath>      // std::sqrt

namespace starlag::physics {

double clampBeta(double beta) {
    // NaN vira 0 (entrada inválida tratada como "parado"); demais valores são
    // limitados a [0, kMaxBeta]. std::clamp não trata NaN, então filtramos antes.
    if (std::isnan(beta)) {
        return 0.0;
    }
    return std::clamp(beta, 0.0, kMaxBeta);
}

double lorentzFactor(double beta) {
    // γ = 1/√(1 − β²). Reescrevemos 1 − β² = (1 − β)(1 + β): quando β → 1, o
    // fator (1 − β) carrega toda a perda de precisão de forma controlada, sem o
    // cancelamento catastrófico de calcular β*β e subtrair de 1 (onde, para
    // β ≈ 0.9999999, β² fica tão perto de 1 que dígitos significativos somem).
    const double oneMinusBetaSq = (1.0 - beta) * (1.0 + beta);
    return 1.0 / std::sqrt(oneMinusBetaSq);
}

double properTimeFromCoordinate(double coordinateTime, double gamma) {
    return coordinateTime / gamma;
}

double coordinateTimeFromProper(double properTime, double gamma) {
    return properTime * gamma;
}

ConstVelocityTrip computeConstVelocityTrip(double distanceLy, double beta) {
    ConstVelocityTrip trip;

    // Sanitização das entradas.
    trip.distanceLy = std::max(0.0, distanceLy);
    trip.beta = clampBeta(beta);
    trip.gamma = lorentzFactor(trip.beta);

    if (trip.beta <= 0.0 || trip.distanceLy <= 0.0) {
        // Sem movimento (ou sem distância): nenhum tempo decorre na viagem.
        // γ permanece 1 (β=0) e os tempos ficam em 0. Evita divisão por zero.
        trip.coordinateTimeYr = 0.0;
        trip.properTimeYr = 0.0;
        trip.contractedDistanceLy = trip.distanceLy;
        return trip;
    }

    // Unidades naturais: c = 1 ly/yr.
    //   tempo coordenado t = distância / β   (anos)
    //   tempo próprio    τ = t / γ
    //   distância contraída (referencial da nave) = distância / γ
    trip.coordinateTimeYr = trip.distanceLy / trip.beta;
    trip.properTimeYr = properTimeFromCoordinate(trip.coordinateTimeYr, trip.gamma);
    trip.contractedDistanceLy = trip.distanceLy / trip.gamma;

    return trip;
}

}  // namespace starlag::physics
