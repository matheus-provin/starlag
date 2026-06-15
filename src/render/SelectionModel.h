// ============================================================================
//  SelectionModel — máquina de seleção origem/destino (T4.2).
//
//  Unifica os dois caminhos de seleção (clique no espaço 3D, T2.4; e busca
//  textual na UI, T4.2) numa única lógica testável. C++ puro: índices na lista
//  de estrelas do catálogo; nenhuma dependência de render/UI.
//
//  Máquina de estados de `selectStar(index)`:
//    - sem origem        → define ORIGEM;
//    - origem, sem destino, índice ≠ origem → define DESTINO;
//    - origem + destino (ou clique na própria origem na fase de destino)
//                        → reinicia: o índice clicado vira a NOVA origem.
//  `setOrigin`/`setDestination` permitem atribuição direta (botões da UI).
// ============================================================================

#pragma once

#include <cstddef>

namespace starlag::render {

class SelectionModel {
public:
    static constexpr long kNone = -1;

    long origin() const { return origin_; }
    long destination() const { return destination_; }
    bool hasOrigin() const { return origin_ != kNone; }
    bool hasDestination() const { return destination_ != kNone; }
    bool complete() const { return hasOrigin() && hasDestination(); }

    // Aplica a máquina de estados (clique/seleção). Retorna true se a seleção
    // mudou (para o chamador reconstruir marcadores / limpar resultado).
    bool selectStar(long index);

    // Atribuição direta (botões "definir como origem/destino" da UI).
    void setOrigin(long index) { origin_ = index; }
    void setDestination(long index) { destination_ = index; }

    // Zera a seleção.
    void clear() { origin_ = kNone; destination_ = kNone; }

private:
    long origin_ = kNone;
    long destination_ = kNone;
};

}  // namespace starlag::render
