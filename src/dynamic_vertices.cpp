#include "dynamic_vertices.h"

#include <cstring>

namespace gta2 {

bool DynamicVertices::Draw(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, UINT primitives, const void* vertices,
                           UINT vertexCount, UINT stride) {
    if (!device || !vertices || vertexCount == 0 || stride == 0) return false;
    const UINT bytes = vertexCount * stride;
    if (device != device_ || bytes > capacity_) {
        Release();
        UINT capacity = 64 * 1024;
        while (capacity < bytes) capacity *= 2;
        if (FAILED(device->CreateVertexBuffer(capacity, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                                              &buffer_, nullptr))) {
            buffer_ = nullptr;
            return false;
        }
        device_ = device;
        capacity_ = capacity;
        cursor_ = capacity;  // the first lock discards
    }
    // The start has to be a whole vertex of this stride.
    UINT start = (cursor_ + stride - 1) / stride * stride;
    DWORD flags = D3DLOCK_NOOVERWRITE;
    if (start + bytes > capacity_) {
        start = 0;
        flags = D3DLOCK_DISCARD;
    }
    void* data = nullptr;
    if (FAILED(buffer_->Lock(start, bytes, &data, flags))) return false;
    memcpy(data, vertices, bytes);
    buffer_->Unlock();
    cursor_ = start + bytes;
    device->SetStreamSource(0, buffer_, 0, stride);
    return SUCCEEDED(device->DrawPrimitive(type, start / stride, primitives));
}

void DynamicVertices::Release() {
    if (buffer_) buffer_->Release();
    buffer_ = nullptr;
    device_ = nullptr;
    capacity_ = 0;
    cursor_ = 0;
}

}  // namespace gta2
