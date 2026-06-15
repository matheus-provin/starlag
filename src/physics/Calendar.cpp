// ============================================================================
//  Calendar.cpp — implementação dos algoritmos de calendário gregoriano (T3.3).
//
//  Os algoritmos daysFromCivil/civilFromDays são de Howard Hinnant
//  ("chrono-Compatible Low-Level Date Algorithms"), domínio público. São exatos
//  para todo o calendário gregoriano proléptico usando apenas aritmética
//  inteira. Mantemos os nomes/estrutura próximos do original para rastreabilidade.
// ============================================================================

#include "physics/Calendar.h"
#include "physics/Constants.h"

#include <cmath>    // std::floor
#include <cstdio>   // std::snprintf

namespace starlag::physics {

int64_t daysFromCivil(int64_t y, int month, int day) {
    // Trata jan/fev como meses 13/14 do ano anterior (simplifica os bissextos).
    y -= (month <= 2) ? 1 : 0;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;                       // [0, 399]
    const int64_t doy =
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;  // [0, 365]
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;     // [0, 146096]
    return era * 146097 + doe - 719468;  // 719468 = dias de 0000-03-01 a 1970-01-01.
}

Date civilFromDays(int64_t z) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int64_t doe = z - era * 146097;                          // [0, 146096]
    const int64_t yoe =
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;     // [0, 399]
    const int64_t y = yoe + era * 400;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);   // [0, 365]
    const int64_t mp = (5 * doy + 2) / 153;                        // [0, 11]
    const int day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);  // [1, 31]
    const int month = static_cast<int>(mp + (mp < 10 ? 3 : -9));   // [1, 12]

    Date d;
    d.year = y + (month <= 2 ? 1 : 0);
    d.month = month;
    d.day = day;
    d.dayFraction = 0.0;
    return d;
}

Date addYears(const Date& start, double years) {
    // Posição absoluta da data de partida em dias (com fração), em escala double.
    const int64_t startDays = daysFromCivil(start.year, start.month, start.day);
    const double absDays =
        static_cast<double>(startDays) + start.dayFraction + years * kJulianYear_days;

    // Separa parte inteira (dia civil) da fração (hora do dia).
    const double whole = std::floor(absDays);
    Date result = civilFromDays(static_cast<int64_t>(whole));
    result.dayFraction = absDays - whole;  // em [0,1)
    return result;
}

std::string formatDate(const Date& d) {
    // Ano pode ser negativo ou ter muitos dígitos; %lld cobre int64.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%lld-%02d-%02d",
                  static_cast<long long>(d.year), d.month, d.day);
    return std::string(buf);
}

}  // namespace starlag::physics
