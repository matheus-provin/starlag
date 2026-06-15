# REQUIREMENTS — Simulador 3D de Viagens Interestelares Relativísticas

> **Codinome do projeto:** `starlag` (star + lag temporal)
> **Versão do documento:** 1.0 — 2026-06-15
> **Status:** Especificação aprovada, pronta para planejamento

---

## 1. Visão Geral

`starlag` é uma aplicação desktop em **C++**, compilada via **Xcode** para **macOS**, que
simula viagens interestelares entre estrelas reais e mostra, de forma visual e quantitativa,
os **efeitos da Relatividade Restrita e Geral** sobre a passagem do tempo.

O usuário navega livremente por um mapa galáctico 3D povoado com estrelas reais, seleciona
uma **estrela de origem** e uma **estrela de destino**, ajusta parâmetros de viagem
(velocidade de cruzeiro como fração de `c`, aceleração própria, etc.) e dispara a simulação.
Ao final, o sistema responde à pergunta central do projeto:

> _"Ao chegar ao destino, quanto tempo passou para a tripulação da nave, e quanto tempo
> passou para um observador que permaneceu na estrela de origem?"_

A simulação ancora todas as datas no **calendário gregoriano**, permitindo ao usuário ver,
por exemplo, que partindo em 2026 a uma dada velocidade, a tripulação chega a Vega
"envelhecida" X anos, enquanto na Terra se passaram Y anos (e qual data gregoriana isso
representa em cada referencial).

### 1.1 Propósito

Projeto de **aprendizado pessoal**: aprofundamento prático em **C++ moderno**, na API
gráfica **Metal** da Apple, e em **física relativística** aplicada. As prioridades são:
código claro e bem documentado, evolução incremental (MVP → recursos avançados), e
correção científica dos cálculos. Não é um produto comercial; é uma plataforma de estudo
com qualidade de demonstração.

### 1.2 Caso de Uso Canônico

1. Usuário abre o app e vê a vizinhança solar renderizada em 3D.
2. Voa livremente pela galáxia (câmera livre) e clica no **Sol** como origem.
3. Clica em **Vega** como destino (≈ 25 anos-luz).
4. Define velocidade de cruzeiro = `99.9% c` e aceleração = `1 g`.
5. Define data de partida = `15/06/2026` (gregoriano).
6. Dispara a simulação e assiste à nave percorrer o trajeto em 3D, com efeitos visuais
   relativísticos (aberração da luz, Doppler).
7. Ao chegar, lê o painel de resultados:
   - **Tempo próprio (tripulação):** ex. ~7,1 anos → chegada em ~2033 no relógio de bordo.
   - **Tempo coordenado (observador na origem):** ex. ~25,1 anos → ano 2051 na Terra.
   - **Fator de Lorentz (γ)**, distância contraída percebida, velocidade máxima atingida.

---

## 2. Stack Tecnológica (Decidida)

| Camada | Escolha | Justificativa |
|---|---|---|
| **Linguagem** | C++17/20 | Requisito do projeto; compilado no Xcode. |
| **Build/IDE** | Xcode (projeto `.xcodeproj`), CMake opcional para organização | Compilação nativa macOS. |
| **Render 3D** | **Metal** (API nativa Apple) | Performance máxima no Mac; aprendizado da API moderna da Apple. |
| **UI / Controles** | **Dear ImGui** (backend Metal + integração com a janela) | UI imediata em C++, ideal para ferramentas/simulações; iteração rápida. |
| **Janela / Input** | **GLFW** ou **SDL2** (criação de janela + eventos, com camada Metal) | Abstrai janela/teclado/mouse; GLFW tem backend ImGui pronto. _A definir na fase de setup._ |
| **Dados estelares** | **HYG Database** (CSV) — modelo **remoto + enriquecimento** | Catálogo consolidado (~120k estrelas) com coordenadas XYZ já calculadas. |
| **Consumo de API** | Download remoto do HYG (cache local) + APIs sob demanda (SIMBAD / Wikipedia) | Atende ao requisito de "consumir APIs" sem sacrificar performance. |
| **Parsing/HTTP** | `libcurl` (HTTP) + parser CSV próprio/`fast-cpp-csv-parser`; `nlohmann/json` para respostas de API | Bibliotecas maduras e leves. |
| **Matemática** | **GLM** (vetores/matrizes) ou matemática própria | Álgebra linear para câmera, transformações e física. |

> **Observação sobre Metal + UI:** Metal não fornece widgets. A UI fica a cargo do Dear
> ImGui, renderizado em um pass Metal sobre a cena 3D.

---

## 3. Modelo de Dados Estelares

### 3.1 Fonte primária — HYG Database
- **Origem:** catálogo HYG (Hipparcos + Yale Bright Star + Gliese), distribuído como CSV
  público (ex.: repositório Astronexus no GitHub).
- **Estratégia "remoto + enriquecimento":**
  1. No **primeiro uso**, o app baixa o CSV de uma URL remota e o cacheia em disco
     (`~/Library/Application Support/starlag/` ou diretório local do projeto).
  2. Em execuções seguintes, lê do cache (offline-friendly).
  3. Ao **selecionar** uma estrela, o app pode chamar **APIs externas** (SIMBAD,
     Wikipedia REST) para enriquecer com nome comum, tipo espectral detalhado e descrição.

### 3.2 Campos relevantes por estrela
- Identificadores: `id`, `hip`, `hd`, `hr`, `gl`, nome próprio (`proper`).
- Posição 3D (parsecs, sistema equatorial cartesiano): `x`, `y`, `z`.
- Distância: `dist` (parsecs) → convertida para anos-luz.
- Fotometria: magnitude aparente (`mag`), magnitude absoluta (`absmag`).
- Tipo espectral (`spect`) e índice de cor (`ci`) → usados para **cor** de renderização.
- Movimento (opcional/futuro): movimento próprio, velocidade radial.

### 3.3 Conversões e unidades
- 1 parsec = 3,2616 anos-luz; 1 ano-luz = 9,4607×10¹⁵ m.
- Cor da estrela derivada do índice B−V (`ci`) ou do tipo espectral (sequência OBAFGKM).
- Tamanho/brilho do ponto na tela derivado de `absmag` + distância.

---

## 4. Modelo Físico (Profundidade: "Completo")

A simulação implementa as três camadas de fidelidade, de forma incremental:

### 4.1 Dilatação temporal (núcleo)
- **Fator de Lorentz:** γ = 1 / √(1 − v²/c²).
- **Velocidade máxima permitida:** `99.99999% c` (limite de input; nunca ≥ c).
- **Tempo próprio vs. coordenado:** para trecho a velocidade constante,
  Δτ = Δt / γ, onde Δt é o tempo medido na origem.
- **Saída:** datas gregorianas em ambos os referenciais (origem e nave).

### 4.2 Aceleração realista (perfil de voo)
- Perfil **acelera-coast-desacelera** com **aceleração própria constante** (ex.: 1 g
  para conforto/realismo de naves de fusão/antimatéria hipotéticas).
- **Fórmulas hiperbólicas** do movimento uniformemente acelerado relativístico:
  - Distância: d = (c²/a)·(cosh(a·τ/c) − 1).
  - Tempo coordenado: t = (c/a)·sinh(a·τ/c).
  - Velocidade: v(τ) = c·tanh(a·τ/c).
- Integração do **tempo próprio total** somando as fases de aceleração, cruzeiro
  (se a velocidade-alvo for atingida antes do ponto médio) e desaceleração.
- Tratamento de viagens curtas (nunca atingem v de cruzeiro → perfil triangular).

### 4.3 Efeitos visuais relativísticos (render)
- **Aberração relativística da luz:** estrelas "deslocam-se" para a frente do observador
  conforme v aumenta (concentração no ponto de fuga).
- **Efeito Doppler relativístico:** blueshift à frente, redshift atrás — alteração de
  cor das estrelas conforme a velocidade.
- **Headlight / beaming:** aumento de brilho aparente das estrelas à frente.
- **Contração do espaço à frente** (visualização do encurtamento percebido da rota).
- Implementação preferencialmente em **shaders Metal** (vertex/fragment) para performance.

> **Premissa científica:** modelo de **espaço plano (Minkowski)**, sem expansão cósmica
> nem campos gravitacionais fortes ao longo da rota (Relatividade Restrita domina).
> Efeitos de Relatividade Geral entram apenas como nota conceitual (não há trajetória
> próxima a buracos negros no MVP).

---

## 5. Parâmetros de Entrada do Usuário

| Parâmetro | Tipo | Faixa / Padrão | Observação |
|---|---|---|---|
| Estrela de origem | seleção 3D | qualquer estrela do catálogo | clique no mapa. |
| Estrela de destino | seleção 3D | qualquer estrela do catálogo | clique no mapa. |
| Velocidade de cruzeiro | float (% de c) | 0 < v ≤ 99.99999% c | limite rígido < c. |
| Aceleração própria | float (g) | ex. 0.1 g – 10 g (padrão 1 g) | usada no perfil de voo. |
| Data de partida (gregoriana) | data | padrão = data atual | base para datas de chegada. |
| Modo de física | enum | constante / acelerada | acelerada = padrão "Completo". |
| Efeitos visuais | toggle | on/off | aberração, Doppler. |

---

## 6. Saídas / Resultados

Ao concluir a simulação, exibir em painel ImGui:
- **Tempo próprio total** (anos/dias para a tripulação) + **data gregoriana de chegada
  no relógio de bordo**.
- **Tempo coordenado total** (anos/dias para o observador na origem) + **data gregoriana
  correspondente na origem**.
- **Diferença / "dívida temporal"** entre os dois referenciais.
- **Fator de Lorentz** máximo atingido.
- **Distância** real (anos-luz) e **distância contraída** percebida pela tripulação.
- **Velocidade máxima** atingida (se perfil acelerado).
- Resumo textual em linguagem natural (ex.: _"Você chegou a Vega. Para você, passaram-se
  7,1 anos; na Terra, 25,1 anos — você 'viajou' 18 anos ao futuro terrestre."_).

---

## 7. Experiência / Interação (Escopo: "Exploração livre 3D")

- **Câmera livre 6-DOF** (orbit + fly): WASD/setas + mouse para navegar pela galáxia.
- **Seleção de estrelas** por clique (picking 3D) com destaque/hover e tooltip de info.
- **Renderização de estrelas** como pontos/billboards coloridos por tipo espectral, com
  brilho proporcional à magnitude.
- **Linha de rota** entre origem e destino; **nave/marcador** animado percorrendo o trajeto
  durante a simulação, com timeline controlável (play/pause/velocidade de reprodução).
- **HUD/ImGui:** painéis de parâmetros, resultados, lista/busca de estrelas por nome.
- **Escalas visuais** ajustáveis (a galáxia é vasta; permitir zoom logarítmico).

---

## 8. Requisitos Não-Funcionais

- **Plataforma:** macOS (Apple Silicon e Intel), compilação Xcode.
- **Performance:** 60 FPS com dezenas de milhares de estrelas visíveis (instancing/billboards).
- **Offline-first:** após o primeiro download do catálogo, funciona sem internet
  (APIs de enriquecimento são opcionais e degradam graciosamente se offline).
- **Código:** C++ moderno, modular, comentado; separação clara entre física, dados,
  render e UI. Adequado a leitura e estudo.
- **Precisão numérica:** usar `double` nos cálculos físicos; cuidado com perda de precisão
  perto de v → c (usar formulações numericamente estáveis, ex. evitar 1−v²/c² catastrófico).
- **Robustez:** tratamento de falhas de rede, CSV corrompido, seleção inválida.

---

## 9. Fora de Escopo (MVP)

- Multiplayer / rede em tempo real.
- Relatividade Geral completa (geodésicas em campos fortes, lentes gravitacionais).
- Expansão cósmica / cosmologia (redshift cosmológico).
- Planetas, naves detalhadas, combustível/massa-energia realista (pode virar extensão).
- Build para Windows/Linux (foco macOS/Metal).

---

## 10. Glossário

- **c:** velocidade da luz no vácuo (~299.792.458 m/s).
- **γ (gamma):** fator de Lorentz.
- **Tempo próprio (τ):** tempo medido no referencial da nave (relógio de bordo).
- **Tempo coordenado (t):** tempo medido no referencial da origem (observador parado).
- **Parsec (pc):** ~3,26 anos-luz.
- **HYG:** Hipparcos + Yale + Gliese (catálogo estelar).
- **Aberração:** distorção aparente da posição das estrelas devido à velocidade do observador.
- **Doppler relativístico:** mudança de cor (frequência) da luz devido à velocidade relativa.
