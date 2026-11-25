#include "raster_lighting.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "log.h"
#include "remix_api.h"
#include "remix_lights.h"

namespace gta2dx9 {
namespace {

bool g_active = false;

// Fixed-function D3D9 guarantees eight; wined3d offers exactly that.
constexpr DWORD kMaxLights = 8;

// d3ddll.dll falls off linearly, (radius - d) / radius. D3D9 can only do
// 1 / (a0 + a1 d + a2 d^2), so this is a quadratic that is 1 at the centre and
// about a tenth at the radius, where Range cuts it off. ponytail: close, not
// exact; a vertex shader could match the linear curve if it ever shows.
constexpr float kFalloff = 9.0f;

DWORD Grey(float level) {
    const DWORD v = static_cast<DWORD>(std::clamp(level, 0.0f, 1.0f) * 255.0f + 0.5f);
    return D3DCOLOR_XRGB(v, v, v);
}

}  // namespace

bool RasterLightingApply(IDirect3DDevice9* device, float cameraX, float cameraZ) {
    g_active = false;
    if (!device) return false;
    const float ambient = LightsWorldAmbient();
    if (RemixApiAvailable() || !LightsWorldAmbientKnown() || ambient >= 1.0f) return false;

    // The ambient floor, through the material's ambient reflectance of one.
    D3DMATERIAL9 material = {};
    material.Ambient = {1.0f, 1.0f, 1.0f, 1.0f};
    material.Diffuse = {0.0f, 0.0f, 0.0f, 1.0f};
    device->SetMaterial(&material);
    device->SetRenderState(D3DRS_AMBIENT, Grey(ambient));
    device->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
    device->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    device->SetRenderState(D3DRS_COLORVERTEX, FALSE);

    // The frame's lights nearest the middle of the view: the camera looks
    // straight down, so that is what is on screen.
    std::vector<const RemixLightDesc*> lights;
    for (const RemixLightDesc& light : LightsPending()) {
        if (light.radius > 0.0f && light.intensity > 0.0f) lights.push_back(&light);
    }
    const auto distance = [&](const RemixLightDesc* light) {
        const float dx = light->pos[0] - cameraX, dz = light->pos[2] - cameraZ;
        return dx * dx + dz * dz;
    };
    const size_t count = std::min<size_t>(lights.size(), kMaxLights);
    std::partial_sort(lights.begin(), lights.begin() + count, lights.end(),
                      [&](const RemixLightDesc* a, const RemixLightDesc* b) { return distance(a) < distance(b); });

    // All eight stay enabled, the unused ones black: wined3d builds a shader per
    // combination of enabled lights, and a count that changes with every lamp
    // the camera passes would keep it compiling new ones.
    for (DWORD i = 0; i < kMaxLights; ++i) {
        D3DLIGHT9 light = {};
        light.Type = D3DLIGHT_POINT;
        light.Attenuation0 = 1.0f;
        if (i < count) {
            // Ambient only: attenuated by distance, blind to the surface's angle,
            // which is how GTA2 lights a floor from a lamp at its own height.
            const RemixLightDesc& source = *lights[i];
            light.Ambient = {source.rgb[0] * source.intensity, source.rgb[1] * source.intensity,
                             source.rgb[2] * source.intensity, 1.0f};
            light.Position = {source.pos[0], source.pos[1], source.pos[2]};
            light.Range = source.radius;
            light.Attenuation2 = kFalloff / (source.radius * source.radius);
        } else {
            light.Position = {-1000.0f, -1000.0f, -1000.0f};
            light.Range = 1.0f;
        }
        device->SetLight(i, &light);
        device->LightEnable(i, TRUE);
    }

    static bool said = false;
    if (!said) {
        said = true;
        Log("raster lighting on: world ambient %.3f, %u of %u lights", ambient, static_cast<unsigned>(count),
            static_cast<unsigned>(lights.size()));
    }
    g_active = true;
    return true;
}

bool RasterLightingActive() { return g_active; }

DWORD RasterLightingColourAt(float x, float y, float z) {
    const float ambient = LightsWorldAmbient();
    float colour[3] = {ambient, ambient, ambient};
    for (const RemixLightDesc& light : LightsPending()) {
        if (light.radius <= 0.0f || light.intensity <= 0.0f) continue;
        const float dx = x - light.pos[0], dy = y - light.pos[1], dz = z - light.pos[2];
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 >= light.radius * light.radius) continue;
        const float reach = (light.radius - std::sqrt(d2)) / light.radius * light.intensity;
        for (int c = 0; c < 3; ++c) colour[c] += light.rgb[c] * reach;
    }
    const auto channel = [](float v) {
        return static_cast<DWORD>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    return D3DCOLOR_XRGB(channel(colour[0]), channel(colour[1]), channel(colour[2]));
}

void RasterLightingSetSpriteStage(IDirect3DDevice9* device) {
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
}

}  // namespace gta2dx9
