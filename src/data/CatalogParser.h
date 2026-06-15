// ============================================================================
//  CatalogParser — leitura do CSV do HYG para um vetor de Star (T1.2).
//
//  Parser CSV simples porém robusto para o formato do HYG: respeita aspas
//  duplas (campos podem ser "texto" ou texto cru), trata campos vazios, e
//  localiza colunas pelos nomes do cabeçalho (não por posição fixa) — assim
//  variações entre versões do HYG não quebram o parser silenciosamente.
//
//  Linhas malformadas são contadas e puladas (não abortam o carregamento);
//  o relatório de parse informa quantas estrelas vieram e quantas falharam.
// ============================================================================

#pragma once

#include "data/Star.h"

#include <string>
#include <vector>

namespace starlag::data {

// Relatório do carregamento do catálogo.
struct ParseReport {
    std::vector<Star> stars;     // estrelas válidas carregadas.
    size_t totalLines = 0;       // linhas de dados lidas (sem o cabeçalho).
    size_t skipped = 0;          // linhas puladas (malformadas / sem posição).
    bool ok = false;             // o arquivo foi aberto e teve cabeçalho válido?
    std::string message;         // descrição legível (UI/log).
};

// Carrega e parseia o CSV do HYG no caminho dado.
ParseReport parseCatalogFile(const std::string& path);

// Parseia o catálogo a partir de um conteúdo já em memória (útil p/ testes
// unitários com pequenos CSVs sintéticos, sem tocar o disco).
ParseReport parseCatalogString(const std::string& csvContent);

// Divide uma linha CSV em campos, respeitando aspas duplas. Exposta para teste.
std::vector<std::string> splitCsvLine(const std::string& line);

}  // namespace starlag::data
