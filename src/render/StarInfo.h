// ============================================================================
//  StarInfo — resumo legível das informações de uma estrela (T2.4).
//
//  Reúne tudo que dá para mostrar sobre uma estrela SEM rede (offline-first):
//  identidade/designação (catálogo HYG) + fotometria + métricas derivadas
//  (luminosidade, distância à origem escolhida). A descrição "enciclopédica"
//  via SIMBAD/Wikipedia fica para a T1.4 (adiada).
//
//  C++ puro: produz strings prontas para o console (T2.4) e, depois, para o
//  painel de info do ImGui (T4.2) — a formatação fica num lugar só.
// ============================================================================

#pragma once

#include "data/Star.h"

#include <string>

namespace starlag::render {

// Nome de exibição: nome próprio se houver; senão designação Bayer/Flamsteed +
// constelação (ex.: "Alp Lyr"); senão um identificador de catálogo (HIP/HD/id).
std::string displayName(const data::Star& star);

// Luminosidade em unidades solares. Usa o campo `lum` do HYG se presente; senão
// estima a partir da magnitude absoluta: L/L☉ = 10^((Msol − M)/2.5), Msol=4.83.
double luminositySolar(const data::Star& star);

// Resumo multi-linha pronto para o console / tooltip. Se `origin` for não-nulo,
// inclui a distância entre `origin` e `star` (para o par origem→destino).
std::string formatStarInfo(const data::Star& star, const data::Star* origin = nullptr);

}  // namespace starlag::render
