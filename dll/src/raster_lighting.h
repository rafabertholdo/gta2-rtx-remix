// GTA2's own lighting - dusk and night, its lamps and headlights - for the
// ordinary D3D9 path, when the game runs without RTX Remix (Wine on a Mac).
//
// Under Remix the scene is path traced and the game's lights become Remix
// lights (remix_lights.h); the rasterised world is drawn unlit and Remix does
// the rest. Without it that same unlit world is all there is, so every level
// looks like noon. This lights it the way d3ddll.dll did: an additive ambient
// floor plus the game's point lights with a falloff to their radius, ignoring
// the surface's angle (docs/lighting-analysis.md).
//
// The static world uses D3D9's fixed-function lights, whose ambient term does
// exactly that - attenuated, with no N.L - up to the eight D3D9 allows. Sprites
// are lit on the CPU instead, one colour each, with every light in reach.
#pragma once

#include <d3d9.h>

namespace gta2dx9 {

// Sets up the device's lights and ambient for this frame from the game's, and
// returns whether the world should be drawn lit. False under Remix, before the
// game has set an ambient, and in daylight (ambient 1), which GTA2 itself never
// lights; the world is then drawn exactly as before.
bool RasterLightingApply(IDirect3DDevice9* device, float cameraX, float cameraZ);

// Whether the last RasterLightingApply turned lighting on.
bool RasterLightingActive();

// The colour GTA2's own renderer would give a vertex at (x, y, z) in world-mesh
// coordinates - the ambient floor plus every light in reach, falling off
// linearly - for a sprite's D3DRS_TEXTUREFACTOR.
DWORD RasterLightingColourAt(float x, float y, float z);

// Stage 0 for a lit sprite: the texture times D3DRS_TEXTUREFACTOR, lighting off.
void RasterLightingSetSpriteStage(IDirect3DDevice9* device);

}  // namespace gta2dx9
