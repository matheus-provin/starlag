// ============================================================================
//  test_catalog_parser — testes do parser CSV + struct Star (T1.2).
//
//  Combina testes sintéticos (CSV em memória, para bordas do parser) com o
//  catálogo HYG real (caminho injetado via STARLAG_TEST_CATALOG no CMake) para
//  validar contagem e estrelas conhecidas (Sol, Vega, Sirius).
// ============================================================================

#include "data/CatalogParser.h"
#include "physics/Constants.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace starlag::data;

namespace {

int g_failures = 0;

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

void expectNear(const char* name, double got, double want, double tol = 1e-4) {
    const double diff = std::fabs(got - want);
    const double scale = std::fmax(1.0, std::fabs(want));
    const bool ok = diff <= tol * scale;
    std::printf("  [%s] %s  (got=%.6g, want=%.6g)\n", ok ? "PASS" : "FALHA", name, got, want);
    if (!ok) ++g_failures;
}

#ifndef STARLAG_TEST_CATALOG
#define STARLAG_TEST_CATALOG ""
#endif

// Acha uma estrela pelo nome próprio (linear; só para teste).
const Star* findByProper(const ParseReport& rep, const std::string& name) {
    for (const auto& s : rep.stars) {
        if (s.proper == name) return &s;
    }
    return nullptr;
}

// --- splitCsvLine: bordas do CSV --------------------------------------------
void testSplit() {
    std::printf("[splitCsvLine]\n");
    auto a = splitCsvLine("0,,,\"\",Sol,1.5");
    expectTrue("6 campos", a.size() == 6);
    expectTrue("campo vazio sem aspas", a[1].empty());
    expectTrue("campo aspas vazio", a[3].empty());
    expectTrue("texto cru", a[4] == "Sol");

    auto b = splitCsvLine("\"a,b\",c");  // vírgula DENTRO de aspas vira 1 campo.
    expectTrue("virgula em aspas = 1 campo", b.size() == 2 && b[0] == "a,b");
}

// --- Parse sintético: campos e conversão pc->ly -----------------------------
void testSynthetic() {
    std::printf("[parse sintetico]\n");
    const std::string csv =
        "id,hip,hd,gl,proper,dist,mag,absmag,spect,ci,x,y,z\n"
        "0,,,,Sol,0.0,-26.7,4.85,G2V,0.656,0,0,0\n"
        "42,1234,5678,\"Gl 1\",TesteStar,10.0,5.0,3.0,K0V,0.8,1,2,2\n";
    ParseReport rep = parseCatalogString(csv);

    expectTrue("ok", rep.ok);
    expectTrue("2 estrelas", rep.stars.size() == 2);
    expectTrue("0 puladas", rep.skipped == 0);

    const Star& sol = rep.stars[0];
    expectTrue("Sol id=0", sol.id == 0);
    expectTrue("Sol proper", sol.proper == "Sol");
    expectTrue("Sol posicao zero", sol.x == 0 && sol.y == 0 && sol.z == 0);

    const Star& t = rep.stars[1];
    expectTrue("id=42", t.id == 42);
    expectTrue("hip=1234", t.hip == 1234);
    expectTrue("gl com aspas", t.gl == "Gl 1");
    expectTrue("spect", t.spect == "K0V");
    expectNear("dist 10pc -> ly", t.distLy, 10.0 * starlag::physics::kParsec_ly);
    expectTrue("hasCi true", t.hasCi);
}

// --- Linha malformada é pulada, não aborta ----------------------------------
void testSkipMalformed() {
    std::printf("[linha malformada]\n");
    const std::string csv =
        "id,hip,hd,gl,proper,dist,mag,absmag,spect,ci,x,y,z\n"
        "1,,,,A,1,0,0,G,0.5,1,2,3\n"
        "MALFORMADA_SEM_CAMPOS\n"          // poucos campos -> pulada.
        "2,,,,B,2,0,0,K,0.6,4,5,6\n";
    ParseReport rep = parseCatalogString(csv);
    expectTrue("2 validas", rep.stars.size() == 2);
    expectTrue("1 pulada", rep.skipped == 1);
    expectTrue("total 3 linhas", rep.totalLines == 3);
}

// --- Catálogo HYG real ------------------------------------------------------
void testRealCatalog() {
    std::printf("[catalogo HYG real]\n");
    const std::string path = STARLAG_TEST_CATALOG;
    if (path.empty()) {
        std::printf("  (STARLAG_TEST_CATALOG nao definido; pulando)\n");
        return;
    }
    ParseReport rep = parseCatalogFile(path);
    std::printf("  -> %s\n", rep.message.c_str());

    expectTrue("ok", rep.ok);
    // HYG v4.2: 119626 estrelas (linhas de dados).
    expectTrue("~119626 estrelas", rep.stars.size() > 119000 && rep.stars.size() < 120000);
    expectTrue("0 puladas", rep.skipped == 0);

    // Sol: primeira estrela, id 0, na origem.
    const Star* sol = findByProper(rep, "Sol");
    expectTrue("Sol presente", sol != nullptr);
    if (sol) {
        expectTrue("Sol id=0", sol->id == 0);
        // No HYG o Sol tem x=0.000005 (epsilon do dataset), não zero exato:
        // verificamos que está essencialmente na origem (< 1e-3 pc).
        const double r = std::sqrt(sol->x * sol->x + sol->y * sol->y + sol->z * sol->z);
        expectTrue("Sol ~na origem (<1e-3 pc)", r < 1e-3);
        expectTrue("Sol spect G2V", sol->spect == "G2V");
    }

    // Vega: ~7.6787 pc ≈ 25.04 ly (bate com os testes de física!).
    const Star* vega = findByProper(rep, "Vega");
    expectTrue("Vega presente", vega != nullptr);
    if (vega) {
        expectNear("Vega dist ~7.6787 pc", vega->distPc, 7.6787, 1e-3);
        expectNear("Vega dist ~25.04 ly", vega->distLy, 25.04, 0.05);
    }

    // Sirius: ~2.6371 pc.
    const Star* sirius = findByProper(rep, "Sirius");
    expectTrue("Sirius presente", sirius != nullptr);
    if (sirius) {
        expectNear("Sirius dist ~2.6371 pc", sirius->distPc, 2.6371, 1e-3);
    }
}

}  // namespace

int main() {
    std::printf("== starlag T1.2 — testes do parser de catalogo ==\n");

    testSplit();
    testSynthetic();
    testSkipMalformed();
    testRealCatalog();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
