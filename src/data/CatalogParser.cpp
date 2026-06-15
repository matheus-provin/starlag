// ============================================================================
//  CatalogParser.cpp — implementação do parser do catálogo HYG (T1.2).
// ============================================================================

#include "data/CatalogParser.h"
#include "physics/Constants.h"

#include <cstdlib>      // std::strtod, std::strtoll
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace starlag::data {

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                // Aspas duplas escapadas ("") dentro de campo entre aspas.
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;  // fecha o campo entre aspas.
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;       // abre campo entre aspas.
            } else if (c == ',') {
                fields.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
    }
    fields.push_back(cur);  // último campo.
    return fields;
}

namespace {

// Converte string para double; vazio → 0.0 e marca `present=false`.
double toDouble(const std::string& s, bool* present = nullptr) {
    if (s.empty()) {
        if (present) *present = false;
        return 0.0;
    }
    if (present) *present = true;
    return std::strtod(s.c_str(), nullptr);
}

// Converte string para inteiro 64; vazio → 0.
int64_t toInt64(const std::string& s) {
    if (s.empty()) return 0;
    return static_cast<int64_t>(std::strtoll(s.c_str(), nullptr, 10));
}

// Constrói o mapa nome-da-coluna → índice a partir do cabeçalho.
std::unordered_map<std::string, int> headerIndex(const std::vector<std::string>& header) {
    std::unordered_map<std::string, int> idx;
    for (int i = 0; i < static_cast<int>(header.size()); ++i) {
        idx[header[i]] = i;
    }
    return idx;
}

// Acesso seguro a um campo por índice; retorna "" se fora de faixa.
const std::string& fieldAt(const std::vector<std::string>& f, int i) {
    static const std::string kEmpty;
    if (i < 0 || i >= static_cast<int>(f.size())) return kEmpty;
    return f[i];
}

// Faz o parse de todas as linhas de um stream já posicionado após abrir.
ParseReport parseStream(std::istream& in) {
    ParseReport rep;

    std::string headerLine;
    if (!std::getline(in, headerLine)) {
        rep.ok = false;
        rep.message = "Arquivo vazio ou ilegivel (sem cabecalho).";
        return rep;
    }
    // Remove um possível '\r' final (CSV salvo no Windows).
    if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

    const std::vector<std::string> header = splitCsvLine(headerLine);
    const std::unordered_map<std::string, int> col = headerIndex(header);

    // Localiza as colunas que nos interessam pelo nome (robusto a reordenação).
    auto colOf = [&](const char* name) -> int {
        auto it = col.find(name);
        return (it == col.end()) ? -1 : it->second;
    };
    const int iId = colOf("id");
    const int iHip = colOf("hip");
    const int iHd = colOf("hd");
    const int iGl = colOf("gl");
    const int iProper = colOf("proper");
    const int iDist = colOf("dist");
    const int iMag = colOf("mag");
    const int iAbsmag = colOf("absmag");
    const int iSpect = colOf("spect");
    const int iCi = colOf("ci");
    const int iX = colOf("x");
    const int iY = colOf("y");
    const int iZ = colOf("z");
    // Designação/catálogo (T2.4): opcionais — ausência não invalida o load.
    const int iCon = colOf("con");
    const int iBayer = colOf("bayer");
    const int iFlam = colOf("flam");
    const int iLum = colOf("lum");

    // Cabeçalho precisa ter ao menos posição e distância para ser útil.
    if (iX < 0 || iY < 0 || iZ < 0 || iDist < 0 || iId < 0) {
        rep.ok = false;
        rep.message = "Cabecalho HYG invalido: faltam colunas essenciais (id/x/y/z/dist).";
        return rep;
    }

    rep.ok = true;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        ++rep.totalLines;

        const std::vector<std::string> f = splitCsvLine(line);
        // Sanidade mínima: precisa ter campos suficientes para os índices usados.
        if (static_cast<int>(f.size()) <= iZ) {
            ++rep.skipped;
            continue;
        }

        Star s;
        s.id = toInt64(fieldAt(f, iId));
        s.hip = toInt64(fieldAt(f, iHip));
        s.hd = toInt64(fieldAt(f, iHd));
        s.gl = fieldAt(f, iGl);
        s.proper = fieldAt(f, iProper);
        s.x = toDouble(fieldAt(f, iX));
        s.y = toDouble(fieldAt(f, iY));
        s.z = toDouble(fieldAt(f, iZ));
        s.distPc = toDouble(fieldAt(f, iDist));
        s.distLy = s.distPc * physics::kParsec_ly;  // parsec → ano-luz.
        s.mag = toDouble(fieldAt(f, iMag));
        s.absmag = toDouble(fieldAt(f, iAbsmag));
        s.spect = fieldAt(f, iSpect);
        s.ci = toDouble(fieldAt(f, iCi), &s.hasCi);

        // Designação/catálogo (T2.4): só lê se a coluna existir no cabeçalho.
        if (iCon >= 0) s.con = fieldAt(f, iCon);
        if (iBayer >= 0) s.bayer = fieldAt(f, iBayer);
        if (iFlam >= 0) s.flam = fieldAt(f, iFlam);
        if (iLum >= 0) s.lum = toDouble(fieldAt(f, iLum), &s.hasLum);

        rep.stars.push_back(std::move(s));
    }

    std::ostringstream msg;
    msg << "Catalogo parseado: " << rep.stars.size() << " estrelas ("
        << rep.skipped << " linhas puladas).";
    rep.message = msg.str();
    return rep;
}

}  // namespace

ParseReport parseCatalogFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        ParseReport rep;
        rep.ok = false;
        rep.message = "Nao foi possivel abrir '" + path + "'.";
        return rep;
    }
    return parseStream(in);
}

ParseReport parseCatalogString(const std::string& csvContent) {
    std::istringstream in(csvContent);
    return parseStream(in);
}

}  // namespace starlag::data
