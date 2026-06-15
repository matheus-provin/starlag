// ============================================================================
//  InputState — estado de input por frame, desacoplado do GLFW (T2.3).
//
//  O MetalWindow preenche este struct a cada pollEvents() (lendo o GLFW por
//  baixo), e o FlyCameraController o consome para mover a câmera. Assim a lógica
//  de navegação não depende de GLFW (é C++ puro e testável headless).
//
//  Convenções:
//    - Os booleanos são o estado ATUAL (segurando) das teclas naquele frame.
//    - mouseDx/mouseDy são o DELTA do mouse desde o frame anterior (pixels),
//      válidos apenas quando looking == true (botão direito segurando).
//    - scrollDelta é o acumulado da roda desde o último frame (dolly/zoom).
// ============================================================================

#pragma once

namespace starlag::render {

struct InputState {
    // Movimento (WASD + subir/descer).
    bool forward = false;   // W
    bool back = false;      // S
    bool left = false;      // A
    bool right = false;     // D
    bool up = false;        // E / Space
    bool down = false;      // Q / Ctrl

    // Ajuste de velocidade de voo por teclas dedicadas (passo multiplicativo).
    bool speedUp = false;     // '=' / ']'
    bool speedDown = false;   // '-' / '['

    // Mouse-look: delta desde o último frame (só quando `looking`).
    bool looking = false;     // botão direito segurando.
    double mouseDx = 0.0;
    double mouseDy = 0.0;

    // Roda do mouse: dolly (avança/recua a câmera ao longo do forward).
    double scrollDelta = 0.0;

    // Seleção (T2.4): posição do cursor (pixels, origem no topo-esq.) e um
    // pulso de clique esquerdo (true apenas no frame em que o botão foi solto,
    // i.e. um clique completo — não enquanto arrasta para olhar).
    double cursorX = 0.0;
    double cursorY = 0.0;
    bool clicked = false;
};

}  // namespace starlag::render
