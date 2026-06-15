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
#include "data/StarIndex.h"
#include "physics/Calendar.h"
#include "physics/Constants.h"
#include "physics/Simulation.h"
#include "render/Camera.h"
#include "render/FlyCameraController.h"
#include "render/ImGuiLayer.h"
#include "render/MetalWindow.h"
#include "render/Picker.h"
#include "render/SelectionModel.h"
#include "render/StarField.h"
#include "render/StarInfo.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <string>
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
        starlag::data::StarIndex starIndex;  // busca textual/lookup (T1.3 → T4.2).
        if (catalog.ok) {
            starField = starlag::render::buildStarField(catalog.stars);
            starIndex.build(catalog.stars);
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

        // Inicializa a camada de UI ImGui (T4.1) com os handles do MetalWindow.
        starlag::render::ImGuiLayer ui;
        ui.init(window.glfwWindowHandle(), window.metalDeviceHandle());

        std::printf("starlag v0.1.0 — campo de estrelas 3D + UI.\n");
        std::printf("  Navegacao: WASD mover | E/Q (Space/Ctrl) subir/descer | botao DIR olhar\n");
        std::printf("             scroll = avancar/recuar | -/= ajustar velocidade\n");
        std::printf("  Selecao  : CLIQUE esquerdo (ou busque no painel) p/ origem; 2o p/ destino.\n");
        std::printf("  Painel   : ajuste velocidade/aceleracao/data e clique 'Simular viagem'.\n");

        // --- Estado de seleção origem/destino (T4.2) ---
        // SelectionModel unifica a seleção por clique (T2.4) e por busca (T4.2):
        // ambos chamam selectStar() / setOrigin() / setDestination().
        starlag::render::SelectionModel selection;
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
            addMarker(selection.origin(), 0.2f, 1.0f, 1.0f);       // ciano = origem.
            addMarker(selection.destination(), 1.0f, 0.2f, 1.0f);  // magenta = destino.
        };

        // Buffer do campo de busca textual (T4.2) e um modo de atribuição: o
        // próximo resultado clicado vira origem, destino, ou segue a máquina de
        // estados (auto). Persistem entre frames (a UI é redesenhada por frame).
        char searchBuf[64] = {0};
        enum class AssignMode { Auto, Origin, Destination };
        AssignMode assignMode = AssignMode::Auto;

        // --- Parâmetros de viagem controlados pelo painel (T4.1) ---
        // Velocidade em % de c (até 99.99999); aceleração em g; modo de física;
        // data de partida (campos inteiros). Alimentam runSimulation ao "Simular".
        struct UiState {
            float cruisePercentC = 99.0f;   // % de c (0 < v < 100).
            float accelG = 1.0f;            // aceleração própria em g.
            bool accelerated = true;        // true=acelerado, false=v constante.
            int year = 2026, month = 6, day = 15;
            bool hasResult = false;
            std::string resultSummary;      // resumo em linguagem natural.
            double properYr = 0, coordYr = 0, peakGamma = 0, contractedLy = 0;
            std::string arrivalShip, arrivalOrigin;
        } uiState;

        // Roda a simulação com os parâmetros atuais do painel para o par
        // origem→destino selecionado. Preenche uiState com o resultado.
        auto runTrip = [&]() {
            if (!selection.complete()) return;
            const auto& o = catalog.stars[static_cast<size_t>(selection.origin())];
            const auto& d = catalog.stars[static_cast<size_t>(selection.destination())];
            const double dPc = std::sqrt(
                (d.x - o.x) * (d.x - o.x) + (d.y - o.y) * (d.y - o.y) +
                (d.z - o.z) * (d.z - o.z));
            starlag::physics::TripRequest req;
            req.distanceLy = dPc * starlag::physics::kParsec_ly;
            req.mode = uiState.accelerated
                           ? starlag::physics::PhysicsMode::Accelerated
                           : starlag::physics::PhysicsMode::ConstantVelocity;
            req.accelG = uiState.accelG;
            // % de c → beta, limitado a < 1 pela física (clampBeta).
            req.cruiseBeta = uiState.cruisePercentC / 100.0;
            req.departureDate = starlag::physics::Date{uiState.year, uiState.month,
                                                       uiState.day, 0.0};
            req.originName = starlag::render::displayName(o);
            req.destinationName = starlag::render::displayName(d);
            const auto r = starlag::physics::runSimulation(req);
            uiState.hasResult = true;
            uiState.resultSummary = r.summary;
            uiState.properYr = r.properTimeYr;
            uiState.coordYr = r.coordinateTimeYr;
            uiState.peakGamma = r.peakGamma;
            uiState.contractedLy = r.contractedDistanceLy;
            uiState.arrivalShip = starlag::physics::formatDate(r.arrivalDateShip);
            uiState.arrivalOrigin = starlag::physics::formatDate(r.arrivalDateOrigin);
        };

        // Callback de UI: desenha os painéis. Roda dentro do render pass (T4.1).
        window.setUiCallback([&](void* pass, void* cmd, void* enc) {
            ui.beginFrame(pass);

            // Helper: aplica uma seleção (por índice) seguindo o modo escolhido.
            auto applySelection = [&](long idx) {
                switch (assignMode) {
                    case AssignMode::Origin:      selection.setOrigin(idx); break;
                    case AssignMode::Destination: selection.setDestination(idx); break;
                    case AssignMode::Auto:        selection.selectStar(idx); break;
                }
                uiState.hasResult = false;  // seleção mudou: resultado antigo obsoleto.
                rebuildMarkers();
            };

            ImGui::Begin("Viagem interestelar");
            // Origem/destino selecionados.
            ImGui::Text("Origem : %s", selection.hasOrigin()
                ? starlag::render::displayName(
                      catalog.stars[static_cast<size_t>(selection.origin())]).c_str()
                : "(clique ou busque)");
            ImGui::Text("Destino: %s", selection.hasDestination()
                ? starlag::render::displayName(
                      catalog.stars[static_cast<size_t>(selection.destination())]).c_str()
                : "(2o clique ou busque)");
            if (ImGui::Button("Limpar selecao")) {
                selection.clear();
                uiState.hasResult = false;
                rebuildMarkers();
            }
            ImGui::Separator();

            // Parâmetros.
            ImGui::Checkbox("Acelerado (1g-style)", &uiState.accelerated);
            if (uiState.accelerated) {
                ImGui::SliderFloat("Aceleracao (g)", &uiState.accelG, 0.1f, 10.0f, "%.2f g");
            }
            ImGui::SliderFloat("Velocidade (% c)", &uiState.cruisePercentC,
                               1.0f, 99.99999f, "%.5f %%");
            ImGui::InputInt("Ano", &uiState.year);
            ImGui::InputInt("Mes", &uiState.month);
            ImGui::InputInt("Dia", &uiState.day);

            const bool canSim = selection.complete();
            if (!canSim) ImGui::BeginDisabled();
            if (ImGui::Button("Simular viagem")) runTrip();
            if (!canSim) ImGui::EndDisabled();

            ImGui::End();

            // --- Painel de busca de estrelas por nome (T4.2) ---
            ImGui::Begin("Buscar estrela");
            ImGui::TextUnformatted("Nome proprio (ex.: Vega, Sirius):");
            ImGui::InputText("##busca", searchBuf, sizeof(searchBuf));

            // Para onde vai o próximo resultado escolhido.
            int modeInt = static_cast<int>(assignMode);
            ImGui::TextUnformatted("Atribuir a:");
            ImGui::SameLine(); ImGui::RadioButton("Auto", &modeInt, 0);
            ImGui::SameLine(); ImGui::RadioButton("Origem", &modeInt, 1);
            ImGui::SameLine(); ImGui::RadioButton("Destino", &modeInt, 2);
            assignMode = static_cast<AssignMode>(modeInt);

            ImGui::Separator();
            if (searchBuf[0] != '\0' && !starIndex.empty()) {
                const auto hits = starIndex.searchByName(searchBuf, 15);
                if (hits.empty()) {
                    ImGui::TextDisabled("(nenhum resultado)");
                }
                for (const auto& hit : hits) {
                    const starlag::data::Star& s = *hit.star;
                    // Rótulo: nome + distância + tipo espectral. ID no '##' p/ unicidade.
                    char label[128];
                    std::snprintf(label, sizeof(label), "%s  —  %.1f ly  %s##%lld",
                                  starlag::render::displayName(s).c_str(),
                                  s.distLy, s.spect.c_str(),
                                  static_cast<long long>(s.id));
                    if (ImGui::Selectable(label)) {
                        // Acha o índice da estrela no vetor do catálogo (id → posição).
                        // O StarIndex referencia o mesmo vetor; o ponteiro aritmético
                        // dá o índice diretamente.
                        const long idx = static_cast<long>(hit.star - &catalog.stars[0]);
                        applySelection(idx);
                    }
                }
            } else if (starIndex.empty()) {
                ImGui::TextDisabled("(catalogo nao carregado)");
            } else {
                ImGui::TextDisabled("(digite para buscar)");
            }
            ImGui::End();

            // Painel de resultado (básico — completo na T4.3).
            if (uiState.hasResult) {
                ImGui::Begin("Resultado");
                ImGui::Text("Tempo a bordo : %.3f anos  (chegada %s)",
                            uiState.properYr, uiState.arrivalShip.c_str());
                ImGui::Text("Tempo origem  : %.3f anos  (chegada %s)",
                            uiState.coordYr, uiState.arrivalOrigin.c_str());
                ImGui::Text("Fator Lorentz : %.3f (pico)", uiState.peakGamma);
                ImGui::Text("Dist. contraida: %.3f anos-luz", uiState.contractedLy);
                ImGui::Separator();
                ImGui::TextWrapped("%s", uiState.resultSummary.c_str());
                ImGui::End();
            }

            ui.endFrame(cmd, enc);
        });

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

            // Gating de input pela UI: quando o cursor está sobre um painel
            // ImGui, ele captura o mouse/teclado — então não navegamos nem
            // fazemos picking (evita "voar" ou selecionar ao mexer no painel).
            const bool uiMouse = ui.wantCaptureMouse();
            const bool uiKeyboard = ui.wantCaptureKeyboard();

            // Aplica a navegação 6-DOF (T2.3), exceto quando a UI usa o teclado.
            if (!uiKeyboard) {
                fly.update(camera, window.input(), dt);
            }

            // Acompanha o aspect atual do framebuffer (cobre resize da janela).
            int fbW = 0, fbH = 0;
            window.framebufferSize(&fbW, &fbH);
            if (fbW > 0 && fbH > 0) {
                camera.setAspect(static_cast<float>(fbW) / static_cast<float>(fbH));
            }

            const glm::mat4 vp = camera.viewProjection();

            // --- Seleção por clique (T2.4): ray-cast → estrela mais próxima ---
            const starlag::render::InputState& in = window.input();
            if (in.clicked && !uiMouse && catalog.ok && fbW > 0 && fbH > 0) {
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
                    // Detecta o papel resultante comparando o estado antes/depois.
                    const bool wasComplete = selection.complete();
                    const bool hadOrigin = selection.hasOrigin();
                    selection.selectStar(idx);
                    if (!hadOrigin || wasComplete) {
                        std::printf("\n>>> ORIGEM selecionada:\n%s",
                                    starlag::render::formatStarInfo(s).c_str());
                    } else if (selection.destination() == idx) {
                        const starlag::data::Star& origin =
                            catalog.stars[static_cast<size_t>(selection.origin())];
                        std::printf("\n>>> DESTINO selecionado:\n%s",
                                    starlag::render::formatStarInfo(s, &origin).c_str());
                    }
                    uiState.hasResult = false;  // seleção mudou: resultado obsoleto.
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
