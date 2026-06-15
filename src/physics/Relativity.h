// ============================================================================
//  Relativity — núcleo de Relatividade Restrita: dilatação temporal a
//  velocidade CONSTANTE (T3.1). O perfil acelerado (hiperbólico) entra na T3.2.
//
//  Modelo: espaço plano de Minkowski. Tudo em `double`. As unidades naturais do
//  app são anos-luz (ly) e anos julianos (yr), com c = 1 ly/yr — assim a viagem
//  a velocidade constante tem tempo coordenado = distância_ly / β, numericamente
//  estável e direto.
//
//  Símbolos:
//    β (beta)  = v / c            (fração da velocidade da luz, 0..<1)
//    γ (gamma) = 1/√(1 − β²)      (fator de Lorentz)
//    t         = tempo coordenado (observador na origem)
//    τ (tau)   = tempo próprio    (relógio de bordo / tripulação)
// ============================================================================

#pragma once

namespace starlag::physics {

// Resultado de uma viagem a velocidade constante.
struct ConstVelocityTrip {
    double beta = 0.0;             // β efetivamente usado (após clamp).
    double gamma = 1.0;            // fator de Lorentz γ.
    double distanceLy = 0.0;       // distância no referencial da origem (ly).
    double coordinateTimeYr = 0.0; // tempo para o observador na origem (yr).
    double properTimeYr = 0.0;     // tempo para a tripulação (yr).
    double contractedDistanceLy = 0.0; // distância contraída vista pela nave (ly).
};

// Garante 0 ≤ β ≤ kMaxBeta. Valores negativos viram 0; acima do limite, saturam
// em kMaxBeta (nunca permitimos β ≥ 1). Retorna o β válido.
double clampBeta(double beta);

// Fator de Lorentz γ = 1/√(1 − β²).
// Implementação numericamente estável perto de c: usa (1−β)(1+β) em vez de
// 1−β², evitando o cancelamento catastrófico quando β → 1.
// Pré-condição: 0 ≤ β < 1 (use clampBeta antes se a entrada for crua).
double lorentzFactor(double beta);

// Tempo próprio a partir do tempo coordenado: τ = t / γ.
double properTimeFromCoordinate(double coordinateTime, double gamma);

// Tempo coordenado a partir do tempo próprio: t = τ · γ.
double coordinateTimeFromProper(double properTime, double gamma);

// Calcula uma viagem completa a velocidade constante.
//   distanceLy : distância no referencial da origem (anos-luz), ≥ 0.
//   beta       : velocidade de cruzeiro (será passada por clampBeta).
// Em unidades naturais (c = 1 ly/yr): t = distância/β, τ = t/γ,
// distância contraída = distância/γ.
ConstVelocityTrip computeConstVelocityTrip(double distanceLy, double beta);

}  // namespace starlag::physics
