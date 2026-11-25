// Replacement artwork for world tiles: gta2hd\tiles\<hash>.png beside gta2.exe,
// where <hash> is TileHash of the tile's 64x64 pixels as the game hands them
// over. Keyed by content rather than tile number, so a replacement follows its
// artwork through the game's remap table and across districts that share it.
// Any size works; ReRun's pack is the original upscaled 4x.
#pragma once

#include <d3d9.h>

#include <cstdint>

namespace gta2 {

// FNV-1a over the pixels, the same key UpdateTileTexture uses for frames.
uint64_t TileHash(const uint32_t* pixels);

// The replacement for this artwork as a mipmapped texture, or nullptr when the
// pack has none (or there is no pack). Counts hits and misses for the log.
IDirect3DTexture9* LoadHdTile(IDirect3DDevice9* device, uint64_t hash);

void HdTileStats(int* hits, int* misses);

}  // namespace gta2
