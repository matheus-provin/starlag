// ============================================================================
//  CatalogCache — localização e acesso ao cache local do catálogo HYG (T1.1).
//
//  Estratégia "offline-first": o app usa um CSV do HYG cacheado em disco. Esta
//  camada resolve ONDE o arquivo está e se ele está utilizável, sem ainda
//  parsear o conteúdo (isso é a T1.2). O download remoto via libcurl está
//  previsto (ver fetchRemote, ainda um stub) mas, neste escopo, assumimos que
//  o CSV já foi obtido e colocado em `data/`.
//
//  Decisão: o caminho default do cache é relativo ao diretório de trabalho
//  (`data/hygdata_v42.csv`), o que funciona ao rodar a partir da raiz do projeto.
//  Em produção (app empacotado) isso migraria para
//  ~/Library/Application Support/starlag/ — anotado como TODO.
// ============================================================================

#pragma once

#include <cstdint>
#include <string>

namespace starlag::data {

// Resultado da resolução do cache do catálogo.
struct CatalogCacheStatus {
    bool found = false;          // o arquivo existe e é legível?
    std::string path;            // caminho resolvido (mesmo se não encontrado).
    std::uintmax_t sizeBytes = 0;  // tamanho do arquivo (0 se ausente).
    std::string message;         // descrição legível (para UI/log).
};

// Caminho default do cache local do catálogo HYG.
std::string defaultCatalogPath();

// Verifica se o cache local existe e é legível, preenchendo o status.
// Não lê/parseia o conteúdo (apenas metadados do arquivo).
CatalogCacheStatus locateCatalog(const std::string& path = defaultCatalogPath());

// TODO (escopo futuro da T1.1): baixar o CSV do HYG de uma URL remota via
// libcurl e salvar em `destPath`. Hoje é um stub que sempre retorna false,
// porque o ambiente de desenvolvimento do agente não tem acesso de rede ao
// repositório do HYG; o usuário baixa o arquivo manualmente para `data/`.
// URL de referência (Astronexus HYG v4.2):
//   https://raw.githubusercontent.com/astronexus/HYG-Database/main/hyg/v42/hygdata_v42.csv
bool fetchRemote(const std::string& url, const std::string& destPath);

}  // namespace starlag::data
