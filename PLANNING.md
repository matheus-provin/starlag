# PLANNING — Taskboard do `starlag`

> Derivado de [REQUIREMENTS.md](REQUIREMENTS.md). Versão 1.0 — 2026-06-15.
>
> **Como usar este board:** ao concluir uma tarefa, marque o checkbox como `[x]`, mude o
> badge de status para `✅ DONE`, e preencha o bloco expansível `<details>` abaixo dela com
> o que foi feito, decisões tomadas, arquivos criados e observações relevantes.
>
> **Legenda de status:** ⬜ TODO · 🟡 IN PROGRESS · ✅ DONE · ⏸️ BLOCKED

---

## Marco 0 — Fundação do Projeto

### [x] T0.1 — Setup do projeto Xcode + estrutura de pastas · ✅ DONE
Criar o `.xcodeproj` (com CMake opcional), estrutura de diretórios (`src/`, `shaders/`,
`data/`, `third_party/`, `assets/`), `.gitignore` e README mínimo. Definir padrão C++17/20.
<details><summary>Notas de implementação</summary>

**O que foi feito**
- Estrutura de pastas criada em `claude-cpp/`: `src/{core,physics,data,render,ui}`,
  `shaders/`, `data/`, `third_party/`, `assets/`, `tests/` (cada pasta com `.gitkeep`).
- Build via **CMake como fonte única**, configurado para gerar Xcode (decisão do usuário).
  Padrão **C++20** (`CMAKE_CXX_STANDARD 20`, sem extensões), avisos `-Wall -Wextra -Wpedantic`,
  `compile_commands.json` exportado, `Debug` como build type padrão.
- Stub `src/main.cpp` que compila e roda (imprime versão + próximo passo).
- Suíte de testes via **CTest** com teste-sentinela (`tests/test_sentinel.cpp`) validando a
  infra de testes desde o M0. Decisão: sem framework externo por ora (asserções + exit code);
  reavaliar Catch2/doctest header-only se a suíte crescer.
- `.gitignore` (build/, cache de dados, Xcode userdata, `.DS_Store`) e `README.md` com stack,
  estrutura e comandos de build.

**Arquivos criados**
- `CMakeLists.txt` (raiz) — C++20, opção `STARLAG_BUILD_TESTS`, `add_subdirectory(src/tests)`.
- `src/CMakeLists.txt` — alvo `starlag` (executável); subdiretórios de camadas comentados
  para ativar conforme os marcos.
- `src/main.cpp` — ponto de entrada stub.
- `tests/CMakeLists.txt` + `tests/test_sentinel.cpp` — alvo de teste + `add_test`.
- `.gitignore`, `README.md`, `.gitkeep` nas pastas vazias.

**Comandos de build**
```sh
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build-xcode -G Xcode   # gera o .xcodeproj
```

**Validação / observações**
- ⚠️ `cmake` **não está instalado** nesta máquina (`brew install cmake` ou CMake.app
  pendente). Não foi possível rodar o fluxo `cmake -S . -B build` aqui.
- Como prova de que o código está correto, compilei direto com o toolchain do sistema:
  `clang++ -std=c++20 -Wall -Wextra -Wpedantic -Isrc src/main.cpp` → **compila e roda OK**;
  `tests/test_sentinel.cpp` → **compila e passa (exit 0)**. Logo o `.xcodeproj`/CMake só
  dependem de instalar o CMake; os fontes e a configuração estão prontos.

</details>

### [x] T0.2 — Decidir e integrar camada de janela (GLFW vs SDL2) · ✅ DONE
Avaliar GLFW vs SDL2 para criação de janela + input com camada Metal. Integrar a escolhida,
abrir uma janela Metal vazia (clear color) rodando a 60 FPS.
<details><summary>Notas de implementação</summary>

**Decisão: GLFW** (já indicado no bootstrap). Instalado via Homebrew (`brew install glfw`,
v3.4). Motivos: backend ImGui pronto (útil no M4), API enxuta de janela/input, e exposição
nativa da `NSWindow` (`glfwGetCocoaWindow`) que permite anexar uma `CAMetalLayer` sem
fricção. SDL2 ficaria como alternativa caso precisássemos de áudio/gamepad — fora de escopo.

**O que foi feito**
- Ponte **Objective-C++** isolada na camada `render/`, com interface C++ limpa via PIMPL
  (nenhum tipo ObjC vaza para o resto do app):
  - `MetalWindow.h` — classe `MetalWindow` (C++ puro): `isOpen()`, `pollEvents()`,
    `renderClearFrame(ClearColor)`; struct `ClearColor` (RGBA em `double`).
  - `MetalWindow.mm` — impl ObjC++: `glfwInit` + `GLFW_NO_API`, `MTLCreateSystemDefaultDevice`,
    `MTLCommandQueue`, `CAMetalLayer` (`BGRA8Unorm`, `framebufferOnly`) anexada ao
    `contentView` da `NSWindow`. `contentsScale`/`drawableSize` acompanham o backing
    scale (Retina) e o framebuffer size (resize). Render loop: `nextDrawable` → render pass
    com `loadAction = Clear` (clear color) → `presentDrawable` → `commit` (present no vsync).
- `main.cpp`: abre janela 1280×720, roda o loop até fechar, mede e imprime **FPS médio** a
  cada ~1s. Clear color = azul-noite (`0.02, 0.02, 0.06`), o céu profundo do app.
- Contagem global de janelas para `glfwInit`/`glfwTerminate` idempotentes.

**Arquivos criados/alterados**
- `src/render/MetalWindow.h` (novo), `src/render/MetalWindow.mm` (novo),
  `src/render/CMakeLists.txt` (novo — lib `starlag_render`).
- `src/main.cpp` (reescrito: stub → render loop).
- `src/CMakeLists.txt` (linka `starlag_render`), `CMakeLists.txt` raiz (`LANGUAGES CXX OBJCXX`).

**CMake / build**
- `find_package(glfw3 REQUIRED)`; frameworks `Metal QuartzCore Cocoa IOKit CoreFoundation`.
- `-fobjc-arc` no `.mm` (ARC gerencia os objetos ObjC).
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
./build/src/starlag
```

**Validação (na sessão gráfica do usuário)**
- Janela abriu; Terminal mostrou FPS estável travado no **vsync do display** (~100–120 FPS
  em monitor ProMotion do usuário — i.e., sincronizado ao refresh, requisito atendido);
  fechamento limpo (`Janela fechada. Ate logo.`).
- ⚠️ Observação de ambiente: o agente roda headless (sem WindowServer), então a janela
  **não** abre no contexto do agente (erros `Connection invalid`/`hiservices-xpcservice`);
  a validação visual/FPS foi feita pelo usuário rodando o binário localmente. O build em si
  é validável pelo agente (compila e linka limpo).

</details>

### [x] T0.3 — Integrar dependências (ImGui, GLM, libcurl, nlohmann/json) · ✅ DONE
Adicionar bibliotecas via submódulos/`third_party` ou gerenciador. Garantir build limpo no
Xcode com todas linkando. Validar com um "hello" de cada (ImGui na tela, request HTTP, parse JSON).
<details><summary>Notas de implementação</summary>

**Estratégia de obtenção das deps**
- **GLM** (1.0.3), **nlohmann/json** (3.12.0), **GLFW** (3.4) → Homebrew.
- **libcurl** (8.7.1) → já incluída no SDK do macOS (`find_package(CURL)`); nada a instalar.
- **Dear ImGui** (v1.91.5) → vendorizada em `third_party/imgui/` via `git clone`. Compilada
  como lib estática própria (ImGui não tem pacote CMake oficial). Apenas o **core** (imgui,
  imgui_draw, imgui_tables, imgui_widgets); backend Metal+plataforma fica para a T4.1.

**O que foi feito**
- Smoke test headless `src/core/deps_smoke.cpp`: prova cada lib no nível básico e retorna
  exit 0 só se TODOS os checks passam (vira teste CTest, validável pelo agente sem GUI):
  - GLM: soma de vetores, `length` 3-4-5, `glm::perspective`.
  - json: round-trip parse/serialize (string, double, array).
  - libcurl: `curl_global_init`, `curl_version_info`, `curl_easy_init` (sem tráfego de rede —
    request real fica para T1.1).
  - ImGui: `CreateContext`, atlas de fonte em memória, um frame headless `NewFrame→Render`
    com `DisplaySize`/`DeltaTime` manuais (sem backend/GPU — render real fica para T4.1).
- CMake resiliente: se `third_party/imgui` não existir, emite WARNING com o comando de clone
  e apenas desabilita o `deps_smoke` (não quebra o build).

**Arquivos criados/alterados**
- `src/core/deps_smoke.cpp` (novo), `src/core/CMakeLists.txt` (novo).
- `third_party/CMakeLists.txt` (novo — lib `imgui`).
- `CMakeLists.txt` raiz: `enable_testing()` movido para antes dos subdiretórios;
  `add_subdirectory(third_party)` antes de `src/`.
- `src/CMakeLists.txt`: ativa `core/`.
- `README.md`: seção "Dependências" com comandos.

**CMake / build / validação**
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
./build/src/core/deps_smoke      # 12/12 checks PASS, exit 0
ctest --test-dir build           # 2/2 testes (deps_smoke + sentinel) PASS
```
- Build limpo de todos os alvos (`imgui`, `starlag_render`, `starlag`, `deps_smoke`,
  `test_sentinel`). Validação 100% pelo agente (headless) — não precisou de GUI.

</details>

---

## Marco 1 — Camada de Dados Estelares

### [x] T1.1 — Cliente HTTP + download remoto do catálogo HYG com cache · ✅ DONE (parcial: cache local)
Implementar download do CSV HYG de URL remota via libcurl, salvar em cache local, detectar
cache existente e pular o download. Tratar falha de rede graciosamente.
<details><summary>Notas de implementação</summary>

**Escopo desta entrega (decidido com o usuário): cache local agora; download remoto adiado.**
Motivo: o ambiente do agente não tem acesso de rede ao repositório do HYG (sandbox só alcança
hosts internos). O usuário baixou `hygdata_v42.csv.gz` manualmente; eu descompactei para
`data/hygdata_v42.csv` (32 MB, 119626 estrelas, HYG v4.2).

**O que foi feito** (`data/CatalogCache.{h,cpp}`, lib `starlag_data`):
- `defaultCatalogPath()` → "data/hygdata_v42.csv" (TODO: migrar p/ ~/Library/Application
  Support/starlag/ no app empacotado).
- `locateCatalog(path)` → `CatalogCacheStatus {found, path, sizeBytes, message}`. Verifica
  existência, tipo (arquivo regular), tamanho (rejeita 0 bytes), via `std::filesystem` com
  `error_code` (sem exceções). Mensagens legíveis para UI/log.
- `fetchRemote(url, dest)` — **stub documentado** que retorna false (download via libcurl fica
  para quando houver rede; URL de referência do Astronexus anotada no header). libcurl já
  linkada no CMake p/ não re-mexer depois.

**Testes (`tests/test_catalog_cache.cpp`, alvo `catalog_cache`) — 9 checks PASS:**
- Catálogo real detectado (caminho absoluto injetado via `STARLAG_TEST_CATALOG` no CMake, pois
  o CTest roda de outro diretório): encontrado, >1MB, path ecoado.
- Ausência tratada (não encontrado, tamanho 0, mensagem não-vazia).
- Stub de download retorna false. Caminho default correto.

**Validação do catálogo (sanidade dos dados):** Vega=7.6787pc≈25.04ly (bate com os testes de
física!), Sirius=2.64pc, Sol na linha 2 (id 0, x=y=z=0). Esquema HYG v4.2 confirmado com todos
os campos do REQUIREMENTS §3.2.

**Pendência explícita p/ fechar a T1.1 plena:** implementar `fetchRemote` real (libcurl GET +
salvar em cache + pular se já existe) quando o download remoto for viável.

**Arquivos:** `src/data/{CatalogCache.h,CatalogCache.cpp,CMakeLists.txt}` (novos),
`src/CMakeLists.txt` (ativa data/), `tests/test_catalog_cache.cpp` + CMakeLists.
```sh
cmake --build build && ctest --test-dir build   # 7/7 PASS
```

</details>

### [x] T1.2 — Parser CSV e modelo `Star` em memória · ✅ DONE
Definir `struct Star` (id, nome, x/y/z, dist, mag, absmag, spect, ci...). Parsear o CSV para
um `std::vector<Star>`. Validar contagem (~120k) e campos. Converter parsecs→anos-luz.
<details><summary>Notas de implementação</summary>

**O que foi feito**
- `data/Star.h`: `struct Star` com campos do REQUIREMENTS §3.2 (id, hip, hd, gl, proper, x/y/z,
  distPc **e** distLy, mag, absmag, spect, ci + flag hasCi). Guarda distância nas duas unidades;
  `hasProperName()`.
- `data/CatalogParser.{h,cpp}`:
  - `splitCsvLine` — splitter que **respeita aspas duplas** (campos "texto"/cru, vazios, aspas
    escapadas ""). Verificado nos dados: todas as 119627 linhas têm 37 campos, sem vírgula
    embutida — mas o splitter trata o caso geral mesmo assim.
  - `parseCatalogFile`/`parseCatalogString` → `ParseReport {stars, totalLines, skipped, ok, message}`.
  - **Colunas localizadas por NOME do cabeçalho** (não por índice fixo) — robusto a reordenação
    entre versões do HYG. Exige id/x/y/z/dist no header senão falha com mensagem clara.
  - Linhas malformadas (poucos campos) são **contadas e puladas**, não abortam o load.
  - Conversão parsec→ano-luz via `physics::kParsec_ly` (data passa a depender de physics no CMake).

**Testes (`tests/test_catalog_parser.cpp`, alvo `catalog_parser`) — 31 checks PASS:**
- splitCsvLine: campos vazios, aspas, vírgula-dentro-de-aspas.
- Parse sintético: campos, conversão 10pc→32.6156ly, hasCi, gl com aspas.
- Robustez: linha malformada pulada (não aborta), contadores corretos.
- **Catálogo HYG real:** 119626 estrelas, 0 puladas. Sol (id 0, spect G2V), Vega (7.6787pc /
  25.04ly — **bate com a física!**), Sirius (2.6371pc).

**Correção de referência (4ª vez):** "Sol na origem" falhou — o HYG dá Sol com **x=0.000005**
(epsilon do dataset), não zero exato. O parser estava certo; ajustei o teste para "essencialmente
na origem" (raio < 1e-3 pc). Reforça o hábito de checar o dado real.

**Arquivos:** `src/data/{Star.h,CatalogParser.h,CatalogParser.cpp}` (novos), `CMakeLists.txt` da
data (+CatalogParser, +link physics), `tests/test_catalog_parser.cpp` + CMakeLists.
```sh
cmake --build build && ctest --test-dir build   # 8/8 PASS
```

</details>

### [ ] T1.3 — Índice de busca + seleção de estrelas por nome/id · ⬜ TODO
Estruturas para localizar estrela por nome próprio, HIP/HD, e por proximidade espacial.
Suporte a busca textual (para o painel de busca da UI) e lookup rápido.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T1.4 — Enriquecimento via API externa (SIMBAD/Wikipedia) sob demanda · ⬜ TODO
Ao selecionar uma estrela, buscar nome comum/tipo espectral/descrição via API. Cache em
memória/disco. Degradar graciosamente se offline. (Requisito "consumir APIs".)
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

---

## Marco 2 — Render 3D (Metal)

### [ ] T2.1 — Pipeline Metal base + câmera e matrizes (MVP render) · ⬜ TODO
Configurar device/queue/pipeline Metal, depth buffer, e shaders mínimos. Implementar câmera
com matrizes view/projection (GLM). Renderizar um triângulo/grade de teste.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T2.2 — Renderizar campo de estrelas (billboards/pontos instanciados) · ⬜ TODO
Enviar as estrelas do catálogo para a GPU (instancing). Renderizar como pontos/billboards,
cor por tipo espectral (ci), brilho por magnitude. Alvo: dezenas de milhares a 60 FPS.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T2.3 — Câmera livre 6-DOF (fly/orbit) com input · ⬜ TODO
Navegação WASD + mouse, zoom logarítmico (escala da galáxia), controles suaves. Integrar
com a camada de janela (T0.2).
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T2.4 — Picking 3D: seleção de estrela por clique + hover/tooltip · ⬜ TODO
Ray-casting do mouse para o espaço 3D, encontrar estrela mais próxima do raio, destacar no
hover, selecionar no clique. Base para escolher origem/destino.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

---

## Marco 3 — Núcleo Físico Relativístico

### [x] T3.1 — Módulo de física: dilatação temporal (velocidade constante) · ✅ DONE
Implementar γ, Δτ = Δt/γ, com `double` e formulação numericamente estável perto de c.
Limitar v ≤ 99.99999% c. Testes unitários com casos conhecidos.
<details><summary>Notas de implementação</summary>

**O que foi feito**
- Módulo `physics/` como biblioteca C++ pura (`starlag_physics`), sem deps externas — 100%
  testável headless.
- `Constants.h`: constantes em `double` (c exato, ano juliano, ano-luz, parsec→ly,
  `kMaxBeta = 0.9999999`). **Unidades naturais do app: ly e ano juliano, com c = 1 ly/yr** —
  simplifica e estabiliza a viagem a v constante.
- `Relativity.{h,cpp}`:
  - `lorentzFactor(β)` = 1/√((1−β)(1+β)). **Decisão de estabilidade numérica:** fatorar
    1−β² como (1−β)(1+β) evita o cancelamento catastrófico quando β→1 (calcular β² e subtrair
    de 1 perde dígitos significativos perto de c).
  - `clampBeta(β)`: satura em [0, kMaxBeta], NaN→0 (nunca permite β≥1).
  - `properTimeFromCoordinate` (τ=t/γ), `coordinateTimeFromProper` (t=τ·γ).
  - `computeConstVelocityTrip(distLy, β)` → struct `ConstVelocityTrip` {β, γ, dist, tempo
    coordenado, tempo próprio, distância contraída}. Em c=1 ly/yr: t=dist/β, τ=t/γ,
    dist_contraída=dist/γ. Trata bordas (β=0, dist=0) sem divisão por zero.

**Testes (`tests/test_relativity.cpp`, alvo CTest `relativity`)** — casos de referência:
  - γ: β=0→1; β=0.6→1.25; β=0.8→5/3; β=√3/2→2; β=0.99→7.0888; β=0.999→22.36627.
  - Estabilidade no limite kMaxBeta (γ≈2236.07, finito).
  - clampBeta (negativo, normal, saturação, NaN).
  - Relações de tempo (round-trip τ↔t).
  - Viagem completa: 10ly@0.8 (t=12.5, τ=7.5, contraída=6.0); Sol→Vega 25.04ly@0.999;
    bordas β=0 e dist=0. Sanidade física τ<t.
  - **27 checks, 3/3 testes CTest PASS.**

**Correção científica validada**
- Durante o desenvolvimento, 2 checks falharam — eram **valores esperados errados que eu
  havia hardcoded** (γ(0.999) como 22.366333602). O **código** dava 22.3662720421. Verifiquei
  independentemente em Python (`1/√(1−β²)`) → 22.3662720421: o código estava certo, os valores
  de referência do teste é que estavam errados. Corrigidos. Lição: validar a referência, não
  só o código.

**Arquivos:** `src/physics/{Constants.h,Relativity.h,Relativity.cpp,CMakeLists.txt}` (novos),
`tests/test_relativity.cpp` + `tests/CMakeLists.txt` (alvo novo), `src/CMakeLists.txt` (ativa physics).
```sh
cmake --build build && ctest --test-dir build --output-on-failure   # 3/3 PASS
```

</details>

### [x] T3.2 — Perfil de voo com aceleração própria constante (hiperbólico) · ✅ DONE
Fases acelera-coast-desacelera com fórmulas hiperbólicas (cosh/sinh/tanh). Calcular tempo
próprio e coordenado totais. Tratar viagem curta (perfil triangular). Testes unitários.
<details><summary>Notas de implementação</summary>

**Modelo:** "foguete relativístico" em espaço plano. Aceleração própria constante `a` (ly/yr²),
c=1 ly/yr. Fórmulas hiperbólicas: β=tanh(aτ), γ=cosh(aτ), d=(1/a)(cosh(aτ)−1), t=(1/a)sinh(aτ).

**O que foi feito**
- `FlightProfile.{h,cpp}` na lib `starlag_physics`:
  - `computeAcceleratedTrip(distLy, a, targetBeta)` → `FlightProfileResult` com perfil
    (`FlightKind::Trapezoidal`/`Triangular`), τ e t totais, β/γ de pico, e **decomposição por
    fase** (τ/t/dist de aceleração e de cruzeiro — úteis p/ a timeline da animação T5.1).
  - **Trapezoidal** (acelera–coast–desacelera): quando 2·d_aceleração ≤ D, há cruzeiro a
    β_alvo cobrindo a distância restante (t=d/β, τ=t/γ).
  - **Triangular** (viagem curta): quando não cabe, acelera até D/2 e desacelera; β_alvo nunca
    atingido. γ_pico = 1 + a·(D/2); β_pico = √(1 − 1/γ²); τ_meio = acosh(γ)/a.
  - Reusa `lorentzFactor` (γ estável perto de c) e `clampBeta`. Trata bordas (dist=0, a=0,
    β=0) sem divisão por zero / NaN.
- `Constants.h`: `kStandardGravity_m_s2`, `kOneG_ly_yr2` (≈1.032295 ly/yr²) e `gToLyPerYr2(g)`.
  Detalhe físico bonito: c/a a 1g ≈ 0.9687 ano (o "~1 ano até perto de c").

**Testes (`tests/test_flight_profile.cpp`, alvo CTest `flight_profile`) — 30 checks, todos PASS**
  - **Valores de referência verificados em Python ANTES de escrever o teste** (lição da T3.1):
    - Vega 25.04ly@1g (triangular): τ≈6.44291, t≈26.90777, γ_pico≈13.92434, β_pico≈0.997418.
    - 1000ly@1g cap0.99 (trapezoidal): τ≈145.93906, t≈1011.78194, γ_pico=7.0888120.
    - 1ly@1g (triangular): τ≈1.89234, t≈2.20791, γ_pico≈1.516148.
  - Conservação de distância (2·accel + coast = D), bordas, sanidade τ≤t.
  - Resultado bate com a literatura do relativistic rocket (≈6.4 anos próprios a 1g até Vega).

**Arquivos:** `src/physics/{FlightProfile.h,FlightProfile.cpp}` (novos), `Constants.h` (+1g),
`src/physics/CMakeLists.txt` (+FlightProfile.cpp), `tests/test_flight_profile.cpp` + CMakeLists.
```sh
cmake --build build && ctest --test-dir build   # 4/4 PASS (sentinel, deps_smoke, relativity, flight_profile)
```

</details>

### [x] T3.3 — Calendário gregoriano: datas de partida/chegada nos dois referenciais · ✅ DONE
Converter durações (tempo próprio e coordenado) em datas gregorianas a partir da data de
partida. Lidar com anos longos (séculos/milênios) com precisão.
<details><summary>Notas de implementação</summary>

**Decisão técnica:** NÃO usar `std::chrono` diretamente — viagens longas geram chegadas a
milhares/milhões de anos, fora do suporte robusto de chrono entre toolchains. Usamos os
**algoritmos de calendário de Howard Hinnant** (`days_from_civil`/`civil_from_days`, baseados
em "dias desde 1970-01-01"), exatos para todo o gregoriano proléptico com aritmética inteira
de 64 bits. Domínio público; nomes mantidos próximos do original p/ rastreabilidade.

**O que foi feito** (`Calendar.{h,cpp}` na lib `starlag_physics`):
- `struct Date {int64_t year; int month, day; double dayFraction;}` — ano `int64` (séculos/
  milênios/negativos), `dayFraction` em [0,1) preserva a hora ao somar durações fracionárias.
- `daysFromCivil(y,m,d)` / `civilFromDays(days)` — conversão exata data↔dia civil.
- `addYears(Date, double anos)` — soma duração em **anos julianos (365.25 d)**, consistente com
  a definição de ano-luz da física. Usa `floor` para o dia civil (a fração de dia NÃO arredonda
  para cima — uma chegada às 17h do dia 3 é dia 3, não dia 4).
- `formatDate` → "YYYY-MM-DD".
- `Constants.h`: extraído `kJulianYear_days = 365.25`.

**Testes (`tests/test_calendar.cpp`, alvo CTest `calendar`) — todos PASS**
- **Referências verificadas em Python** (Hinnant): âncoras (1970-01-01=0, 2000-01-01=10957,
  2026-06-15=20619, 2032-11-23=22972, 1969-12-31=-1).
- Round-trip civil↔days exaustivo: **0 falhas** no intervalo ~ano -1900 a 6200.
- `addYears`: 6.443yr→2032-11-23, 25.293yr→2051-09-30, 1011.78yr→3038-04-03, +0 mantém,
  fração em [0,1), salto de +100000 anos com round-trip.

**Correção de referência (de novo!):** 1 check falhou — eu havia esperado 3038-04-04 (do meu
Python com `round()`), mas o código usa `floor()` (correto: frac de dia 0.645 → ainda dia 3).
Confirmei em Python que floor→dia 3 e round→dia 4: o **código** estava certo. Corrigi a referência.

**Integração no app:** o demo de viagem do `main.cpp` agora imprime **datas gregorianas de
chegada nos dois referenciais** (ex.: Vega@1g → Terra chega 2053-05-12, relógio de bordo
2032-11-23). Substitui o placeholder "(datas chegam na T3.3)".

**Arquivos:** `src/physics/{Calendar.h,Calendar.cpp}` (novos), `Constants.h`, CMakeLists da
física, `tests/test_calendar.cpp` + CMakeLists, `src/main.cpp` (datas no demo).
```sh
cmake --build build && ctest --test-dir build   # 5/5 PASS
```

</details>

### [x] T3.4 — Distância contraída e métricas derivadas · ✅ DONE
Calcular contração de Lorentz da distância percebida pela tripulação, velocidade máxima
atingida, e "dívida temporal" entre referenciais. Empacotar num `struct SimulationResult`.
<details><summary>Notas de implementação</summary>

**Fachada do núcleo físico** (`Simulation.{h,cpp}`): une T3.1/T3.2/T3.3 numa API única que a UI
(M4) vai consumir. Entrada `TripRequest` (distância, modo const/acelerado, β cruzeiro, accel em g,
data de partida, nomes origem/destino) → `runSimulation()` → `SimulationResult`.

**`SimulationResult` (REQUIREMENTS §6):** peakBeta, peakGamma, contractedDistanceLy, properTimeYr,
coordinateTimeYr, timeDebtYr (t−τ), arrivalDateOrigin, arrivalDateShip, FlightProfileResult (detalhe
do perfil), e **summary** — resumo em linguagem natural PT-BR pronto p/ exibir (ex.: "Voce chegou a
Vega. Para a tripulacao passaram-se 6.4 anos...; na Terra, 26.9 anos (ano 2053). Voce 'saltou' 20.5
anos para o futuro.").

**Decisões:**
- `PhysicsMode::{ConstantVelocity, Accelerated}` seleciona o caminho físico.
- **Distância contraída no modo acelerado:** γ varia ao longo da rota, então usamos D/γ_pico (o
  encurtamento MÁXIMO, na velocidade de pico) — métrica representativa, documentada como
  simplificação (a "distância própria" integrada exigiria integrar a trajetória; fora do MVP). No
  modo constante é D/γ (exato).
- `summary` escolhe unidade legível (dias se τ<2 anos; senão anos).

**Testes (`tests/test_simulation.cpp`, alvo `simulation`) — 22 checks PASS:** valores agregados
batem com os módulos subjacentes (Vega@0.99c e Vega@1g, refs já verificadas nas T3.1–3.3); métricas
derivadas (dívida temporal, D/γ); consistência entre modos (τ<t, contraída≤real, chegada origem≥bordo);
bordas (dist=0). Datas de chegada conferem (origem 2051-09-30; bordo 2032-11-23).

**Refator do demo:** `main.cpp` agora usa `runSimulation()` em vez de chamar os módulos soltos —
demonstra que a fachada substitui e simplifica o uso. Imprime o resumo em linguagem natural.

**Arquivos:** `src/physics/{Simulation.h,Simulation.cpp}` (novos), CMakeLists da física,
`tests/test_simulation.cpp` + CMakeLists, `src/main.cpp` (refatorado p/ a fachada).
```sh
cmake --build build && ctest --test-dir build   # 6/6 PASS
```

**>>> MARCO 3 (núcleo físico relativístico) COMPLETO <<<** — toda a física testável headless,
validada contra casos de referência, exposta por uma API limpa pronta para a UI.

</details>

---

## Marco 4 — UI / Interação (Dear ImGui)

### [ ] T4.1 — Integrar ImGui ao loop Metal + painel de parâmetros · ⬜ TODO
Backend ImGui-Metal renderizando sobre a cena. Painel com sliders/inputs: velocidade,
aceleração, data de partida, modo de física, toggles de efeitos visuais.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T4.2 — Painel de seleção origem/destino + busca de estrelas · ⬜ TODO
UI para confirmar origem/destino (via picking ou busca textual), com info enriquecida
(T1.4). Lista/busca por nome.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T4.3 — Painel de resultados da simulação · ⬜ TODO
Exibir SimulationResult: tempo próprio, tempo coordenado, datas gregorianas, γ, distância
contraída, velocidade máxima, e resumo textual em linguagem natural.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

---

## Marco 5 — Animação da Viagem & Efeitos Visuais Relativísticos

### [ ] T5.1 — Linha de rota + nave animada com timeline (play/pause/velocidade) · ⬜ TODO
Desenhar rota origem→destino, marcador/nave percorrendo o trajeto em função do tempo
simulado. Controles de reprodução (play/pause, velocidade de replay).
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T5.2 — Aberração relativística da luz (shader) · ⬜ TODO
Deslocar posições aparentes das estrelas para a frente conforme v aumenta. Implementar em
vertex shader Metal. Toggle on/off.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T5.3 — Doppler relativístico + beaming (shader) · ⬜ TODO
Blueshift à frente / redshift atrás; aumento de brilho à frente (headlight effect).
Fragment shader Metal. Toggle on/off.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

---

## Marco 6 — Polimento & Robustez

### [ ] T6.1 — Tratamento de erros (rede, CSV, seleção inválida) · ⬜ TODO
Mensagens claras na UI para falhas de download, parse, e entradas inválidas. Estados de
carregamento. Fallbacks offline.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T6.2 — Validação científica dos cálculos (casos de referência) · ⬜ TODO
Comparar resultados com valores conhecidos (ex.: viagem a 1g, casos de livros de
relatividade). Documentar precisão e limites numéricos.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

### [ ] T6.3 — Documentação final + comentários de código · ⬜ TODO
README de build/uso, comentários explicativos (foco aprendizado), diagrama de arquitetura
(física/dados/render/UI). Atualizar REQUIREMENTS se algo mudou.
<details><summary>Notas de implementação</summary>

_(a preencher ao concluir)_

</details>

---

## Dependências entre tarefas (ordem sugerida)

```
T0.1 → T0.2 → T0.3
                 ├─→ T1.1 → T1.2 → T1.3 → T1.4
                 └─→ T2.1 → T2.2 → T2.3 → T2.4
T3.1 → T3.2 → T3.3 → T3.4         (física, independente do render; pode ir em paralelo)
(T2.* + T1.*) → T4.1 → T4.2 → T4.3
(T3.* + T4.*) → T5.1 → T5.2 → T5.3
tudo → T6.1 → T6.2 → T6.3
```

> A **física (Marco 3)** pode ser desenvolvida e testada em paralelo com render/dados, pois
> não depende de gráficos — ótimos candidatos a testes unitários puros em C++.
