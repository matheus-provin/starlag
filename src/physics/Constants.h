// ============================================================================
//  Constants — constantes físicas e de conversão usadas pela simulação.
//
//  TODAS em `double` (precisão dupla), conforme requisito do projeto. Valores
//  de referência do SI e da IAU. Unidades indicadas no nome de cada constante.
//
//  Convenção de unidades "naturais" do app: distâncias em anos-luz (ly) e
//  tempos em anos julianos (yr). Nessas unidades, a velocidade da luz vale
//  exatamente 1 ly/yr — o que simplifica e estabiliza os cálculos de viagem.
// ============================================================================

#pragma once

namespace starlag::physics {

// Velocidade da luz no vácuo (exata, por definição do SI).
inline constexpr double kSpeedOfLight_m_s = 299'792'458.0;

// Ano juliano = 365,25 dias (padrão IAU para "ano-luz").
inline constexpr double kJulianYear_days = 365.25;
inline constexpr double kJulianYear_s = kJulianYear_days * 86'400.0;  // 31'557'600 s

// Ano-luz em metros (c · 1 ano juliano).
inline constexpr double kLightYear_m = kSpeedOfLight_m_s * kJulianYear_s;

// 1 parsec em anos-luz (IAU 2015). Catálogo HYG dá distâncias em parsecs.
inline constexpr double kParsec_ly = 3.2615637771;

// Aceleração da gravidade padrão (m/s²), referência para "1 g" de conforto.
inline constexpr double kStandardGravity_m_s2 = 9.80665;

// 1 g convertido para as unidades naturais do app (anos-luz / ano-juliano²).
//   a[ly/yr²] = a[m/s²] · ano_juliano² / ano-luz_em_metros
// Resultado ≈ 1.0323 ly/yr² → c/a ≈ 0.9687 yr (o famoso "~1 ano até perto de c").
inline constexpr double kOneG_ly_yr2 =
    kStandardGravity_m_s2 * (kJulianYear_s * kJulianYear_s) / kLightYear_m;

// Converte uma aceleração dada em múltiplos de g para ly/yr².
inline constexpr double gToLyPerYr2(double g_multiples) {
    return g_multiples * kOneG_ly_yr2;
}

// Limite rígido de velocidade permitido como entrada: 99.99999% de c.
// Nunca atingir ou ultrapassar c (γ → ∞). β = v/c.
inline constexpr double kMaxBeta = 0.9999999;

}  // namespace starlag::physics
