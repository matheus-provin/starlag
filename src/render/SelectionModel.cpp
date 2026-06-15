// ============================================================================
//  SelectionModel.cpp — máquina de seleção origem/destino (T4.2).
// ============================================================================

#include "render/SelectionModel.h"

namespace starlag::render {

bool SelectionModel::selectStar(long index) {
    if (index < 0) return false;  // índice inválido: ignora.

    if (!hasOrigin()) {
        // 1ª seleção → origem.
        origin_ = index;
        destination_ = kNone;
        return true;
    }

    if (!hasDestination()) {
        // 2ª seleção → destino, desde que diferente da origem. Clicar de novo na
        // origem nesta fase reinicia (vira nova origem, sem destino).
        if (index == origin_) {
            origin_ = index;
            destination_ = kNone;
            return true;  // (sem mudança efetiva de origem, mas estado normalizado)
        }
        destination_ = index;
        return true;
    }

    // Já completa → reinicia: o índice clicado é a nova origem.
    origin_ = index;
    destination_ = kNone;
    return true;
}

}  // namespace starlag::render
