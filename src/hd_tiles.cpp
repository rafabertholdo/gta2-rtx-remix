#include "hd_tiles.h"

#include <wincodec.h>
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace gta2 {
namespace {

int g_hits = 0;
int g_misses = 0;
int g_state = 0;   // 0 unknown, 1 pack present, -1 none
char g_dir[MAX_PATH] = "";
IWICImagingFactory* g_factory = nullptr;

bool Ready() {
    if (g_state) return g_state > 0;
    GetModuleFileNameA(nullptr, g_dir, sizeof(g_dir));
    char* slash = strrchr(g_dir, '\\');
    // Off when gta2dx9.ini says hd_tiles=0 (ReRun's HD textures switch).
    char ini[MAX_PATH];
    strcpy(ini, g_dir);
    if (slash) strcpy(ini + (slash - g_dir) + 1, "gta2dx9.ini");
    if (!GetPrivateProfileIntA("renderer", "hd_tiles", 1, ini)) {
        g_state = -1;
        return false;
    }
    if (slash) strcpy(slash + 1, "gta2hd\\tiles\\");
    const DWORD attributes = GetFileAttributesA(g_dir);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        g_state = -1;
        return false;
    }
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&g_factory)))) {
        g_state = -1;
        return false;
    }
    g_state = 1;
    return true;
}

// The image as BGRA rows, or false.
bool Decode(const char* path, std::vector<uint32_t>* pixels, UINT* width, UINT* height) {
    wchar_t wide[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wide, MAX_PATH);
    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(g_factory->CreateDecoderFromFilename(wide, nullptr, GENERIC_READ,
                                                    WICDecodeMetadataCacheOnDemand, &decoder))) {
        return false;
    }
    bool ok = false;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    if (SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(g_factory->CreateFormatConverter(&converter))
        && SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                           nullptr, 0.0, WICBitmapPaletteTypeCustom))
        && SUCCEEDED(converter->GetSize(width, height)) && *width && *height) {
        pixels->resize(static_cast<size_t>(*width) * *height);
        ok = SUCCEEDED(converter->CopyPixels(nullptr, *width * 4, *width * *height * 4,
                                             reinterpret_cast<BYTE*>(pixels->data())));
    }
    if (converter) converter->Release();
    if (frame) frame->Release();
    decoder->Release();
    return ok;
}

}  // namespace

uint64_t TileHash(const uint32_t* pixels) {
    uint64_t key = 0xCBF29CE484222325ULL;
    for (size_t i = 0; i < 64 * 64; ++i) {
        key ^= pixels[i];
        key *= 0x100000001B3ULL;
    }
    return key;
}

IDirect3DTexture9* LoadHdTile(IDirect3DDevice9* device, uint64_t hash) {
    if (!device || !Ready()) return nullptr;
    char path[MAX_PATH];
    _snprintf(path, sizeof(path) - 1, "%s%016llX.png", g_dir, static_cast<unsigned long long>(hash));
    path[sizeof(path) - 1] = '\0';
    std::vector<uint32_t> pixels;
    UINT width = 0, height = 0;
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES || !Decode(path, &pixels, &width, &height)) {
        ++g_misses;
        return nullptr;
    }
    // Mipmapped: an upscaled tile is drawn smaller than its size, and without
    // mips it shimmers as the camera moves.
    IDirect3DTexture9* texture = nullptr;
    if (FAILED(device->CreateTexture(width, height, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8,
                                     D3DPOOL_MANAGED, &texture, nullptr))) {
        ++g_misses;
        return nullptr;
    }
    D3DLOCKED_RECT rect;
    if (FAILED(texture->LockRect(0, &rect, nullptr, 0))) {
        texture->Release();
        ++g_misses;
        return nullptr;
    }
    for (UINT y = 0; y < height; ++y) {
        memcpy(static_cast<uint8_t*>(rect.pBits) + y * rect.Pitch, &pixels[static_cast<size_t>(y) * width],
               width * 4);
    }
    texture->UnlockRect(0);
    texture->GenerateMipSubLevels();
    ++g_hits;
    return texture;
}

void HdTileStats(int* hits, int* misses) {
    *hits = g_hits;
    *misses = g_misses;
}

}  // namespace gta2
