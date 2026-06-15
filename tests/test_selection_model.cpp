// ============================================================================
//  test_selection_model — testes da máquina de seleção origem/destino (T4.2).
// ============================================================================

#include "render/SelectionModel.h"

#include <cstdio>

using namespace starlag::render;

namespace {

int g_failures = 0;

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// --- Ciclo origem → destino → reinicia --------------------------------------
void testCycle() {
    std::printf("[ciclo de selecao]\n");
    SelectionModel m;
    expectTrue("inicio: sem origem", !m.hasOrigin() && !m.hasDestination());

    // 1ª seleção → origem.
    bool changed = m.selectStar(10);
    expectTrue("1a selecao muda", changed);
    expectTrue("origem = 10", m.origin() == 10 && !m.hasDestination());

    // 2ª seleção (diferente) → destino.
    m.selectStar(20);
    expectTrue("destino = 20", m.origin() == 10 && m.destination() == 20);
    expectTrue("completa", m.complete());

    // 3ª seleção → reinicia (nova origem, sem destino).
    m.selectStar(30);
    expectTrue("reinicia: origem = 30", m.origin() == 30 && !m.hasDestination());
}

// --- Casos de borda ---------------------------------------------------------
void testEdges() {
    std::printf("[bordas]\n");
    SelectionModel m;

    // Índice inválido é ignorado (não muda nada).
    expectTrue("indice negativo ignorado", !m.selectStar(-1) && !m.hasOrigin());

    // Selecionar a própria origem na fase de destino reinicia (não vira destino).
    m.selectStar(5);
    m.selectStar(5);  // mesma estrela: não pode ser origem E destino.
    expectTrue("clique na origem nao vira destino", m.origin() == 5 && !m.hasDestination());

    // clear() zera tudo.
    m.selectStar(7);  // agora 5→origem já existia; este vira destino.
    expectTrue("destino setado p/ clear", m.complete());
    m.clear();
    expectTrue("clear zera", !m.hasOrigin() && !m.hasDestination());
}

// --- Atribuição direta (botões da UI) ---------------------------------------
void testDirectSet() {
    std::printf("[atribuicao direta]\n");
    SelectionModel m;
    m.setOrigin(100);
    m.setDestination(200);
    expectTrue("setOrigin/setDestination", m.origin() == 100 && m.destination() == 200);
    expectTrue("completa por atribuicao", m.complete());

    // Sobrescrever origem direto não mexe no destino.
    m.setOrigin(101);
    expectTrue("setOrigin preserva destino", m.origin() == 101 && m.destination() == 200);
}

}  // namespace

int main() {
    std::printf("== starlag T4.2 — testes do modelo de selecao ==\n");

    testCycle();
    testEdges();
    testDirectSet();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
