// DrawPrimitiveUP without the UP: vertices appended to a dynamic vertex buffer
// this owns, NOOVERWRITE while there is room and DISCARD when it wraps, which
// hands wined3d a fresh buffer instead of making it wait for the GPU.
//
// Under Wine on a Mac, wined3d's own streaming of UP data - the ring it copies
// every *PrimitiveUP call into - stopped for good a few minutes into dusk play:
// the next UP draw waited forever, from whichever pass reached one first.
#pragma once

#include <d3d9.h>

namespace gta2 {

class DynamicVertices {
public:
    ~DynamicVertices() { Release(); }

    // As DrawPrimitiveUP. False when the buffer can't be made or filled.
    bool Draw(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, UINT primitives, const void* vertices,
              UINT vertexCount, UINT stride);

    // Before the device goes.
    void Release();

private:
    IDirect3DVertexBuffer9* buffer_ = nullptr;
    IDirect3DDevice9* device_ = nullptr;
    UINT capacity_ = 0;  // bytes
    UINT cursor_ = 0;    // bytes
};

}  // namespace gta2
