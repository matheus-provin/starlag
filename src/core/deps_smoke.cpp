// ============================================================================
//  deps_smoke — "hello" de cada dependência de terceiros (T0.3).
//
//  Objetivo: provar que GLM, nlohmann/json, libcurl e Dear ImGui estão
//  corretamente integradas (compilam, linkam e executam o básico). É um
//  executável separado do app principal, roda HEADLESS (sem janela/GPU), e
//  retorna exit code 0 só se TODOS os checks passarem — assim vira um teste
//  automatizável (CTest) e validável pelo agente.
//
//  Nota: aqui NÃO fazemos request de rede (libcurl) nem render real (ImGui);
//  isso fica para T1.1 (download HYG) e T4.1 (backend ImGui+Metal). O foco é
//  só "a lib está linkada e funcional no nível básico".
// ============================================================================

#include <cmath>
#include <cstdio>
#include <string>

// --- GLM: álgebra linear (vetores/matrizes) ---
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// --- nlohmann/json: parse/serialize JSON ---
#include <nlohmann/json.hpp>

// --- libcurl: HTTP (aqui só init + versão) ---
#include <curl/curl.h>

// --- Dear ImGui: UI imediata (aqui só core, sem backend/render) ---
#include "imgui.h"

namespace {

// Helper de teste: imprime PASS/FAIL e acumula falhas.
int g_failures = 0;
void check(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// 1) GLM: soma de vetores e uma transformação de projeção simples.
void smokeGlm() {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    glm::vec3 c = a + b;  // (5, 7, 9)
    const bool vecOk = (c.x == 5.0f && c.y == 7.0f && c.z == 9.0f);

    // Comprimento de (3,4,0) deve ser 5 (triângulo 3-4-5).
    const float len = glm::length(glm::vec3(3.0f, 4.0f, 0.0f));
    const bool lenOk = std::fabs(len - 5.0f) < 1e-5f;

    // Uma matriz de projeção em perspectiva deve ser construível (usada na câmera).
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    const bool projOk = (proj[3][3] == 0.0f);  // perspectiva: w-row específica.

    check("GLM: soma de vetores", vecOk);
    check("GLM: length 3-4-5", lenOk);
    check("GLM: matriz de projecao", projOk);
}

// 2) nlohmann/json: serializar e re-parsear, conferindo round-trip.
void smokeJson() {
    nlohmann::json j;
    j["star"] = "Vega";
    j["dist_ly"] = 25.04;
    j["coords"] = {1.0, 2.0, 3.0};

    const std::string dumped = j.dump();
    nlohmann::json reparsed = nlohmann::json::parse(dumped);

    const bool nameOk = (reparsed["star"] == "Vega");
    const bool distOk = (std::fabs(reparsed["dist_ly"].get<double>() - 25.04) < 1e-9);
    const bool arrOk = (reparsed["coords"].size() == 3 && reparsed["coords"][1] == 2.0);

    check("JSON: round-trip string", nameOk);
    check("JSON: round-trip double", distOk);
    check("JSON: array", arrOk);
}

// 3) libcurl: inicializar a lib e ler a versão (sem tráfego de rede).
void smokeCurl() {
    const CURLcode initRc = curl_global_init(CURL_GLOBAL_DEFAULT);
    const bool initOk = (initRc == CURLE_OK);

    curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    const bool versionOk = (info != nullptr && info->version != nullptr);
    if (versionOk) {
        std::printf("    (libcurl %s)\n", info->version);
    }

    CURL* handle = curl_easy_init();
    const bool handleOk = (handle != nullptr);
    if (handle) curl_easy_cleanup(handle);

    curl_global_cleanup();

    check("libcurl: global init", initOk);
    check("libcurl: version info", versionOk);
    check("libcurl: easy handle", handleOk);
}

// 4) Dear ImGui: criar contexto e rodar um frame headless (sem backend/GPU).
//    Configuramos manualmente DisplaySize/DeltaTime e a fonte para que
//    NewFrame()/Render() funcionem sem nenhum render backend.
void smokeImGui() {
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    const bool ctxOk = (ctx != nullptr);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    // Constrói o atlas de fontes em memória (não envia para GPU).
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    const bool fontOk = (pixels != nullptr && w > 0 && h > 0);

    ImGui::NewFrame();
    ImGui::Begin("smoke");
    ImGui::Text("starlag deps OK");
    ImGui::End();
    ImGui::Render();  // gera draw data em CPU; sem backend não há desenho real.

    ImDrawData* drawData = ImGui::GetDrawData();
    const bool renderOk = (drawData != nullptr && drawData->Valid);

    ImGui::DestroyContext();

    check("ImGui: criar contexto", ctxOk);
    check("ImGui: atlas de fonte", fontOk);
    check("ImGui: frame headless", renderOk);
}

}  // namespace

int main() {
    std::printf("== starlag T0.3 — smoke test de dependencias ==\n");

    std::printf("[GLM]\n");      smokeGlm();
    std::printf("[JSON]\n");     smokeJson();
    std::printf("[libcurl]\n");  smokeCurl();
    std::printf("[ImGui]\n");    smokeImGui();

    if (g_failures == 0) {
        std::printf("\nTODAS as dependencias OK.\n");
        return 0;
    }
    std::printf("\n%d check(s) FALHARAM.\n", g_failures);
    return 1;
}
