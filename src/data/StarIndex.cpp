// ============================================================================
//  StarIndex.cpp — implementação do índice de busca/seleção (T1.3).
// ============================================================================

#include "data/StarIndex.h"

#include <algorithm>
#include <cctype>

namespace starlag::data {

namespace {

// Normaliza texto para busca: minúsculas (ASCII) e sem espaços nas pontas.
// Os nomes próprios do HYG são ASCII, então um lower simples basta no MVP.
std::string normalize(const std::string& s) {
    size_t begin = 0, end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;

    std::string out;
    out.reserve(end - begin);
    for (size_t i = begin; i < end; ++i) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
    }
    return out;
}

// Distância euclidiana ao quadrado entre dois pontos (evita a raiz quando só
// comparamos magnitudes relativas).
double dist2(double ax, double ay, double az, double bx, double by, double bz) {
    const double dx = ax - bx, dy = ay - by, dz = az - bz;
    return dx * dx + dy * dy + dz * dz;
}

// Pontuação de casamento textual. Sem casamento → 0.
//  3 = igualdade exata, 2 = prefixo, 1 = substring.
int matchScore(const std::string& lowerName, const std::string& lowerQuery) {
    if (lowerQuery.empty()) return 0;
    if (lowerName == lowerQuery) return 3;
    const size_t at = lowerName.find(lowerQuery);
    if (at == 0) return 2;                 // prefixo.
    if (at != std::string::npos) return 1; // substring no meio/fim.
    return 0;                              // não casa.
}

}  // namespace

StarIndex::StarIndex(const std::vector<Star>& stars) { build(stars); }

void StarIndex::build(const std::vector<Star>& stars) {
    stars_ = &stars;
    byId_.clear();
    byHip_.clear();
    byHd_.clear();
    named_.clear();

    byId_.reserve(stars.size());
    for (size_t pos = 0; pos < stars.size(); ++pos) {
        const Star& s = stars[pos];

        // id é único no HYG; em caso de duplicata, o primeiro vence.
        byId_.emplace(s.id, pos);

        // HIP/HD == 0 significa "ausente" — não indexar (evita colidir todos os
        // sem-catálogo no mesmo balde 0).
        if (s.hip != 0) byHip_.emplace(s.hip, pos);
        if (s.hd != 0) byHd_.emplace(s.hd, pos);

        if (s.hasProperName()) {
            named_.push_back({normalize(s.proper), pos});
        }
    }
}

// --- Lookup exato -----------------------------------------------------------

const Star* StarIndex::byId(int64_t id) const {
    if (!stars_) return nullptr;
    auto it = byId_.find(id);
    return (it == byId_.end()) ? nullptr : at(it->second);
}

const Star* StarIndex::byHip(int64_t hip) const {
    if (!stars_ || hip == 0) return nullptr;
    auto it = byHip_.find(hip);
    return (it == byHip_.end()) ? nullptr : at(it->second);
}

const Star* StarIndex::byHd(int64_t hd) const {
    if (!stars_ || hd == 0) return nullptr;
    auto it = byHd_.find(hd);
    return (it == byHd_.end()) ? nullptr : at(it->second);
}

// --- Busca textual ----------------------------------------------------------

std::vector<SearchHit> StarIndex::searchByName(const std::string& query,
                                               size_t limit) const {
    std::vector<SearchHit> hits;
    if (!stars_) return hits;

    const std::string q = normalize(query);
    if (q.empty()) return hits;

    for (const auto& entry : named_) {
        const int score = matchScore(entry.lowerName, q);
        if (score > 0) {
            hits.push_back({at(entry.pos), score});
        }
    }

    // Ordena por: maior score; depois nome mais curto (casamento mais "exato");
    // depois alfabético — produz uma ordem estável e previsível para a UI.
    std::sort(hits.begin(), hits.end(), [](const SearchHit& a, const SearchHit& b) {
        if (a.score != b.score) return a.score > b.score;
        const std::string& na = a.star->proper;
        const std::string& nb = b.star->proper;
        if (na.size() != nb.size()) return na.size() < nb.size();
        return na < nb;
    });

    if (limit != 0 && hits.size() > limit) hits.resize(limit);
    return hits;
}

const Star* StarIndex::bestByName(const std::string& query) const {
    const std::vector<SearchHit> hits = searchByName(query, 1);
    return hits.empty() ? nullptr : hits.front().star;
}

// --- Consultas espaciais ----------------------------------------------------
//
//  Varredura linear O(n). Para seleção pontual (hover/clique) isto roda em
//  fração de milissegundo mesmo com ~120k estrelas. Se virar gargalo (muitas
//  queries por frame), substituir por um k-d tree de 3 dimensões.

const Star* StarIndex::nearestTo(double x, double y, double z,
                                 const Star* exclude) const {
    if (!stars_ || stars_->empty()) return nullptr;

    const Star* best = nullptr;
    double bestD2 = 0.0;
    for (const Star& s : *stars_) {
        if (&s == exclude) continue;
        const double d2 = dist2(x, y, z, s.x, s.y, s.z);
        if (best == nullptr || d2 < bestD2) {
            best = &s;
            bestD2 = d2;
        }
    }
    return best;
}

std::vector<const Star*> StarIndex::withinRadius(double x, double y, double z,
                                                 double radiusPc,
                                                 size_t limit) const {
    std::vector<const Star*> out;
    if (!stars_ || radiusPc < 0.0) return out;

    const double r2 = radiusPc * radiusPc;
    // Coleta candidatos com a distância² para ordenar sem recomputar.
    std::vector<std::pair<double, const Star*>> cand;
    for (const Star& s : *stars_) {
        const double d2 = dist2(x, y, z, s.x, s.y, s.z);
        if (d2 <= r2) cand.emplace_back(d2, &s);
    }

    std::sort(cand.begin(), cand.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    const size_t n = (limit == 0) ? cand.size() : std::min(limit, cand.size());
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) out.push_back(cand[i].second);
    return out;
}

}  // namespace starlag::data
