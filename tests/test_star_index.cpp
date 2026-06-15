// ============================================================================
//  test_star_index — testes do índice de busca/seleção (T1.3).
//
//  Sintéticos (lookups, busca textual, espacial, bordas) + validação contra o
//  catálogo HYG real (caminho injetado via STARLAG_TEST_CATALOG no CMake).
// ============================================================================

#include "data/CatalogParser.h"
#include "data/StarIndex.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace starlag::data;

namespace {

int g_failures = 0;

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

#ifndef STARLAG_TEST_CATALOG
#define STARLAG_TEST_CATALOG ""
#endif

// Monta um pequeno conjunto de estrelas sintéticas para os testes de unidade.
std::vector<Star> makeSynthetic() {
    std::vector<Star> v;
    auto add = [&](int64_t id, int64_t hip, int64_t hd, const std::string& proper,
                   double x, double y, double z) {
        Star s;
        s.id = id; s.hip = hip; s.hd = hd; s.proper = proper;
        s.x = x; s.y = y; s.z = z;
        v.push_back(std::move(s));
    };
    //   id  hip   hd    proper        x    y    z
    add(0,   0,    0,    "Sol",        0,   0,   0);
    add(1,   100,  200,  "Alpha",      1,   0,   0);
    add(2,   101,  0,    "Alphard",    0,   2,   0);   // prefixo "alph" também.
    add(3,   0,    300,  "Beta",       0,   0,   3);
    add(4,   102,  301,  "",           5,   5,   5);   // sem nome próprio.
    return v;
}

// --- Lookup exato por id/HIP/HD ---------------------------------------------
void testExactLookup() {
    std::printf("[lookup exato]\n");
    std::vector<Star> stars = makeSynthetic();
    StarIndex idx(stars);

    expectTrue("size = 5", idx.size() == 5);
    expectTrue("byId(0) = Sol", idx.byId(0) && idx.byId(0)->proper == "Sol");
    expectTrue("byId(3) = Beta", idx.byId(3) && idx.byId(3)->proper == "Beta");
    expectTrue("byId(99) = null", idx.byId(99) == nullptr);

    expectTrue("byHip(100) = Alpha", idx.byHip(100) && idx.byHip(100)->id == 1);
    expectTrue("byHip(0) = null (ausente)", idx.byHip(0) == nullptr);
    expectTrue("byHd(300) = Beta", idx.byHd(300) && idx.byHd(300)->id == 3);
    expectTrue("byHd(0) = null (ausente)", idx.byHd(0) == nullptr);
}

// --- Busca textual: case, prefixo vs substring, limite ----------------------
void testTextSearch() {
    std::printf("[busca textual]\n");
    std::vector<Star> stars = makeSynthetic();
    StarIndex idx(stars);

    // Case-insensitive e igualdade exata pontua mais que prefixo.
    auto sol = idx.searchByName("sol");
    expectTrue("'sol' acha Sol", sol.size() == 1 && sol[0].star->proper == "Sol");

    // "alph" casa Alpha (prefixo) e Alphard (prefixo); Alpha vem antes por ser
    // o nome mais curto no desempate.
    auto alph = idx.searchByName("alph");
    expectTrue("'alph' acha 2", alph.size() == 2);
    expectTrue("'alph' Alpha primeiro (mais curto)",
               !alph.empty() && alph[0].star->proper == "Alpha");

    // Substring no meio pontua menos que prefixo: "lph" casa ambos por substring.
    auto lph = idx.searchByName("LPH");
    expectTrue("'LPH' (maiusc) acha 2 por substring", lph.size() == 2);

    // Igualdade exata "Alpha" deve ranquear Alpha acima de Alphard.
    auto exact = idx.searchByName("Alpha");
    expectTrue("'Alpha' exato ranqueia Alpha 1o",
               !exact.empty() && exact[0].star->proper == "Alpha" &&
               exact[0].score == 3);

    // Sem casamento e query vazia → vazio.
    expectTrue("'xyz' nao casa", idx.searchByName("xyz").empty());
    expectTrue("query vazia → vazio", idx.searchByName("").empty());

    // Limite corta o resultado.
    expectTrue("limite=1 corta", idx.searchByName("alph", 1).size() == 1);

    // bestByName devolve o topo.
    const Star* best = idx.bestByName("beta");
    expectTrue("bestByName('beta') = Beta", best && best->proper == "Beta");
    expectTrue("bestByName('zzz') = null", idx.bestByName("zzz") == nullptr);
}

// --- Consultas espaciais ----------------------------------------------------
void testSpatial() {
    std::printf("[consultas espaciais]\n");
    std::vector<Star> stars = makeSynthetic();
    StarIndex idx(stars);

    // Mais próxima da origem (excluindo o próprio Sol em (0,0,0)) é Alpha em (1,0,0).
    const Star* sol = idx.byId(0);
    const Star* near = idx.nearestTo(0, 0, 0, sol);
    expectTrue("vizinha do Sol = Alpha", near && near->proper == "Alpha");

    // Sem exclusão, a mais próxima de (0,0,0) é o próprio Sol.
    const Star* self = idx.nearestTo(0, 0, 0);
    expectTrue("nearest sem exclude = Sol", self && self->proper == "Sol");

    // Perto de (0,0,3) a mais próxima é Beta.
    const Star* beta = idx.nearestTo(0, 0, 3);
    expectTrue("vizinha de (0,0,3) = Beta", beta && beta->proper == "Beta");

    // Raio 1.5 a partir da origem: Sol (0) e Alpha (1) entram; Alphard (2) não.
    auto within = idx.withinRadius(0, 0, 0, 1.5);
    expectTrue("raio 1.5 pega 2", within.size() == 2);
    expectTrue("raio 1.5 ordenado: Sol antes de Alpha",
               within.size() == 2 && within[0]->proper == "Sol" &&
               within[1]->proper == "Alpha");

    // Raio grande com limite corta mantendo ordem por distância.
    auto limited = idx.withinRadius(0, 0, 0, 100.0, 3);
    expectTrue("raio grande limit=3", limited.size() == 3);
    expectTrue("limit mantem mais proximas (Sol 1o)",
               !limited.empty() && limited[0]->proper == "Sol");

    // Raio negativo → vazio (borda).
    expectTrue("raio negativo → vazio", idx.withinRadius(0, 0, 0, -1.0).empty());
}

// --- Bordas: índice vazio / não construído ----------------------------------
void testEmpty() {
    std::printf("[bordas vazias]\n");
    StarIndex idx;
    expectTrue("vazio: size 0", idx.size() == 0 && idx.empty());
    expectTrue("vazio: byId null", idx.byId(0) == nullptr);
    expectTrue("vazio: nearest null", idx.nearestTo(0, 0, 0) == nullptr);
    expectTrue("vazio: search vazio", idx.searchByName("x").empty());

    std::vector<Star> none;
    StarIndex idx2(none);
    expectTrue("0 estrelas: empty", idx2.empty());
    expectTrue("0 estrelas: nearest null", idx2.nearestTo(0, 0, 0) == nullptr);
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
    expectTrue("catalogo ok", rep.ok);
    if (!rep.ok) return;

    StarIndex idx(rep.stars);
    expectTrue("indice cobre ~120k", idx.size() > 119000 && idx.size() < 120000);

    // Busca por nome conhecido.
    const Star* vega = idx.bestByName("Vega");
    expectTrue("Vega por nome", vega && vega->proper == "Vega");
    if (vega) {
        // Lookup cruzado: a mesma estrela pelo id deve bater.
        const Star* vegaById = idx.byId(vega->id);
        expectTrue("Vega id round-trip", vegaById == vega);
        // HIP da Vega = 91262 (Alpha Lyrae) — confirma o índice por HIP.
        expectTrue("Vega HIP 91262", idx.byHip(91262) == vega);
    }

    const Star* sirius = idx.bestByName("sirius");  // case-insensitive.
    expectTrue("Sirius (minusculo) por nome", sirius && sirius->proper == "Sirius");

    // Busca parcial: "cent" deve trazer várias (Alpha/Proxima Centauri etc.).
    auto cent = idx.searchByName("cent");
    expectTrue("'cent' traz resultados", !cent.empty());

    // Vizinha mais próxima do Sol: deve ser uma das Centauri (~1.3 pc).
    const Star* sol = idx.bestByName("Sol");
    expectTrue("Sol presente", sol != nullptr);
    if (sol) {
        const Star* neighbor = idx.nearestTo(sol->x, sol->y, sol->z, sol);
        expectTrue("vizinha do Sol existe", neighbor != nullptr);
        if (neighbor) {
            const double dx = neighbor->x - sol->x;
            const double dy = neighbor->y - sol->y;
            const double dz = neighbor->z - sol->z;
            const double d = std::sqrt(dx * dx + dy * dy + dz * dz);
            std::printf("  -> vizinha do Sol: '%s' a %.3f pc (id %lld)\n",
                        neighbor->proper.empty() ? "(sem nome)" : neighbor->proper.c_str(),
                        d, static_cast<long long>(neighbor->id));
            // A estrela mais próxima do Sol é Proxima Centauri, ~1.30 pc.
            expectTrue("vizinha do Sol < 1.5 pc", d < 1.5);
        }

        // Estrelas a 2 pc do Sol: deve haver um punhado (vizinhança solar).
        auto near = idx.withinRadius(sol->x, sol->y, sol->z, 2.0);
        std::printf("  -> %zu estrelas a <= 2 pc do Sol\n", near.size());
        expectTrue("ha estrelas a <=2pc do Sol", near.size() >= 1);
        // A primeira (mais próxima, excluindo o Sol que está em d=0) é o Sol.
        expectTrue("withinRadius inclui o proprio Sol primeiro",
                   !near.empty() && near[0]->id == sol->id);
    }
}

}  // namespace

int main() {
    std::printf("== starlag T1.3 — testes do indice de busca/selecao ==\n");

    testExactLookup();
    testTextSearch();
    testSpatial();
    testEmpty();
    testRealCatalog();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
