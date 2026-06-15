// ============================================================================
//  stars.metal — render do campo de estrelas como pontos instanciados (T2.2).
//
//  Uma estrela = um vértice desenhado com MTLPrimitiveTypePoint. O vertex shader
//  projeta a posição (parsecs) pela matriz MVP e define o tamanho do ponto via
//  [[point_size]]; o fragment usa [[point_coord]] para desenhar um disco macio
//  (alpha cai do centro p/ a borda), evitando "quadradinhos" duros.
//
//  Layout do vértice (deve casar com StarInstance em StarField.h):
//    position (float3) + color (float3) + size (float1).
//  Compilado em runtime por MetalWindow.mm.
// ============================================================================

#include <metal_stdlib>
using namespace metal;

struct StarIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
    float  size     [[attribute(2)]];
};

struct StarOut {
    float4 position [[position]];
    float  pointSize [[point_size]];
    float3 color;
};

struct Uniforms {
    float4x4 mvp;
};

vertex StarOut star_vertex(StarIn in [[stage_in]],
                           constant Uniforms& u [[buffer(1)]]) {
    StarOut out;
    out.position = u.mvp * float4(in.position, 1.0);
    out.color = in.color;
    out.pointSize = in.size;
    return out;
}

fragment float4 star_fragment(StarOut in [[stage_in]],
                              float2 pc [[point_coord]]) {
    // Distância ao centro do ponto (point_coord vai de 0..1 nas bordas).
    float2 d = pc - float2(0.5, 0.5);
    float r = length(d) * 2.0;            // 0 no centro, 1 na borda.
    // Disco macio: opaco no centro, transparente na borda (descarta o canto).
    float alpha = smoothstep(1.0, 0.2, r);
    if (alpha <= 0.0) discard_fragment();
    return float4(in.color, alpha);
}
