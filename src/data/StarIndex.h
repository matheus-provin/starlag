// ============================================================================
//  StarIndex — índice de busca e seleção de estrelas (T1.3).
//
//  Constrói estruturas de lookup sobre o `std::vector<Star>` carregado pelo
//  parser (T1.2), oferecendo:
//    - lookup exato O(1) por id, HIP e HD (hash maps);
//    - busca textual por nome próprio (case-insensitive, prefixo > substring),
//      para o painel de busca da UI (T4.2);
//    - consultas espaciais (vizinha mais próxima, estrelas num raio), base para
//      o picking 3D (T2.4) e listas de "estrelas próximas".
//
//  O índice é NÃO-DONO dos dados: guarda um ponteiro para o vetor de Star (que
//  vive no ParseReport) e armazena apenas posições (índices) nas tabelas. Logo,
//  o vetor de estrelas deve sobreviver ao índice (não realocar/mover enquanto o
//  índice estiver em uso). As consultas devolvem `const Star*` para a estrela.
//
//  MVP: consultas espaciais são por varredura linear O(n). Com ~120k estrelas
//  isto é trivial para chamadas pontuais (seleção/hover). Se o perfil exigir
//  (ex.: muitas queries por frame), trocar por um k-d tree — anotado no .cpp.
// ============================================================================

#pragma once

#include "data/Star.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace starlag::data {

// Um resultado de busca textual: a estrela e o quão "forte" foi o casamento.
// `score` maior = casamento melhor (prefixo > substring); usado para ordenar.
struct SearchHit {
    const Star* star = nullptr;
    int score = 0;
};

class StarIndex {
public:
    StarIndex() = default;

    // Constrói o índice a partir de um vetor de estrelas já carregado. O vetor
    // referenciado deve permanecer vivo e estável (sem realocar) durante o uso.
    explicit StarIndex(const std::vector<Star>& stars);

    // (Re)constrói o índice apontando para `stars`.
    void build(const std::vector<Star>& stars);

    // Quantas estrelas o índice cobre.
    size_t size() const { return stars_ ? stars_->size() : 0; }
    bool empty() const { return size() == 0; }

    // --- Lookup exato (O(1)) ------------------------------------------------
    // Retornam nullptr se não houver estrela com aquele identificador.
    // (HIP/HD = 0 significam "ausente" no HYG e nunca são indexados.)
    const Star* byId(int64_t id) const;
    const Star* byHip(int64_t hip) const;
    const Star* byHd(int64_t hd) const;

    // --- Busca textual por nome próprio -------------------------------------
    // Case-insensitive. Casamento por prefixo pontua mais que por substring;
    // empate é desfeito por nome mais curto (mais "exato") e depois alfabético.
    // `limit` corta o número de resultados (0 = sem limite). Só considera
    // estrelas com nome próprio (campo `proper` não-vazio).
    std::vector<SearchHit> searchByName(const std::string& query,
                                        size_t limit = 0) const;

    // Atalho: a melhor correspondência textual (ou nullptr se nenhuma).
    const Star* bestByName(const std::string& query) const;

    // --- Consultas espaciais (posições em parsecs) --------------------------
    // Estrela mais próxima do ponto (x,y,z), opcionalmente ignorando uma estrela
    // (ex.: a própria origem). Retorna nullptr se o índice estiver vazio.
    const Star* nearestTo(double x, double y, double z,
                          const Star* exclude = nullptr) const;

    // Todas as estrelas a até `radiusPc` parsecs do ponto, ordenadas da mais
    // próxima para a mais distante. `limit` corta o resultado (0 = sem limite).
    std::vector<const Star*> withinRadius(double x, double y, double z,
                                          double radiusPc,
                                          size_t limit = 0) const;

private:
    const std::vector<Star>* stars_ = nullptr;

    // identificador → posição no vetor `*stars_`.
    std::unordered_map<int64_t, size_t> byId_;
    std::unordered_map<int64_t, size_t> byHip_;
    std::unordered_map<int64_t, size_t> byHd_;

    // nome próprio normalizado (lower) → posição. Para busca textual varremos
    // este vetor de pares (preserva o nome normalizado p/ não renormalizar).
    struct NamedEntry {
        std::string lowerName;  // nome próprio em minúsculas.
        size_t pos;             // posição no vetor `*stars_`.
    };
    std::vector<NamedEntry> named_;

    const Star* at(size_t pos) const { return &(*stars_)[pos]; }
};

}  // namespace starlag::data
