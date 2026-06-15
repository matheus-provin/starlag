// ============================================================================
//  Star — modelo em memória de uma estrela do catálogo HYG (T1.2).
//
//  Campos conforme REQUIREMENTS §3.2. Mantemos a distância tanto em parsecs
//  (como vem no CSV) quanto em anos-luz (unidade natural da física do app).
//  Posição (x,y,z) vem em parsecs no sistema equatorial cartesiano do HYG.
// ============================================================================

#pragma once

#include <cstdint>
#include <string>

namespace starlag::data {

struct Star {
    // --- Identificadores ---
    int64_t id = -1;          // id interno do HYG (0 = Sol).
    int64_t hip = 0;          // Hipparcos (0 = ausente).
    int64_t hd = 0;           // Henry Draper (0 = ausente).
    std::string gl;           // Gliese (texto; vazio = ausente).
    std::string proper;       // nome próprio (ex.: "Vega"; vazio = sem nome).

    // --- Posição (parsecs, equatorial cartesiano) ---
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    // --- Distância ---
    double distPc = 0.0;      // distância em parsecs (campo `dist` do HYG).
    double distLy = 0.0;      // distância em anos-luz (derivada: distPc * kParsec_ly).

    // --- Fotometria / classificação ---
    double mag = 0.0;         // magnitude aparente.
    double absmag = 0.0;      // magnitude absoluta.
    std::string spect;        // tipo espectral (ex.: "G2V").
    double ci = 0.0;          // índice de cor B−V (para a cor de renderização).
    bool hasCi = false;       // o campo ci estava presente? (0.0 é um valor válido).

    // --- Designação / catálogo (T2.4: enriquecem o painel de info) ---
    std::string con;          // constelação (abrev. IAU, ex.: "Lyr").
    std::string bayer;        // letra de Bayer (ex.: "Alp" para α).
    std::string flam;         // número de Flamsteed (ex.: "3").
    double lum = 0.0;         // luminosidade em unidades solares (L/L☉).
    bool hasLum = false;      // o campo lum estava presente?

    // Conveniência: a estrela tem um nome próprio legível?
    bool hasProperName() const { return !proper.empty(); }
};

}  // namespace starlag::data
