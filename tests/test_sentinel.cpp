// ============================================================================
//  test_sentinel — teste mínimo que valida a infraestrutura de testes (CTest).
//
//  Não testa lógica de domínio ainda; serve apenas para garantir que o alvo de
//  testes compila, roda e reporta sucesso/falha via exit code desde o Marco 0.
//  Os testes reais de física entram no Marco 3 (T3.1+).
// ============================================================================

#include <cassert>

int main() {
    // Sanidade básica: se isto falhar, o ambiente de build está quebrado.
    assert(1 + 1 == 2);
    return 0;  // exit code 0 = teste passou (CTest interpreta assim).
}
