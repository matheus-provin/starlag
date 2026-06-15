// ============================================================================
//  starlag — Simulador 3D de Viagens Interestelares Relativísticas
//  Ponto de entrada do aplicativo.
//
//  Estado atual: abre uma janela Metal (via GLFW) com a tela limpa (M0/T0.2).
//  Antes de abrir a janela, imprime no terminal um "relatório de viagem" de
//  exemplo (Sol→Vega) usando o núcleo de física já validado (M3: T3.1/T3.2),
//  para tornar a física tangível dentro do app real — não só nos testes.
//  Render de estrelas (M2) e UI ImGui (M4) entram nos próximos marcos.
// ============================================================================

#include "data/CatalogParser.h"
#include "physics/Calendar.h"
#include "physics/Constants.h"
#include "physics/Simulation.h"
#include "render/Camera.h"
#include "render/FlyCameraController.h"
#include "render/MetalWindow.h"
#include "render/Picker.h"
#include "render/StarField.h"
#include "render/StarInfo.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <vector>

namespace {

// Imprime uma simulação usando a fachada runSimulation (T3.4): empacota todas as
// métricas + datas + resumo em linguagem natural numa única chamada.
void printResult(const char* titulo, const starlag::physics::SimulationResult& r) {
    using namespace starlag::physics;
    std::printf("\n[%s]\n", titulo);
    std::printf("  Velocidade de pico       : %.5f c (gamma=%.3f)\n", r.peakBeta, r.peakGamma);
    std::printf("  Tempo na origem          : %.3f anos  -> chegada %s\n",
                r.coordinateTimeYr, formatDate(r.arrivalDateOrigin).c_str());
    std::printf("  Tempo a bordo (tripulac.): %.3f anos  -> relogio  %s\n",
                r.properTimeYr, formatDate(r.arrivalDateShip).c_str());
    std::printf("  Distancia contraida      : %.3f anos-luz\n", r.contractedDistanceLy);
    std::printf("  'Divida temporal'        : %.3f anos para o futuro\n", r.timeDebtYr);
    std::printf("  Resumo: %s\n", r.summary.c_str());
}

// "Prova de vida" da física dentro do executável do app (Sol→Vega, dois modos).
void printDemoTrip() {
    using namespace starlag::physics;

    TripRequest base;
    base.distanceLy = 25.04;
    base.departureDate = Date{2026, 6, 15, 0.0};
    base.originName = "Terra";
    base.destinationName = "Vega";

    std::printf("\n=================================================\n");
    std::printf("  starlag — relatorio de viagem (demo de fisica)\n");
    std::printf("  Rota: Sol -> %s  (%.2f anos-luz)\n",
                base.destinationName.c_str(), base.distanceLy);
    std::printf("  Partida: %s\n", formatDate(base.departureDate).c_str());
    std::printf("=================================================\n");

    TripRequest constReq = base;
    constReq.mode = PhysicsMode::ConstantVelocity;
    constReq.cruiseBeta = 0.99;
    printResult("Modo velocidade constante @ 99% c", runSimulation(constReq));

    TripRequest accelReq = base;
    accelReq.mode = PhysicsMode::Accelerated;
    accelReq.accelG = 1.0;
    accelReq.cruiseBeta = 0.9999999;
    printResult("Modo acelerado @ 1 g", runSimulation(accelReq));

    std::printf("\n");
}

}  // namespace

int main() {
    using clock = std::chrono::steady_clock;

    try {
        // Prova de vida da física dentro do app (antes de abrir a janela).
        printDemoTrip();

        // --- Carrega o catálogo HYG e monta o campo de estrelas (T2.2) ---
        // `catalog.stars` é mantido vivo o loop todo: o picking (T2.4) consulta
        // as posições das estrelas a cada clique.
        const starlag::data::ParseReport catalog =
            starlag::data::parseCatalogFile(STARLAG_CATALOG_PATH);
        std::vector<starlag::render::StarInstance> starField;
        if (catalog.ok) {
            starField = starlag::render::buildStarField(catalog.stars);
            std::printf("Catalogo: %zu estrelas carregadas para render.\n",
                        starField.size());
        } else {
            std::printf("Aviso: catalogo nao carregado (%s). Render so da grade.\n",
                        catalog.message.c_str());
        }

        const int winW = 1280, winH = 720;
        starlag::render::MetalWindow window(winW, winH, "starlag");
        // Azul-noite escuro: o céu profundo onde as estrelas são desenhadas.
        const starlag::render::ClearColor skyColor{0.02, 0.02, 0.06, 1.0};

        // Câmera 3D fly (T2.1/T2.3): começa acima e atrás da origem, orientada
        // para olhar o Sol. Perspectiva de 60°, far amplo p/ a escala estelar.
        starlag::render::Camera camera;
        const glm::vec3 startPos(8.0f, 6.0f, 12.0f);
        camera.setPosition(startPos);
        camera.setPerspective(glm::radians(60.0f),
                              static_cast<float>(winW) / static_cast<float>(winH),
                              0.1f, 2000.0f);
        // Deriva yaw/pitch iniciais da direção (startPos → origem), para o modo
        // fly começar apontando o Sol. forward = -dir; yaw/pitch da convenção.
        {
            const glm::vec3 dir = glm::normalize(glm::vec3(0.0f) - startPos);
            const float pitch = std::asin(dir.y);
            const float yaw = std::atan2(-dir.x, -dir.z);  // inverte a fórmula de forward().
            camera.setYawPitch(yaw, pitch);
        }

        // Controlador de voo 6-DOF (T2.3): WASD move, mouse(dir.) olha, scroll
        // faz dolly, '['/']' (ou -/=) ajustam a velocidade.
        starlag::render::FlyCameraController fly;
        fly.setSpeed(8.0);  // ~8 parsecs/s: cobre a vizinhança solar com agilidade.

        std::printf("starlag v0.1.0 — campo de estrelas 3D.\n");
        std::printf("  Navegacao: WASD mover | E/Q (Space/Ctrl) subir/descer | botao DIR olhar\n");
        std::printf("             scroll = avancar/recuar | -/= ajustar velocidade\n");
        std::printf("  Selecao  : CLIQUE esquerdo p/ escolher origem; 2o clique p/ destino.\n");
        std::printf("             (3o clique reinicia a selecao.) Feche a janela p/ sair.\n");

        // --- Estado de seleção origem/destino (T2.4) ---
        // Índices na lista catalog.stars; -1 = nada selecionado ainda.
        long selOrigin = -1;
        long selDest = -1;
        // Marcadores (origem ciano, destino magenta) — instâncias StarInstance
        // grandes, reconstruídas quando a seleção muda.
        std::vector<starlag::render::StarInstance> markers;
        auto rebuildMarkers = [&]() {
            markers.clear();
            auto addMarker = [&](long idx, float r, float g, float b) {
                if (idx < 0) return;
                const starlag::data::Star& s = catalog.stars[static_cast<size_t>(idx)];
                starlag::render::StarInstance m;
                m.px = static_cast<float>(s.x);
                m.py = static_cast<float>(s.y);
                m.pz = static_cast<float>(s.z);
                m.cr = r; m.cg = g; m.cb = b;
                m.size = 22.0f;  // bem maior que as estrelas comuns.
                markers.push_back(m);
            };
            addMarker(selOrigin, 0.2f, 1.0f, 1.0f);  // ciano = origem.
            addMarker(selDest, 1.0f, 0.2f, 1.0f);    // magenta = destino.
        };

        // Contadores para medir FPS médio a cada ~1 segundo.
        auto fpsWindowStart = clock::now();
        int framesInWindow = 0;
        auto lastFrame = clock::now();  // para o dt da navegação.

        while (window.isOpen()) {
            window.pollEvents();

            // dt do frame (segundos) para movimento independente de frame rate.
            const auto frameNow = clock::now();
            const double dt = std::chrono::duration<double>(frameNow - lastFrame).count();
            lastFrame = frameNow;

            // Aplica a navegação 6-DOF (T2.3) a partir do input do frame.
            fly.update(camera, window.input(), dt);

            // Acompanha o aspect atual do framebuffer (cobre resize da janela).
            int fbW = 0, fbH = 0;
            window.framebufferSize(&fbW, &fbH);
            if (fbW > 0 && fbH > 0) {
                camera.setAspect(static_cast<float>(fbW) / static_cast<float>(fbH));
            }

            const glm::mat4 vp = camera.viewProjection();

            // --- Seleção por clique (T2.4): ray-cast → estrela mais próxima ---
            const starlag::render::InputState& in = window.input();
            if (in.clicked && catalog.ok && fbW > 0 && fbH > 0) {
                const glm::mat4 invVP = glm::inverse(vp);
                const starlag::render::Ray ray = starlag::render::screenPointToRay(
                    in.cursorX, in.cursorY, fbW, fbH, invVP, camera.position());
                // Limiar angular ~0.7° (generoso para clicar sem mira de pixel).
                const starlag::render::PickResult pick =
                    starlag::render::pickNearestStar(ray, catalog.stars,
                                                     glm::radians(0.7));
                if (pick.hit) {
                    const long idx = static_cast<long>(pick.index);
                    const starlag::data::Star& s = catalog.stars[pick.index];
                    if (selOrigin < 0) {
                        selOrigin = idx;
                        std::printf("\n>>> ORIGEM selecionada:\n%s",
                                    starlag::render::formatStarInfo(s).c_str());
                    } else if (selDest < 0 && idx != selOrigin) {
                        selDest = idx;
                        const starlag::data::Star& origin =
                            catalog.stars[static_cast<size_t>(selOrigin)];
                        std::printf("\n>>> DESTINO selecionado:\n%s",
                                    starlag::render::formatStarInfo(s, &origin).c_str());
                        // Bônus: roda a simulação relativística Sol-like origem→destino.
                        const double dPc = std::sqrt(
                            (s.x - origin.x) * (s.x - origin.x) +
                            (s.y - origin.y) * (s.y - origin.y) +
                            (s.z - origin.z) * (s.z - origin.z));
                        starlag::physics::TripRequest req;
                        req.distanceLy = dPc * starlag::physics::kParsec_ly;
                        req.mode = starlag::physics::PhysicsMode::Accelerated;
                        req.accelG = 1.0;
                        req.cruiseBeta = 0.9999999;
                        req.departureDate = starlag::physics::Date{2026, 6, 15, 0.0};
                        req.originName = starlag::render::displayName(origin);
                        req.destinationName = starlag::render::displayName(s);
                        const starlag::physics::SimulationResult r =
                            starlag::physics::runSimulation(req);
                        std::printf("  Viagem @1g: %s\n", r.summary.c_str());
                    } else {
                        // 3º clique: reinicia a seleção começando nova origem.
                        selOrigin = idx;
                        selDest = -1;
                        std::printf("\n>>> Nova ORIGEM (selecao reiniciada):\n%s",
                                    starlag::render::formatStarInfo(s).c_str());
                    }
                    rebuildMarkers();
                }
            }

            // Renderiza o campo de estrelas (+ grade + marcadores) pela câmera.
            window.renderStars(&vp[0][0],
                               starField.empty() ? nullptr
                                                 : &starField[0].px,
                               starField.size(), skyColor, /*drawGrid=*/true,
                               markers.empty() ? nullptr : &markers[0].px,
                               markers.size());

            // --- Medição de FPS ---
            ++framesInWindow;
            const auto now = clock::now();
            const std::chrono::duration<double> elapsed = now - fpsWindowStart;
            if (elapsed.count() >= 1.0) {
                const double fps = framesInWindow / elapsed.count();
                std::printf("FPS: %.1f\n", fps);
                framesInWindow = 0;
                fpsWindowStart = now;
            }
        }

        std::printf("Janela fechada. Ate logo.\n");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Erro fatal: %s\n", e.what());
        return 1;
    }
}
