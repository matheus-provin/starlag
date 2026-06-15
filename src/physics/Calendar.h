// ============================================================================
//  Calendar — datas no calendário gregoriano (proléptico) e aritmética de
//  durações em anos (T3.3).
//
//  Por que NÃO usar std::chrono diretamente: as viagens longas do simulador
//  geram datas de chegada a milhares (ou milhões) de anos no futuro. O suporte
//  de std::chrono a anos muito distantes é limitado/variável entre toolchains.
//  Aqui usamos os algoritmos de calendário de Howard Hinnant (baseados em "dias
//  desde 1970-01-01"), que são exatos e válidos para qualquer ano no calendário
//  gregoriano proléptico, com aritmética inteira de 64 bits.
//
//  Convenções:
//   - Gregoriano proléptico (a regra gregoriana de bissextos estendida ao
//     passado/futuro). Ano 0 existe (= 1 a.C. astronômico).
//   - Um "ano" de duração física = ano juliano = 365.25 dias (consistente com a
//     definição de ano-luz usada na física). addYears usa essa convenção.
// ============================================================================

#pragma once

#include <cstdint>
#include <string>

namespace starlag::physics {

// Data civil no calendário gregoriano proléptico.
struct Date {
    int64_t year = 1970;  // pode ser negativo (proléptico) ou muito grande.
    int month = 1;        // 1..12
    int day = 1;          // 1..31 (válido para o mês/ano)

    // Fração do dia [0,1): permite preservar a parte fracionária ao somar
    // durações não-inteiras em dias (ex.: 6.443 anos). 0 = meia-noite.
    double dayFraction = 0.0;
};

// Dias inteiros desde 1970-01-01 (epoch Unix), pelo algoritmo de Hinnant.
// Exato para qualquer ano no gregoriano proléptico. Ignora dayFraction.
int64_t daysFromCivil(int64_t year, int month, int day);

// Inverso de daysFromCivil: reconstrói (ano, mês, dia) a partir do nº de dias.
Date civilFromDays(int64_t days);

// Soma uma duração em ANOS JULIANOS (365.25 dias) a uma data, preservando a
// fração de dia. Retorna a nova data. `years` pode ser fracionário e grande.
Date addYears(const Date& start, double years);

// Formata como "YYYY-MM-DD" (ano com sinal se negativo). Não inclui hora.
std::string formatDate(const Date& d);

}  // namespace starlag::physics
