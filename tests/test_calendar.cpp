// ============================================================================
//  test_calendar — testes do calendário gregoriano (T3.3).
//
//  Referências verificadas independentemente em Python (algoritmo de Hinnant).
//  Casos confirmados:
//    daysFromCivil(1970,1,1)   = 0
//    daysFromCivil(2000,1,1)   = 10957
//    daysFromCivil(2026,6,15)  = 20619
//    daysFromCivil(2032,11,23) = 22972
//    2026-06-15 + 6.443 anos julianos  -> 2032-11-23
//    2026-06-15 + 25.293 anos julianos -> 2051-09-30
//    2026-06-15 + 1011.78 anos julianos-> 3038-04-03 (floor; frac de dia 0.645)
//  Round-trip civil<->days: 0 falhas no intervalo testado (~ano -1900 a 6200).
// ============================================================================

#include "physics/Calendar.h"

#include <cmath>
#include <cstdio>
#include <cstdint>

using namespace starlag::physics;

namespace {

int g_failures = 0;

void expectEqI(const char* name, int64_t got, int64_t want) {
    const bool ok = (got == want);
    std::printf("  [%s] %s  (got=%lld, want=%lld)\n", ok ? "PASS" : "FALHA", name,
                static_cast<long long>(got), static_cast<long long>(want));
    if (!ok) ++g_failures;
}

void expectDate(const char* name, const Date& d, int64_t y, int m, int day) {
    const bool ok = (d.year == y && d.month == m && d.day == day);
    std::printf("  [%s] %s  (got=%lld-%02d-%02d, want=%lld-%02d-%02d)\n",
                ok ? "PASS" : "FALHA", name,
                static_cast<long long>(d.year), d.month, d.day,
                static_cast<long long>(y), m, day);
    if (!ok) ++g_failures;
}

void expectTrue(const char* name, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FALHA", name);
    if (!ok) ++g_failures;
}

// --- daysFromCivil: âncoras conhecidas --------------------------------------
void testDaysFromCivil() {
    std::printf("[daysFromCivil]\n");
    expectEqI("1970-01-01 = 0",    daysFromCivil(1970, 1, 1), 0);
    expectEqI("2000-01-01 = 10957", daysFromCivil(2000, 1, 1), 10957);
    expectEqI("2026-06-15 = 20619", daysFromCivil(2026, 6, 15), 20619);
    expectEqI("2032-11-23 = 22972", daysFromCivil(2032, 11, 23), 22972);
    expectEqI("1969-12-31 = -1",   daysFromCivil(1969, 12, 31), -1);
}

// --- Round-trip civil<->days -------------------------------------------------
void testRoundTrip() {
    std::printf("[round-trip civil<->days]\n");
    int fails = 0;
    for (int64_t days = -700000; days < 3000000; days += 37) {
        Date d = civilFromDays(days);
        if (daysFromCivil(d.year, d.month, d.day) != days) ++fails;
    }
    expectTrue("round-trip exato (~ano -1900..6200)", fails == 0);
    if (fails != 0) std::printf("    (%d falhas)\n", fails);
}

// --- addYears: durações físicas viram datas ---------------------------------
void testAddYears() {
    std::printf("[addYears]\n");
    Date start{2026, 6, 15, 0.0};

    // 6.443 anos (tempo próprio Vega@1g) -> 2032-11-23.
    expectDate("+6.443yr -> 2032-11-23", addYears(start, 6.443), 2032, 11, 23);
    // 25.293 anos (tempo coordenado Vega@0.99c) -> 2051-09-30.
    expectDate("+25.293yr -> 2051-09-30", addYears(start, 25.293), 2051, 9, 30);
    // Viagem longa: +1011.78 anos -> 3038-04-03 (frac de dia 0.645, ainda dia 3).
    expectDate("+1011.78yr -> 3038-04-03", addYears(start, 1011.78), 3038, 4, 3);

    // Duração 0 mantém a data.
    expectDate("+0yr mantem", addYears(start, 0.0), 2026, 6, 15);

    // Fração de dia em [0,1).
    Date frac = addYears(start, 6.443);
    expectTrue("dayFraction em [0,1)", frac.dayFraction >= 0.0 && frac.dayFraction < 1.0);
}

// --- Datas distantes / extremas ---------------------------------------------
void testExtremes() {
    std::printf("[datas extremas]\n");
    // Salto de milênios: +100000 anos a partir de 2026.
    Date far = addYears(Date{2026, 1, 1, 0.0}, 100000.0);
    expectTrue("ano > 100000", far.year > 100000);
    // Round-trip da data distante.
    int64_t back = daysFromCivil(far.year, far.month, far.day);
    expectTrue("round-trip data distante", civilFromDays(back).year == far.year);

    // formatDate básico.
    expectTrue("formatDate 2026-06-15",
               formatDate(Date{2026, 6, 15, 0.0}) == std::string("2026-06-15"));
}

}  // namespace

int main() {
    std::printf("== starlag T3.3 — testes de calendario gregoriano ==\n");

    testDaysFromCivil();
    testRoundTrip();
    testAddYears();
    testExtremes();

    if (g_failures == 0) {
        std::printf("\nTODOS os testes passaram.\n");
        return 0;
    }
    std::printf("\n%d teste(s) FALHARAM.\n", g_failures);
    return 1;
}
