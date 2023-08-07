#include <windows.h>
#include <windowsx.h>
#include <intrin.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <linmath.h>

/* config */
#define CONFIG_WINDOW_TITLE "hello, world"
#define CONFIG_WINDOW_WIDTH 800
#define CONFIG_WINDOW_HEIGHT 600
#define CONFIG_WINDOW_X CW_USEDEFAULT
#define CONFIG_WINDOW_Y CW_USEDEFAULT
#define CONFIG_WINDOW_STYLE WS_OVERLAPPEDWINDOW
#define CONFIG_CAMERA_FOV 90
#define CONFIG_CAMERA_NEAR 0.01f
#define CONFIG_CAMERA_FAR 100.0f
#define CONFIG_CAMERA_X 0
#define CONFIG_CAMERA_Y 0
#define CONFIG_CAMERA_Z 0
#define CONFIG_CAMERA_PITCH 0
#define CONFIG_CAMERA_YAW 0
#define CONFIG_CAMERA_ROLL 0
#define CONFIG_CAMERA_W 1
#define CONFIG_CAMERA_H 1
#define CONFIG_CAMERA_D 1
#define CONFIG_MODEL_X 0
#define CONFIG_MODEL_Y 0
#define CONFIG_MODEL_Z -5
#define CONFIG_MODEL_PITCH 0
#define CONFIG_MODEL_YAW 0
#define CONFIG_MODEL_ROLL 0
#define CONFIG_MODEL_W 1
#define CONFIG_MODEL_H 1
#define CONFIG_MODEL_D 1

/* macros */
#define ASSERT(s)                                                       \
{                                                                       \
    if (s) {} else {                                                    \
        MessageBoxAF("Error", __FILE__":%llu\n\"%s\"", __LINE__, #s);   \
        __debugbreak();                                                 \
    }                                                                   \
}

#define ASSERT_DO(s, statement)                                         \
{                                                                       \
    if (s) {} else {                                                    \
        statement;                                                      \
        MessageBoxAF("Error", __FILE__":%llu\n\"%s\"", __LINE__, #s);   \
        __debugbreak();                                                 \
    }                                                                   \
}

#define WIN32_ASSERT(s)                                                                                     \
{                                                                                                           \
    if (s) {} else {                                                                                        \
        DWORD err = GetLastError();                                                                         \
        void * buf;                                                                                         \
        FormatMessageA(                                                                                     \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    \
            NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buf, 0, NULL                     \
        );                                                                                                  \
        MessageBoxAF("Error", __FILE__":%llu;\nerror: 0x%08X: %s\n%s", __LINE__, err, buf, #s);             \
        LocalFree(buf);                                                                                     \
        __debugbreak();                                                                                     \
    }                                                                                                       \
}

#define WIN32_ASSERT_DO(s, statement)                                                                       \
{                                                                                                           \
    if (s) {} else {                                                                                        \
        statement;                                                                                          \
        DWORD err = GetLastError();                                                                         \
        void * buf;                                                                                         \
        FormatMessageA(                                                                                     \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    \
            NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buf, 0, NULL                     \
        );                                                                                                  \
        MessageBoxAF("Error", __FILE__":%llu;\nerror: 0x%08X: %s\n%s", __LINE__, err, buf, #s);             \
        LocalFree(buf);                                                                                     \
        __debugbreak();                                                                                     \
    }                                                                                                       \
}

#define D3D11_ASSERT(hr)                                                                \
{                                                                                       \
    HRESULT r = hr;                                                                     \
    if (SUCCEEDED(r)) {} else {                                                         \
        MessageBoxAF("D3D11 Error", __FILE__":%llu\n\"%s\"\n0x%08lX", __LINE__, #hr, r);\
        __debugbreak();                                                                 \
    }                                                                                   \
}

#define D3D11_ASSERT_DO(hr, statement)                                                  \
{                                                                                       \
    HRESULT r = hr;                                                                     \
    if (SUCCEEDED(r)) {} else {                                                         \
        statement;                                                                      \
        MessageBoxAF("D3D11 Error", __FILE__":%llu\n\"%s\"\n0x%08lx", __LINE__, #hr, r);\
        __debugbreak();                                                                 \
    }                                                                                   \
}

#define STR(x) #x

static void MessageBoxAF(const char * title, const char * fmt, ...);
static LRESULT APIENTRY window_proc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam);

/* state */
struct {
    BOOL running;
    BOOL mouse_lock;
    BOOL mouse_hide;
    BOOL active;
    int mouse_x;
    int mouse_y;
    int delta_mouse_x;
    int delta_mouse_y;
    int scroll_delta;
    unsigned int window_width;
    unsigned int window_height;
    struct {
        vec3 pos;
        vec3 rot;
        vec3 scale;
        float fov;
        float near_plane;
        float far_plane;
    } camera;
    struct {
        vec3 pos;
        vec3 rot;
        vec3 scale;
    } model;
    float delta;
    unsigned char keys[256];
    HWND window;
} state;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int showcmd) {
    /* applying configs */
    state.running = TRUE;
    state.mouse_lock = TRUE;
    state.mouse_hide = TRUE;
    state.active = TRUE;
    state.mouse_x = 0;
    state.mouse_y = 0;
    state.delta_mouse_x = 0;
    state.delta_mouse_y = 0;
    state.scroll_delta = 0;
    state.window_width = CONFIG_WINDOW_WIDTH;
    state.window_height = CONFIG_WINDOW_HEIGHT;
    state.model.pos[0] = CONFIG_MODEL_X; state.model.pos[1] = CONFIG_MODEL_Y; state.model.pos[2] = CONFIG_MODEL_Z;
    state.model.rot[0] = CONFIG_MODEL_PITCH; state.model.rot[1] = CONFIG_MODEL_YAW; state.model.rot[2] = CONFIG_MODEL_ROLL;
    state.model.scale[0] = CONFIG_MODEL_W; state.model.scale[1] = CONFIG_MODEL_H; state.model.scale[2] = CONFIG_MODEL_D;
    state.camera.pos[0] = CONFIG_CAMERA_X; state.camera.pos[1] = CONFIG_CAMERA_Y; state.camera.pos[2] = CONFIG_CAMERA_Z;
    state.camera.rot[0] = CONFIG_CAMERA_PITCH; state.camera.rot[1] = CONFIG_CAMERA_YAW; state.camera.rot[2] = CONFIG_CAMERA_ROLL;
    state.camera.scale[0] = CONFIG_CAMERA_W; state.camera.scale[1] = CONFIG_CAMERA_H; state.camera.scale[2] = CONFIG_CAMERA_D;
    state.camera.fov = CONFIG_CAMERA_FOV;
    state.camera.near_plane = CONFIG_CAMERA_NEAR;
    state.camera.far_plane = CONFIG_CAMERA_FAR;
    state.window = NULL;

    /* win32 window */
    WNDCLASSEXA window_class;
    ZeroMemory(&window_class, sizeof(window_class));

    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = (WNDPROC) window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIcon(NULL, IDC_ARROW);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = GetStockObject(0);
    window_class.lpszMenuName = "menu name";
    window_class.lpszClassName = "Window Class";
    window_class.hIconSm = LoadImageA(instance, MAKEINTRESOURCEA(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    WIN32_ASSERT(RegisterClassExA(&window_class) != 0);

    HWND window;
    {
        RECT rect = {
            .top = 0,
            .left = 0,
            .right = state.window_width,
            .bottom = state.window_height,
        };
        WIN32_ASSERT(AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0) != 0);
        WIN32_ASSERT(window = CreateWindowExA(0, window_class.lpszClassName, CONFIG_WINDOW_TITLE, CONFIG_WINDOW_STYLE, CONFIG_WINDOW_X, CONFIG_WINDOW_Y, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, window_class.hInstance, NULL));
    }
    state.window = window;

    /* Direct3D 11 init */
    ID3D11Device * dev;
    ID3D11DeviceContext * devcontext;

    {
        D3D_FEATURE_LEVEL features_levels[1] = { D3D_FEATURE_LEVEL_11_0 };

        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D11_ASSERT(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, features_levels, 1, D3D11_SDK_VERSION, &dev, NULL, &devcontext) == S_OK);
    }

#ifdef _DEBUG
    {
        ID3D11InfoQueue * info;
        D3D11_ASSERT(dev->lpVtbl->QueryInterface(dev, &IID_ID3D11InfoQueue, &info));
        D3D11_ASSERT(info->lpVtbl->SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE));
        D3D11_ASSERT(info->lpVtbl->SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE));
        info->lpVtbl->Release(info);
    }

    HMODULE dxgidebug = LoadLibraryA("dxgidebug.dll");
    if (dxgidebug != NULL) {
        HRESULT (WINAPI *dxgiGetDebugInterface)(REFIID riid, void ** ppDebug);
        *(FARPROC *) &dxgiGetDebugInterface = GetProcAddress(dxgidebug, "DXGIGetDebugInterface");

        IDXGIInfoQueue * dxgiinfo;
        D3D11_ASSERT(dxgiGetDebugInterface(&IID_IDXGIInfoQueue, &dxgiinfo));
        D3D11_ASSERT(dxgiinfo->lpVtbl->SetBreakOnSeverity(dxgiinfo, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
        D3D11_ASSERT(dxgiinfo->lpVtbl->SetBreakOnSeverity(dxgiinfo, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
        dxgiinfo->lpVtbl->Release(dxgiinfo);
    }
#endif

    IDXGISwapChain1 * swapchain;
    {
        IDXGIDevice * dxgidev;
        D3D11_ASSERT(dev->lpVtbl->QueryInterface(dev, &IID_IDXGIDevice, &dxgidev));

        IDXGIAdapter * dxgiadapter;
        D3D11_ASSERT(dxgidev->lpVtbl->GetAdapter(dxgidev, &dxgiadapter));

        IDXGIFactory2 * factory;
        D3D11_ASSERT(dxgiadapter->lpVtbl->GetParent(dxgiadapter, &IID_IDXGIFactory2, &factory));

        DXGI_SWAP_CHAIN_DESC1 desc;
        SecureZeroMemory(&desc, sizeof(desc));
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Width = state.window_width;
        desc.Height = state.window_height;
        
        D3D11_ASSERT(factory->lpVtbl->CreateSwapChainForHwnd(factory, dev, window, &desc, NULL, NULL, &swapchain));
        D3D11_ASSERT(factory->lpVtbl->MakeWindowAssociation(factory, window, DXGI_MWA_NO_ALT_ENTER));
        factory->lpVtbl->Release(factory);
        dxgiadapter->lpVtbl->Release(dxgiadapter);
        dxgidev->lpVtbl->Release(dxgidev);
    }

    typedef struct Vertex {
        float pos[3];
        float uv[2];
        float color[3];
    } vertex_t;

    ID3D11Buffer * vbo;
    {
        vertex_t data[3] = {
            { { 0.0f, 1.0f, 0.0f }, { 0.5f, 1 }, { 1, 1, 0 } },
            { { 1.0f, 0.0f, 0.0f }, { 1, 0 }, { 0, 1, 1 } },
            { { -1.0f, 0.0f, 0.0f }, { 0, 0 }, { 1, 0, 1 } },
        };

        D3D11_BUFFER_DESC desc = {
            .ByteWidth = sizeof(data),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA init = { .pSysMem = data };
        dev->lpVtbl->CreateBuffer(dev, &desc, &init, &vbo);
    }

    ID3D11InputLayout * layout;
    ID3D11VertexShader * vertex_shader;
    ID3D11PixelShader * pixel_shader;

    {
        D3D11_INPUT_ELEMENT_DESC desc[3] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(vertex_t, uv), D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(vertex_t, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        char * hlsl;
        LARGE_INTEGER size;
        {
            HANDLE file = CreateFileA("./shaders.hlsl", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            WIN32_ASSERT_DO(file != INVALID_HANDLE_VALUE, MessageBoxAF("File Read Error", "Failed to open file %s", "./shaders.hlsl"));
            size.QuadPart = 0;
            WIN32_ASSERT(GetFileSizeEx(file, &size) != 0);

            hlsl = _malloca(size.QuadPart + 1);
            ASSERT(hlsl != NULL);
            hlsl[size.QuadPart] = '\0';
            DWORD read = 0;
            WIN32_ASSERT(ReadFile(file, hlsl, size.QuadPart, &read, NULL) != 0);

            CloseHandle(file);
        }

        UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ID3DBlob * err;
        ID3DBlob * vblob;
        ID3DBlob * pblob;

        D3D11_ASSERT_DO(D3DCompile(hlsl, size.QuadPart, "shaders.hlsl", NULL, NULL, "vs", "vs_5_0", flags, 0, &vblob, &err),
            {
                ASSERT(err != NULL);
                const char * message = err->lpVtbl->GetBufferPointer(err);
                OutputDebugStringA(message);
                MessageBoxAF("D3DCompile Failed!", "%s", message);
            }
        );

        D3D11_ASSERT_DO(D3DCompile(hlsl, size.QuadPart, "shaders.hlsl", NULL, NULL, "ps", "ps_5_0", flags, 0, &pblob, &err),
            {
                ASSERT(err != NULL);
                const char * message = err->lpVtbl->GetBufferPointer(err);
                OutputDebugStringA(message);
                MessageBoxAF("D3DCompile Failed!", "%s", message);
            }
        );

        D3D11_ASSERT(dev->lpVtbl->CreateVertexShader(dev, vblob->lpVtbl->GetBufferPointer(vblob), vblob->lpVtbl->GetBufferSize(vblob), NULL, &vertex_shader));
        D3D11_ASSERT(dev->lpVtbl->CreatePixelShader(dev, pblob->lpVtbl->GetBufferPointer(pblob), pblob->lpVtbl->GetBufferSize(pblob), NULL, &pixel_shader));
        D3D11_ASSERT(dev->lpVtbl->CreateInputLayout(dev, desc, 3, vblob->lpVtbl->GetBufferPointer(vblob), vblob->lpVtbl->GetBufferSize(vblob), &layout));

        vblob->lpVtbl->Release(vblob);
        pblob->lpVtbl->Release(pblob);
    }

    ID3D11Buffer * ubuffer;
    {
        D3D11_BUFFER_DESC desc = {
            .ByteWidth = 4 * 4 * sizeof(float),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        D3D11_ASSERT(dev->lpVtbl->CreateBuffer(dev, &desc, NULL, &ubuffer));
    }

    ID3D11ShaderResourceView * texture_view;
    {
        unsigned int pixels[4] = {
            0xFFFFFFFF, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFFFFFFFF,
        };

        UINT width = 2;
        UINT height = 2;

        D3D11_TEXTURE2D_DESC desc = {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc.Count = 1,
            .SampleDesc.Quality = 0,
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data = {
            .pSysMem = pixels,
            .SysMemPitch = width * sizeof(unsigned int),
        };

        ID3D11Texture2D * texture;
        D3D11_ASSERT(dev->lpVtbl->CreateTexture2D(dev, &desc, &data, &texture));
        D3D11_ASSERT(dev->lpVtbl->CreateShaderResourceView(dev, (ID3D11Resource *) texture, NULL, &texture_view));
        texture->lpVtbl->Release(texture);
    }

    ID3D11SamplerState * sampler;
    {
        D3D11_SAMPLER_DESC desc = {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        };

        D3D11_ASSERT(dev->lpVtbl->CreateSamplerState(dev, &desc, &sampler));
    }

    ID3D11BlendState * blendstate;
    {
        D3D11_BLEND_DESC desc = {
            .RenderTarget[0] = {
                .BlendEnable = TRUE,
                .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOp = D3D11_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
                .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
            },
        };

        D3D11_ASSERT(dev->lpVtbl->CreateBlendState(dev, &desc, &blendstate));
    }

    ID3D11RasterizerState * rasterizerstate;
    {
        D3D11_RASTERIZER_DESC desc = {
            .FillMode = D3D11_FILL_SOLID,
            // TODO: back-face culling
            .CullMode = D3D11_CULL_NONE,
        };
        D3D11_ASSERT(dev->lpVtbl->CreateRasterizerState(dev, &desc, &rasterizerstate));
    }

    ID3D11RenderTargetView * render_target = NULL;

    /* Window show-y */

    ShowWindow(window, showcmd);
    UpdateWindow(window);

    /* game loop */
    LARGE_INTEGER freq, c1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c1);

    ID3D11Texture2D * backbuffer;
    D3D11_ASSERT(swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, &backbuffer));
    D3D11_ASSERT(dev->lpVtbl->CreateRenderTargetView(dev, (ID3D11Resource *) backbuffer, NULL, &render_target));
    backbuffer->lpVtbl->Release(backbuffer);

    DWORD prev_w = state.window_width;
    DWORD prev_h = state.window_height;
    float angle = 0;
    while (state.running) {
        if (state.mouse_hide) {
            ShowCursor(FALSE);
        } else {
            ShowCursor(TRUE);
        }

        /* update */
        {
            vec3 up = { 0, 1, 0 };
            vec3 forward = { sinf(state.camera.rot[1] * (3.141592f / 180.0f)), 0, cosf(state.camera.rot[1] * (3.141592f / 180.0f)) };
            vec3 right;
            vec3_mul_cross(right, up, forward);
            float base_speed = 5.0f;
            float scroll_speed = 30.0f;
            float slow_speed = 1.0f;
            float fast_speed = 20.0f;
            float speed = base_speed;
            float base_sens = 150.0f;
            float slow_sens = 80.0f;
            float sens = base_sens;
            float mouse_sens = 15.0f;

            if (state.keys[VK_CONTROL]) {
                speed = slow_speed;
                sens = slow_sens;
            }
            if (state.keys[VK_SHIFT]) {
                speed = fast_speed;
            }
            if (state.keys['W']) {
                vec3_add(state.camera.pos, state.camera.pos, (vec3) { forward[0] * speed * state.delta, forward[1] * speed * state.delta, forward[2] * speed * state.delta });
            }
            if (state.keys['S']) {
                vec3_sub(state.camera.pos, state.camera.pos, (vec3) { forward[0] * speed * state.delta, forward[1] * speed * state.delta, forward[2] * speed * state.delta });
            }
            if (state.keys['D']) {
                vec3_add(state.camera.pos, state.camera.pos, (vec3) { right[0] * speed * state.delta, right[1] * speed * state.delta, right[2] * speed * state.delta });
            }
            if (state.keys['A']) {
                vec3_sub(state.camera.pos, state.camera.pos, (vec3) { right[0] * speed * state.delta, right[1] * speed * state.delta, right[2] * speed * state.delta });
            }
            if (state.keys['E']) {
                vec3_add(state.camera.pos, state.camera.pos, (vec3) { up[0] * speed * state.delta, up[1] * speed * state.delta, up[2] * speed * state.delta });
            }
            if (state.keys['Q']) {
                vec3_sub(state.camera.pos, state.camera.pos, (vec3) { up[0] * speed * state.delta, up[1] * speed * state.delta, up[2] * speed * state.delta });
            }
            if (state.keys[VK_RIGHT]) {
                state.camera.rot[1] += state.delta * sens;
            }
            if (state.keys[VK_LEFT]) {
                state.camera.rot[1] -= state.delta * sens;
            }
            if (state.keys[VK_UP]) {
                state.camera.rot[0] += state.delta * sens;
            }
            if (state.keys[VK_DOWN]) {
                state.camera.rot[0] -= state.delta * sens;
            }

            state.camera.rot[0] += -state.delta_mouse_y * mouse_sens * state.delta;
            state.camera.rot[1] += state.delta_mouse_x * mouse_sens * state.delta;
        }

        /* render */
        RECT rect;
        GetClientRect(window, &rect);
        state.window_width = rect.right - rect.left;
        state.window_height = rect.bottom - rect.top;

        if (state.window_width != prev_w || state.window_height != prev_h) {
            if (render_target != NULL) {
                devcontext->lpVtbl->ClearState(devcontext);
                render_target->lpVtbl->Release(render_target);
                render_target = NULL;
            }

            D3D11_ASSERT(swapchain->lpVtbl->ResizeBuffers(swapchain, 0, state.window_width, state.window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
            ID3D11Texture2D * backbuffer;
            D3D11_ASSERT(swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, &backbuffer));
            D3D11_ASSERT(dev->lpVtbl->CreateRenderTargetView(dev, (ID3D11Resource *) backbuffer, NULL, &render_target));
            backbuffer->lpVtbl->Release(backbuffer);
        }

        prev_w = state.window_width;
        prev_h = state.window_height;

        if (render_target != NULL) {
            LARGE_INTEGER c2;
            QueryPerformanceCounter(&c2);
            state.delta = (float) ((c2.QuadPart - c1.QuadPart) / (double) freq.QuadPart);
            c1 = c2;
            D3D11_VIEWPORT viewport = {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = (float) state.window_width,
                .Height = (float) state.window_height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };

            float color[4] = { 0.06f, 0.03f, 0.12f, 1.0f };
            devcontext->lpVtbl->ClearRenderTargetView(devcontext, render_target, color);

            {
                float ratio = state.window_width / (float) state.window_height;
                mat4x4 m, v, p, mvp;
                mat4x4_identity(m);
                mat4x4_identity(v);
                mat4x4_identity(p);
                mat4x4_identity(mvp);

                /* model to world */
                mat4x4_scale_aniso(m, m, state.model.scale[0], state.model.scale[1], state.model.scale[2]);
                mat4x4_rotate_X(m, m, state.model.rot[0] * (3.141592f / 180.0f));
                mat4x4_rotate_Y(m, m, state.model.rot[1] * (3.141592f / 180.0f));
                mat4x4_rotate_Z(m, m, state.model.rot[2] * (3.141592f / 180.0f));
                mat4x4_translate(m, state.model.pos[0], state.model.pos[1], state.model.pos[2]);

                /* world to camera */
                mat4x4_translate(v, -state.camera.pos[0], -state.camera.pos[1], state.camera.pos[2]);

                /* projection matrix */
                mat4x4_perspective(p, state.camera.fov * (3.141592f / 180.0f), ratio, state.camera.near_plane, state.camera.far_plane);
                mat4x4_scale_aniso(p, p, state.camera.scale[0], state.camera.scale[1], state.camera.scale[2]);
                mat4x4_rotate_X(p, p, -state.camera.rot[0] * (3.141592f / 180.0f));
                mat4x4_rotate_Y(p, p, state.camera.rot[1] * (3.141592f / 180.0f));
                mat4x4_rotate_Z(p, p, state.camera.rot[2] * (3.141592f / 180.0f));

                mat4x4_mul(mvp, mvp, p);
                mat4x4_mul(mvp, mvp, v);
                mat4x4_mul(mvp, mvp, m);

                D3D11_MAPPED_SUBRESOURCE mapped;
                D3D11_ASSERT(devcontext->lpVtbl->Map(devcontext, (ID3D11Resource *) ubuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
                CopyMemory(mapped.pData, mvp, sizeof(mvp));
                devcontext->lpVtbl->Unmap(devcontext, (ID3D11Resource *) ubuffer, 0);
            }

            devcontext->lpVtbl->IASetInputLayout(devcontext, layout);
            devcontext->lpVtbl->IASetPrimitiveTopology(devcontext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            UINT stride = sizeof(vertex_t);
            UINT offset = 0;
            devcontext->lpVtbl->IASetVertexBuffers(devcontext, 0, 1, &vbo, &stride, &offset);

            devcontext->lpVtbl->VSSetConstantBuffers(devcontext, 0, 1, &ubuffer);
            devcontext->lpVtbl->VSSetShader(devcontext, vertex_shader, NULL, 0);

            devcontext->lpVtbl->RSSetViewports(devcontext, 1, &viewport);
            devcontext->lpVtbl->RSSetState(devcontext, rasterizerstate);

            devcontext->lpVtbl->PSSetSamplers(devcontext, 0, 1, &sampler);
            devcontext->lpVtbl->PSSetShaderResources(devcontext, 0, 1, &texture_view);
            devcontext->lpVtbl->PSSetShader(devcontext, pixel_shader, NULL, 0);

            devcontext->lpVtbl->OMSetBlendState(devcontext, blendstate, NULL, ~0U);
            devcontext->lpVtbl->OMSetRenderTargets(devcontext, 1, &render_target, NULL);

            devcontext->lpVtbl->Draw(devcontext, 3, 0);
        }

        state.scroll_delta = 0;
        state.delta_mouse_x = 0;
        state.delta_mouse_y = 0;

        D3D11_ASSERT(swapchain->lpVtbl->Present(swapchain, 1, 0));
        MSG msg;
        if (PeekMessageA(&msg, window, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return 0;
}

static LRESULT APIENTRY window_proc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam) {
    unsigned char key;
    switch (msg) {
        case WM_QUIT:
            state.running = FALSE;
            PostQuitMessage(69);
            break;
        case WM_DESTROY:
            state.running = FALSE;
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:;
            key = LOWORD(wparam);
            if (key == VK_ESCAPE) {
                state.mouse_lock = !state.mouse_lock;
                state.mouse_hide = state.mouse_lock;
                ShowCursor(state.mouse_hide);
            }
            if (key == VK_F1) {
                MessageBoxAF("state",
                    "running = %s\n"
                    "mouse_x = %i\n"
                    "mouse_y = %i\n"
                    "delta_mouse_x = %i\n"
                    "delta_mouse_y = %i\n"
                    "scroll_delta = %i\n"
                    "window_width = %u\n"
                    "window_height = %u\n"
                    "camera = {\n"
                    "    pos = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "    rot = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "    scale = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "    fov = %f\n"
                    "    near_plane = %f\n"
                    "    far_plane = %f\n"
                    "}\n"
                    "model = {\n"
                    "    pos = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "    rot = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "    scale = {\n"
                    "        x = %f\n"
                    "        y = %f\n"
                    "        z = %f\n"
                    "    }\n"
                    "}\n"
                    "delta = %f\n",
                    (state.running == 1) ? "TRUE" : "FALSE",
                    state.mouse_x,
                    state.mouse_y,
                    state.delta_mouse_x,
                    state.delta_mouse_y,
                    state.scroll_delta,
                    state.window_width,
                    state.window_height,
                    /* camera { */
                        /* pos { */
                            state.camera.pos[0],
                            state.camera.pos[1],
                            state.camera.pos[2],
                        /* } */
                        /* rot { */
                            state.camera.rot[0],
                            state.camera.rot[1],
                            state.camera.rot[2],
                        /* } */
                        /* scale { */
                            state.camera.scale[0],
                            state.camera.scale[1],
                            state.camera.scale[2],
                        /* } */
                        state.camera.fov,
                        state.camera.near_plane,
                        state.camera.far_plane,
                    /* } */
                    /* model { */
                        /* pos { */
                            state.model.pos[0],
                            state.model.pos[1],
                            state.model.pos[2],
                        /* } */
                        /* rot { */
                            state.model.rot[0],
                            state.model.rot[1],
                            state.model.rot[2],
                        /* } */
                        /* scale { */
                            state.model.scale[0],
                            state.model.scale[1],
                            state.model.scale[2],
                        /* } */
                    /* } */
                    state.delta
                );
            }
            if (key < 256) {
                state.keys[key] = 1;
            }
            break;
        case WM_SYSKEYUP:
        case WM_KEYUP:;
            key = LOWORD(wparam);
            if (key < 256) {
                state.keys[key] = 0;
            }
            break;
        case WM_MBUTTONDOWN:
            MessageBoxAF("info", "position (%f, %f, %f) rotation (%f, %f, %f) scale (%f, %f, %f)", state.camera.pos[0], state.camera.pos[1], state.camera.pos[2], state.camera.rot[0], state.camera.rot[1], state.camera.rot[2], state.camera.scale[0], state.camera.scale[1], state.camera.scale[2]);
            break;
        case WM_LBUTTONDOWN:
            break;
        case WM_RBUTTONDOWN:
            break;
        case WM_MOUSEWHEEL:;
            int scroll = GET_WHEEL_DELTA_WPARAM(wparam);
            if (scroll < 0) {
                state.scroll_delta = -1;
            } else if (scroll > 0) {
                state.scroll_delta = 1;
            }
            return msg;
        case WM_MOUSEMOVE:
            if (state.mouse_lock && state.active) {
                RECT rect;
                GetWindowRect(state.window, &rect);
                rect.top += 32;
                rect.left += 8;
                rect.right -= 8;
                rect.bottom -= 8;
                ClipCursor(&rect);
                SetCursorPos(rect.right - state.window_width / 2, rect.bottom - state.window_height / 2);
            } else {
                ClipCursor(NULL);
            }
            int new_mouse_x = GET_X_LPARAM(lparam);
            int new_mouse_y = GET_Y_LPARAM(lparam);
            state.delta_mouse_x = new_mouse_x - state.mouse_x;
            state.delta_mouse_y = new_mouse_y - state.mouse_y;
            state.mouse_x = new_mouse_x;
            state.mouse_y = new_mouse_y;
            if (state.mouse_lock && state.active) {
                state.mouse_x = state.window_width / 2;
                state.mouse_y = state.window_height / 2;
            }
            return msg;
        case WM_ACTIVATE:
            state.active = wparam > 0 ? TRUE : FALSE;
            break;
    }

    return DefWindowProcA(win, msg, wparam, lparam);
}

static void MessageBoxAF(const char * title, const char * fmt, ...) {
    va_list list;
    va_start(list, fmt);

    size_t length = strlen(fmt);
    length *= 1.5f;
    length += 512;
    char * buffer = _malloca(length);
    vsnprintf(buffer, length, fmt, list);
    va_end(list);

    MessageBoxA(NULL, buffer, title, MB_OK);
}