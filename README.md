# starlag


<img width="1186" height="707" alt="Captura de Tela 2026-06-15 às 17 05 34" src="https://github.com/user-attachments/assets/1559da3a-829d-4600-a018-a8f3c1ae0347" />


Simulador 3D de viagens interestelares relativísticas (C++ / Metal / macOS).

Navegue por um mapa galáctico 3D com estrelas reais (catálogo HYG), escolha
origem e destino, ajuste velocidade (até 99.99999% de *c*), aceleração própria e
data de partida, e veja, ao chegar, **quanto tempo passou para a tripulação vs.
para um observador na origem**, com datas no calendário gregoriano.

Projeto de **aprendizado pessoal**: C++ moderno, API gráfica **Metal** e física
relativística aplicada. Prioridades: clareza de código, evolução incremental e
correção científica.


## Stack

| Camada | Escolha |
|---|---|
| Linguagem | C++20 |
| Build | CMake (gera Xcode) |
| Render 3D | Metal |
| UI | Dear ImGui (backend Metal) |
| Janela/Input | GLFW (a validar no M0) |
| Dados | HYG Database (CSV) remoto + cache local |
| HTTP/JSON | libcurl, nlohmann/json |
| Matemática | GLM |

## Estrutura de pastas

```
claude-cpp/
├─ CMakeLists.txt        # build raiz (C++20, app + testes)
├─ src/
│  ├─ main.cpp           # ponto de entrada
│  ├─ core/              # tipos base, utilidades, app loop
│  ├─ physics/           # física relativística (Marco 3)
│  ├─ data/              # catálogo HYG, HTTP, parsing (Marco 1)
│  ├─ render/            # pipeline Metal, câmera (Marco 2)
│  └─ ui/                # painéis Dear ImGui (Marco 4)
├─ shaders/              # shaders Metal (.metal)
├─ data/                 # cache do catálogo (não versionado)
├─ third_party/          # dependências (ImGui, GLM, ...)
├─ assets/               # recursos
└─ tests/                # testes unitários (foco: física)
```

## Dependências

| Lib | Como obter |
|---|---|
| GLFW | `brew install glfw` |
| GLM | `brew install glm` |
| nlohmann/json | `brew install nlohmann-json` |
| libcurl | já incluída no SDK do macOS |
| Dear ImGui | `git clone --depth 1 --branch v1.91.5 https://github.com/ocornut/imgui.git third_party/imgui` |

Resumo:

```sh
brew install glfw glm nlohmann-json
git clone --depth 1 --branch v1.91.5 https://github.com/ocornut/imgui.git third_party/imgui
```

## Build

Por linha de comando (Ninja/Make):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/starlag         # app (janela Metal)
./build/src/core/deps_smoke # valida as dependências (headless)
```

Gerar projeto Xcode (macOS):

```sh
cmake -S . -B build-xcode -G Xcode
open build-xcode/starlag.xcodeproj
```

> Requer CMake ≥ 3.20 e um toolchain C++20 (Apple Clang via Xcode Command Line
> Tools).
# starlag
