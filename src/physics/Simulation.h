// ============================================================================
//  Simulation — FACHADA do núcleo físico (T3.4). Une dilatação temporal (T3.1),
//  perfil de voo acelerado (T3.2) e calendário (T3.3) numa única API que a UI
//  (Marco 4) consome: dada uma viagem, devolve TODAS as métricas + datas + um
//  resumo em linguagem natural.
//
//  Esta é a "porta de entrada" da física para o resto do app. Encapsula a
//  escolha de modo (velocidade constante vs. acelerado) e empacota o resultado
//  conforme a Seção 6 do REQUIREMENTS.
// ============================================================================

#pragma once

#include "physics/Calendar.h"
#include "physics/FlightProfile.h"

#include <string>

namespace starlag::physics {

// Modo de física da viagem (REQUIREMENTS §5: "constante / acelerada").
enum class PhysicsMode {
    ConstantVelocity,  // cruza tudo à velocidade-alvo (ignora acelerar/frear).
    Accelerated,       // perfil hiperbólico com aceleração própria (modo "Completo").
};

// Parâmetros de entrada de uma viagem (o que o usuário define na UI).
struct TripRequest {
    double distanceLy = 0.0;          // distância origem→destino (anos-luz).
    PhysicsMode mode = PhysicsMode::Accelerated;

    double cruiseBeta = 0.99;         // velocidade de cruzeiro (fração de c).
    double accelG = 1.0;              // aceleração própria em múltiplos de g (modo acelerado).

    Date departureDate{2026, 6, 15, 0.0};  // data de partida (gregoriana).

    // Rótulos opcionais para o resumo textual.
    std::string originName = "origem";
    std::string destinationName = "destino";
};

// Resultado completo de uma simulação (REQUIREMENTS §6).
struct SimulationResult {
    // Entrada ecoada (conveniência para a UI).
    double distanceLy = 0.0;
    PhysicsMode mode = PhysicsMode::Accelerated;

    // Métricas relativísticas.
    double peakBeta = 0.0;            // maior velocidade atingida (fração de c).
    double peakGamma = 1.0;           // maior fator de Lorentz atingido.
    double contractedDistanceLy = 0.0;  // distância contraída (referencial da nave).

    // Tempos.
    double properTimeYr = 0.0;        // tripulação (relógio de bordo).
    double coordinateTimeYr = 0.0;    // observador na origem.
    double timeDebtYr = 0.0;          // t − τ: "salto" para o futuro da origem.

    // Datas gregorianas de chegada nos dois referenciais.
    Date arrivalDateOrigin{};         // data na origem ao fim da viagem.
    Date arrivalDateShip{};           // data no relógio de bordo ao fim da viagem.

    // Detalhe do perfil de voo (válido no modo acelerado; default no constante).
    FlightProfileResult flight{};

    // Resumo em linguagem natural (PT-BR), pronto para exibir.
    std::string summary;
};

// Executa a simulação completa para a viagem pedida.
SimulationResult runSimulation(const TripRequest& req);

}  // namespace starlag::physics
