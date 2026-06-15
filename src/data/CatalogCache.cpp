// ============================================================================
//  CatalogCache.cpp — implementação da localização do cache do catálogo (T1.1).
// ============================================================================

#include "data/CatalogCache.h"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace starlag::data {

std::string defaultCatalogPath() {
    // Relativo ao diretório de trabalho. Ao rodar a partir da raiz do projeto,
    // aponta para o CSV que o usuário colocou em data/.
    // TODO (app empacotado): usar ~/Library/Application Support/starlag/.
    return "data/hygdata_v42.csv";
}

CatalogCacheStatus locateCatalog(const std::string& path) {
    CatalogCacheStatus status;
    status.path = path;

    std::error_code ec;
    const fs::path p(path);

    // Existe e é um arquivo regular?
    if (!fs::exists(p, ec) || ec) {
        status.found = false;
        status.message = "Catalogo nao encontrado em '" + path +
                         "'. Baixe o HYG e coloque-o em data/.";
        return status;
    }
    if (!fs::is_regular_file(p, ec) || ec) {
        status.found = false;
        status.message = "'" + path + "' existe mas nao e um arquivo regular.";
        return status;
    }

    // Tamanho (sanidade: o HYG v4.2 tem dezenas de MB; um arquivo vazio indica
    // download/cópia corrompido).
    const std::uintmax_t size = fs::file_size(p, ec);
    if (ec) {
        status.found = false;
        status.message = "Nao foi possivel ler o tamanho de '" + path + "'.";
        return status;
    }
    if (size == 0) {
        status.found = false;
        status.sizeBytes = 0;
        status.message = "Catalogo '" + path + "' esta vazio (0 bytes).";
        return status;
    }

    status.found = true;
    status.sizeBytes = size;
    status.message = "Catalogo encontrado: '" + path + "' (" +
                     std::to_string(size / (1024 * 1024)) + " MB).";
    return status;
}

bool fetchRemote(const std::string& /*url*/, const std::string& /*destPath*/) {
    // Stub intencional (ver header). O download via libcurl entra quando o
    // ambiente permitir acesso de rede ao HYG; por ora o cache é provido
    // manualmente. Retornar false sinaliza "não baixei" para o chamador decidir.
    return false;
}

}  // namespace starlag::data
