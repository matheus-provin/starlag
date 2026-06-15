// ============================================================================
//  StarColor — cor de renderização de uma estrela (T2.2).
//
//  Converte os dados fotométricos do HYG (índice de cor B−V e tipo espectral)
//  numa cor RGB plausível, seguindo a sequência OBAFGKM (azul → branco → amarelo
//  → laranja → vermelho). C++ puro: nenhuma dependência de Metal/ObjC, então é
//  100% testável headless contra estrelas conhecidas.
//
//  Pipeline da cor:
//    1. B−V (campo `ci`) → temperatura efetiva (aprox. de Ballesteros 2012).
//    2. Temperatura → RGB (aprox. de corpo negro de Tanner Helland).
//  Quando `ci` está ausente, caímos para a classe espectral (1ª letra de
//  `spect`), mapeada a uma temperatura típica daquela classe.
// ============================================================================

#pragma once

#include "data/Star.h"

#include <string>

namespace starlag::render {

// Cor RGB em ponto flutuante, componentes em [0, 1].
struct Rgb {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
};

// --- Conversões expostas (para teste e reuso) -------------------------------

// Índice de cor B−V → temperatura efetiva em Kelvin (Ballesteros 2012).
// Válida para a faixa típica de B−V de estrelas (~ −0.4 a +2.0).
double colorIndexToTemperature(double bv);

// Temperatura (K) → RGB normalizado (aprox. de corpo negro). T é limitada a uma
// faixa sensata (~1000–40000 K) para evitar valores degenerados.
Rgb temperatureToRgb(double kelvin);

// Classe espectral (1ª letra de `spect`: O,B,A,F,G,K,M) → temperatura típica.
// Letra desconhecida → temperatura solar (fallback neutro).
double spectralClassToTemperature(char spectralClass);

// --- Cor final de uma estrela -----------------------------------------------
// Usa `ci` (B−V) se presente; senão, a classe espectral; senão, branco-solar.
Rgb starColor(const data::Star& star);

}  // namespace starlag::render
