// ============================================================================
//  StarField — empacota o catálogo em instâncias prontas para a GPU (T2.2).
//
//  Converte um std::vector<Star> num buffer plano de instâncias de render, cada
//  uma com posição (parsecs, igual à grade da T2.1), cor RGB (via StarColor) e
//  um tamanho de ponto derivado da magnitude aparente (mais brilhante = maior).
//
//  C++ puro (sem Metal): a montagem é testável headless. O buffer resultante é
//  um vetor contíguo de floats que o MetalWindow copia direto para um MTLBuffer.
// ============================================================================

#pragma once

#include "data/Star.h"
#include "render/StarColor.h"

#include <cstddef>
#include <vector>

namespace starlag::render {

// Uma instância de estrela como vai para a GPU. Layout deve casar com o vertex
// descriptor do shader stars.metal: posição(3) + cor(3) + tamanho(1) = 7 floats.
struct StarInstance {
    float px, py, pz;   // posição em parsecs.
    float cr, cg, cb;   // cor RGB [0,1].
    float size;         // tamanho do ponto em pixels (base; o shader pode escalar).
};

// Parâmetros do mapeamento magnitude → tamanho do ponto.
struct StarFieldParams {
    // Magnitudes de referência: estrelas em `brightMag` ou menos recebem
    // `maxSize`; em `faintMag` ou mais, `minSize`; entre elas, interpola linear.
    double brightMag = 0.0;   // ~estrelas de 1ª grandeza e mais brilhantes.
    double faintMag = 8.0;    // ~limite de visibilidade a olho nu / fundo.
    float minSize = 1.5f;     // tamanho mínimo (pixels) das mais fracas.
    float maxSize = 9.0f;     // tamanho máximo (pixels) das mais brilhantes.
};

// Constrói o buffer de instâncias a partir das estrelas.
std::vector<StarInstance> buildStarField(const std::vector<data::Star>& stars,
                                         const StarFieldParams& params = {});

// Mapeia uma magnitude aparente para o tamanho do ponto (exposta p/ teste).
// Monotonicamente decrescente: mag menor (mais brilhante) → tamanho maior.
float magnitudeToSize(double mag, const StarFieldParams& params);

}  // namespace starlag::render
