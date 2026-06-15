// ============================================================================
//  test_catalog_cache — testes da localização do cache do catálogo (T1.1).
//
//  O caminho do CSV real é injetado pelo CMake (STARLAG_TEST_CATALOG) como
//  caminho absoluto, já que o CTest roda de um diretório de build diferente da
//  raiz do projeto. Testamos: arquivo presente (real), arquivo ausente, e o
//  stub de download.
// ============================================================================

#include "data/CatalogCache.h"

#include <cstdio>
#include <string>

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

void testRealCatalog() {
    std::printf("[catalogo real]\n");
    const std::string path = STARLAG_TEST_CATALOG;
    if (path.empty()) {
        std::printf("  (STARLAG_TEST_CATALOG nao definido; pulando)\n");
        return;
    }

    CatalogCacheStatus st = locateCatalog(path);
    std::printf("  -> %s\n", st.message.c_str());
    expectTrue("encontrado", st.found);
    // HYG v4.2 descompactado tem dezenas de MB; sanidade de tamanho.
    expectTrue("tamanho > 1 MB", st.sizeBytes > 1024 * 1024);
    expectTrue("path ecoado", st.path == path);
}

void testMissing() {
    std::printf("[catalogo ausente]\n");
    CatalogCacheStatus st = locateCatalog("/caminho/que/nao/existe/hyg.csv");
    expectTrue("nao encontrado", !st.found);
    expectTrue("tamanho 0", st.sizeBytes == 0);
    expectTrue("mensagem nao-vazia", !st.message.empty());
}

void testFetchStub() {
    std::printf("[download stub]\n");
    // Stub atual sempre retorna false (sem rede neste ambiente).
    bool ok = fetchRemote("https://example.com/hyg.csv", "/tmp/nope.csv");
    expectTrue("fetchRemote retorna false (stub)", ok == false);
}

void testDefaultPath() {
    std::printf("[caminho default]\n");
    expectTrue("default aponta p/ data/hygdata_v42.csv",
               defaultCatalogPath() == std::string("data/hygdata_v42.csv"));
}

}  // namespace

int main() {
    std::printf("== starlag T1.1 — testes do cache do catalogo ==\n");

    testRealCatalog();
    testMissing();
    testFetchStub();
    testDefaultPath();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
