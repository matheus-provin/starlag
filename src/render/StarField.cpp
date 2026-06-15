// ============================================================================
//  StarField.cpp — montagem do buffer de instâncias de estrelas (T2.2).
// ============================================================================

#include "render/StarField.h"

#include <algorithm>

namespace starlag::render {

float magnitudeToSize(double mag, const StarFieldParams& params) {
    // Interpolação linear invertida entre brightMag (→maxSize) e faintMag
    // (→minSize). Fora da faixa, satura nos extremos.
    const double lo = params.brightMag;
    const double hi = params.faintMag;
    if (hi <= lo) return params.minSize;  // proteção contra faixa degenerada.

    // t=0 em brightMag, t=1 em faintMag.
    double t = (mag - lo) / (hi - lo);
    t = std::max(0.0, std::min(1.0, t));

    // Mais brilhante (t=0) → maxSize; mais fraca (t=1) → minSize.
    const float size = params.maxSize + static_cast<float>(t) * (params.minSize - params.maxSize);
    return size;
}

std::vector<StarInstance> buildStarField(const std::vector<data::Star>& stars,
                                         const StarFieldParams& params) {
    std::vector<StarInstance> out;
    out.reserve(stars.size());

    for (const data::Star& s : stars) {
        const Rgb c = starColor(s);
        StarInstance inst;
        inst.px = static_cast<float>(s.x);
        inst.py = static_cast<float>(s.y);
        inst.pz = static_cast<float>(s.z);
        inst.cr = c.r;
        inst.cg = c.g;
        inst.cb = c.b;
        inst.size = magnitudeToSize(s.mag, params);
        out.push_back(inst);
    }
    return out;
}

}  // namespace starlag::render
