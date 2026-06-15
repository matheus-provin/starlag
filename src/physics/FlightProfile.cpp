// ============================================================================
//  FlightProfile.cpp — implementação do perfil de voo acelerado (T3.2).
// ============================================================================

#include "physics/FlightProfile.h"
#include "physics/Constants.h"
#include "physics/Relativity.h"

#include <algorithm>  // std::max
#include <cmath>      // std::sinh, std::cosh, std::atanh, std::acosh, std::sqrt

namespace starlag::physics {

namespace {

// Caracteriza UMA fase de aceleração que parte do repouso até atingir `betaC`,
// com aceleração própria `a` (ly/yr²). Preenche τ, t e distância da fase.
struct AccelPhase {
    double properTimeYr = 0.0;   // τ₁ = (1/a)·atanh(βC)
    double coordTimeYr = 0.0;    // t₁ = (1/a)·sinh(a·τ₁) = βC·γC/a
    double distanceLy = 0.0;     // d₁ = (1/a)·(γC − 1)
    double gammaC = 1.0;
};

AccelPhase accelPhaseToBeta(double a, double betaC) {
    AccelPhase ph;
    ph.gammaC = lorentzFactor(betaC);          // γ estável perto de c.
    ph.properTimeYr = std::atanh(betaC) / a;   // τ₁
    ph.coordTimeYr = std::sinh(a * ph.properTimeYr) / a;  // t₁
    ph.distanceLy = (ph.gammaC - 1.0) / a;     // d₁
    return ph;
}

}  // namespace

FlightProfileResult computeAcceleratedTrip(double distanceLy,
                                           double accel_ly_yr2,
                                           double targetBeta) {
    FlightProfileResult r;
    r.distanceLy = std::max(0.0, distanceLy);
    r.accel_ly_yr2 = accel_ly_yr2;
    r.targetBeta = clampBeta(targetBeta);

    // Casos degenerados: sem distância ou sem aceleração → viagem nula.
    if (r.distanceLy <= 0.0 || r.accel_ly_yr2 <= 0.0 || r.targetBeta <= 0.0) {
        r.kind = FlightKind::Triangular;
        r.peakBeta = 0.0;
        r.peakGamma = 1.0;
        return r;
    }

    const double a = r.accel_ly_yr2;

    // Distância necessária para acelerar do repouso até β_alvo (e, por simetria,
    // a mesma para desacelerar). Se 2·d₁ couber em D, há cruzeiro.
    const AccelPhase phase = accelPhaseToBeta(a, r.targetBeta);
    const double distTwoPhases = 2.0 * phase.distanceLy;

    if (distTwoPhases <= r.distanceLy) {
        // --------- Perfil TRAPEZOIDAL (acelera–coast–desacelera) ----------
        r.kind = FlightKind::Trapezoidal;
        r.peakBeta = r.targetBeta;
        r.peakGamma = phase.gammaC;

        // Fases de aceleração/desaceleração (idênticas).
        r.accelProperTimeYr = phase.properTimeYr;
        r.accelCoordTimeYr = phase.coordTimeYr;
        r.accelDistanceLy = phase.distanceLy;

        // Trecho de cruzeiro a β_alvo constante cobrindo a distância restante.
        r.coastDistanceLy = r.distanceLy - distTwoPhases;
        r.coastCoordTimeYr = r.coastDistanceLy / r.targetBeta;       // t = d/β
        r.coastProperTimeYr = r.coastCoordTimeYr / phase.gammaC;     // τ = t/γ

        r.properTimeYr = 2.0 * phase.properTimeYr + r.coastProperTimeYr;
        r.coordinateTimeYr = 2.0 * phase.coordTimeYr + r.coastCoordTimeYr;
    } else {
        // --------------- Perfil TRIANGULAR (viagem curta) -----------------
        // Acelera até o ponto médio (D/2) e desacelera. β_alvo nunca é atingido.
        // De d = (1/a)(γ − 1) com d = D/2:  γ_pico = 1 + a·(D/2).
        r.kind = FlightKind::Triangular;

        const double halfDist = r.distanceLy / 2.0;
        const double gammaPeak = 1.0 + a * halfDist;
        // β a partir de γ: β = √(1 − 1/γ²). Estável pois γ ≥ 1.
        const double betaPeak = std::sqrt(1.0 - 1.0 / (gammaPeak * gammaPeak));

        r.peakGamma = gammaPeak;
        r.peakBeta = betaPeak;

        // τ de uma fase: γ = cosh(a·τ) → τ = acosh(γ)/a.
        const double tauHalf = std::acosh(gammaPeak) / a;
        const double tHalf = std::sinh(a * tauHalf) / a;  // t = (1/a)sinh(a·τ)

        r.accelProperTimeYr = tauHalf;
        r.accelCoordTimeYr = tHalf;
        r.accelDistanceLy = halfDist;
        r.coastProperTimeYr = 0.0;
        r.coastCoordTimeYr = 0.0;
        r.coastDistanceLy = 0.0;

        r.properTimeYr = 2.0 * tauHalf;
        r.coordinateTimeYr = 2.0 * tHalf;
    }

    return r;
}

}  // namespace starlag::physics
