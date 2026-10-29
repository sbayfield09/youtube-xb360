/* Standalone XDK diagnostic SOURCE, uncompiled/unverified.
 * Add to a licensed Xbox 360 XDK C++ game project; see ../README.md.
 * It is not a browser, TLS implementation or YouTube player.
 * It does not link libXenon or touch XenonFin's media implementation. */
#include <xtl.h>
#include <stdio.h>
#include "../input.h"

static unsigned selection;
static void key(void *, unsigned code, int down) {
    char text[128];
    sprintf_s(text, sizeof text, "Leanback diagnostic: key=%u down=%d (NO DOM sink)\n", code, down);
    OutputDebugStringA(text);
    if (!down) return;
    if (code==32783 || code==32781) selection=(selection+1)%4;
    if (code==32782 || code==32780) selection=(selection+3)%4;
    if (code==32768) OutputDebugStringA("BLOCKED: no TLS/DNS/browser/media bridge implemented.\n");
}

void __cdecl main() {
    IDirect3D9 *d3d=Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return;
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp,sizeof pp);
    pp.BackBufferWidth=1280; pp.BackBufferHeight=720;
    pp.BackBufferFormat=D3DFMT_X8R8G8B8;
    pp.BackBufferCount=1; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
    IDirect3DDevice9 *device=0;
    if (FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,0,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device))) {
        d3d->Release(); return;
    }
    OutputDebugStringA("Leanback bring-up only: four focus markers; D-pad moves, A reports blockers. No web rendering.\n");
    lb_input input={0};
    for (;;) {
        XINPUT_STATE pad;
        ZeroMemory(&pad,sizeof pad);
        int connected=XInputGetState(0,&pad)==ERROR_SUCCESS;
        WORD b=pad.Gamepad.wButtons;
        unsigned normalized=((b&XINPUT_GAMEPAD_A)?1u:0u)|((b&XINPUT_GAMEPAD_B)?2u:0u)|
            ((b&XINPUT_GAMEPAD_DPAD_UP)?4u:0u)|((b&XINPUT_GAMEPAD_DPAD_DOWN)?8u:0u)|
            ((b&XINPUT_GAMEPAD_DPAD_LEFT)?16u:0u)|((b&XINPUT_GAMEPAD_DPAD_RIGHT)?32u:0u);
        lb_input_poll(&input,normalized,connected,key,0);
        device->Clear(0,0,D3DCLEAR_TARGET,D3DCOLOR_XRGB(48,0,0),1,0);
        for (unsigned i=0;i<4;++i) {
            D3DRECT rect={100+(LONG)i*280,300,340+(LONG)i*280,420};
            device->Clear(1,&rect,D3DCLEAR_TARGET,i==selection?D3DCOLOR_XRGB(255,180,0):D3DCOLOR_XRGB(80,80,80),1,0);
        }
        device->Present(0,0,0,0);
    }
}
