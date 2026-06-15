// ============================================================================
//  basic.metal — shaders mínimos para a cena de teste 3D (T2.1).
//
//  Pipeline mais simples possível que ainda prova o caminho completo:
//    - vértices com posição (float3) e cor (float3);
//    - uma única matriz uniforme MVP (view*projection, modelo na identidade);
//    - fragment que apenas repassa a cor interpolada.
//
//  Carregado e compilado em runtime por MetalWindow.mm (newLibraryWithSource:).
//  Quando o app for empacotado, migrar para um .metallib pré-compilado no bundle.
// ============================================================================

#include <metal_stdlib>
using namespace metal;

// Layout de cada vértice enviado pela CPU (deve casar com VertexCPU no .mm).
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
};

// Saída do vertex → entrada do fragment.
struct VertexOut {
    float4 position [[position]];  // clip-space.
    float3 color;
};

// Uniforme: matriz MVP (column-major, igual ao GLM).
struct Uniforms {
    float4x4 mvp;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant Uniforms& u [[buffer(1)]]) {
    VertexOut out;
    out.position = u.mvp * float4(in.position, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}
