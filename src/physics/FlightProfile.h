// ============================================================================
//  FlightProfile — perfil de voo com ACELERAÇÃO PRÓPRIA CONSTANTE (T3.2).
//
//  Modelo "foguete relativístico" em espaço plano (Minkowski): a nave acelera
//  com aceleração própria constante `a` (sentida pela tripulação, ex.: 1 g),
//  faz cruzeiro (coast) à velocidade-alvo se houver distância para isso, e
//  desacelera simetricamente até parar no destino.
//
//  Fórmulas hiperbólicas do movimento uniformemente acelerado (c em unidades
//  naturais = 1 ly/yr; a em ly/yr²; τ tempo próprio; t tempo coordenado):
//    β(τ) = tanh(a·τ)
//    γ(τ) = cosh(a·τ)
//    d(τ) = (1/a)·(cosh(a·τ) − 1)
//    t(τ) = (1/a)·sinh(a·τ)
//
//  Perfis:
//   - Trapezoidal (acelera–coast–desacelera): se a distância das fases de
//     aceleração+desaceleração couber em D, há um trecho de cruzeiro a β_alvo.
//   - Triangular (viagem curta): se não couber, a nave acelera até o ponto
//     médio e desacelera; nunca atinge β_alvo. γ de pico < γ_alvo.
//
//  Tudo em `double`. Reusa o γ numericamente estável de Relativity.
// ============================================================================

#pragma once

namespace starlag::physics {

// Qual perfil de voo foi efetivamente executado.
enum class FlightKind {
    Trapezoidal,  // atingiu a velocidade de cruzeiro (há fase de coast).
    Triangular,   // viagem curta: acelera até o meio e desacelera (sem coast).
};

// Resultado de uma viagem com aceleração própria constante.
struct FlightProfileResult {
    FlightKind kind = FlightKind::Triangular;

    double distanceLy = 0.0;        // distância total (referencial da origem).
    double accel_ly_yr2 = 0.0;      // aceleração própria usada (ly/yr²).
    double targetBeta = 0.0;        // β de cruzeiro pedido (após clamp).

    double peakBeta = 0.0;          // maior β efetivamente atingido.
    double peakGamma = 1.0;         // maior γ (na velocidade de pico).

    double properTimeYr = 0.0;      // tempo total para a tripulação (τ).
    double coordinateTimeYr = 0.0;  // tempo total para a origem (t).

    // Decomposição por fase (úteis para a UI/animação da timeline).
    double accelProperTimeYr = 0.0;     // τ de UMA fase de aceleração.
    double accelCoordTimeYr = 0.0;      // t de UMA fase de aceleração.
    double accelDistanceLy = 0.0;       // distância de UMA fase de aceleração.
    double coastProperTimeYr = 0.0;     // τ do trecho de cruzeiro (0 se triangular).
    double coastCoordTimeYr = 0.0;      // t do trecho de cruzeiro (0 se triangular).
    double coastDistanceLy = 0.0;       // distância do cruzeiro (0 se triangular).
};

// Calcula a viagem acelerada.
//   distanceLy   : distância no referencial da origem (ly), ≥ 0.
//   accel_ly_yr2 : aceleração própria (ly/yr²); use gToLyPerYr2() p/ múltiplos de g.
//   targetBeta   : velocidade de cruzeiro desejada (passada por clampBeta).
// Se a distância for curta demais para atingir targetBeta, retorna perfil
// triangular com o β/γ de pico realmente alcançado.
FlightProfileResult computeAcceleratedTrip(double distanceLy,
                                           double accel_ly_yr2,
                                           double targetBeta);

}  // namespace starlag::physics
