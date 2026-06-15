// ============================================================================
//  Simulation.cpp — implementação da fachada do núcleo físico (T3.4).
// ============================================================================

#include "physics/Simulation.h"

#include "physics/Constants.h"
#include "physics/Relativity.h"

#include <cmath>
#include <cstdio>

namespace starlag::physics {

namespace {

// Monta o resumo em linguagem natural (REQUIREMENTS §6) a partir do resultado.
std::string buildSummary(const TripRequest& req, const SimulationResult& r) {
    char buf[512];

    // Escolhe a unidade de tempo mais legível para o tempo próprio.
    auto humanYears = [](double years) -> std::string {
        char b[64];
        if (years < 2.0) {
            std::snprintf(b, sizeof(b), "%.0f dias", years * kJulianYear_days);
        } else {
            std::snprintf(b, sizeof(b), "%.1f anos", years);
        }
        return std::string(b);
    };

    std::snprintf(
        buf, sizeof(buf),
        "Voce chegou a %s. Para a tripulacao passaram-se %s (chegada em %s no "
        "relogio de bordo); na %s, %.1f anos (ano %lld). Voce 'saltou' %.1f anos "
        "para o futuro. Velocidade maxima: %.4f c (gamma=%.2f).",
        req.destinationName.c_str(),
        humanYears(r.properTimeYr).c_str(),
        formatDate(r.arrivalDateShip).c_str(),
        req.originName.c_str(),
        r.coordinateTimeYr,
        static_cast<long long>(r.arrivalDateOrigin.year),
        r.timeDebtYr,
        r.peakBeta, r.peakGamma);

    return std::string(buf);
}

}  // namespace

SimulationResult runSimulation(const TripRequest& req) {
    SimulationResult r;
    r.distanceLy = req.distanceLy;
    r.mode = req.mode;

    if (req.mode == PhysicsMode::ConstantVelocity) {
        // --- Modo velocidade constante (T3.1) ---
        ConstVelocityTrip t = computeConstVelocityTrip(req.distanceLy, req.cruiseBeta);
        r.peakBeta = t.beta;
        r.peakGamma = t.gamma;
        r.contractedDistanceLy = t.contractedDistanceLy;  // D/γ (exato neste modo).
        r.properTimeYr = t.properTimeYr;
        r.coordinateTimeYr = t.coordinateTimeYr;
        // flight fica com default (não há perfil acelerado).
    } else {
        // --- Modo acelerado (T3.2) ---
        const double a = gToLyPerYr2(req.accelG);
        FlightProfileResult f = computeAcceleratedTrip(req.distanceLy, a, req.cruiseBeta);
        r.flight = f;
        r.peakBeta = f.peakBeta;
        r.peakGamma = f.peakGamma;
        r.properTimeYr = f.properTimeYr;
        r.coordinateTimeYr = f.coordinateTimeYr;
        // Distância contraída: no modo acelerado γ varia ao longo da rota, então
        // usamos D/γ_pico = o encurtamento MÁXIMO percebido (na velocidade de pico).
        // É uma métrica representativa (não a "distância própria" integrada, que
        // exigiria integrar ao longo da trajetória — fora do escopo do MVP).
        r.contractedDistanceLy =
            (r.peakGamma > 0.0) ? req.distanceLy / r.peakGamma : req.distanceLy;
    }

    // "Dívida temporal": quanto a origem envelheceu a mais que a tripulação.
    r.timeDebtYr = r.coordinateTimeYr - r.properTimeYr;

    // Datas gregorianas de chegada nos dois referenciais (T3.3).
    r.arrivalDateOrigin = addYears(req.departureDate, r.coordinateTimeYr);
    r.arrivalDateShip = addYears(req.departureDate, r.properTimeYr);

    // Resumo em linguagem natural.
    r.summary = buildSummary(req, r);

    return r;
}

}  // namespace starlag::physics
