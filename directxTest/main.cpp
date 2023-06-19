#include "main.h"
#include "DxException.h"
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>
#include "CommonFunc.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <windowsx.h>
#include <comdef.h>

using namespace Microsoft::WRL;

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")


ComPtr<ID3D12Device> d3dDevice;
ComPtr<ID3D12Fence> fence;
ComPtr<IDXGIFactory4> dxgiFactory;
ComPtr<ID3D12GraphicsCommandList> cmdList;
ComPtr<ID3D12CommandAllocator> cmdAllocator;
ComPtr<ID3D12Resource> swapChainBuffer[2];

// ����һ����Դ��һ���ѣ�������Դ�ύ�����У������ģ�������ύ��GPU�Դ��У�
ComPtr<ID3D12Resource> depthStencilBuffer;
ComPtr<ID3D12CommandQueue> cmdQueue;
ComPtr<ID3D12DescriptorHeap> rtvHeap;
ComPtr<ID3D12DescriptorHeap> dsvHeap;
ComPtr<IDXGISwapChain> swapChain;
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

D3D12_VIEWPORT viewPort;
D3D12_RECT scissorRect;
UINT rtvDescriptiorSize = 0;
UINT dsvDescriptionSize = 0;
UINT cbv_srv_uavDescriptionSize = 0;
UINT mCurrentBackBuffer = 0;

int mCurrentFence = 0;

HWND mhMainWnd = 0;	//ĳ�����ڵľ����ShowWindow��UpdateWindow������Ҫ���ô˾��
//���ڹ��̺���������,HWND�������ھ��
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

#if 0

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    //���ڳ�ʼ�������ṹ��(WNDCLASS)
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;	//����������߸ı䣬�����»��ƴ���
    wc.lpfnWndProc = MainWndProc;	//ָ�����ڹ���
    wc.cbClsExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
    wc.cbWndExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
    wc.hInstance = hInstance;	//Ӧ�ó���ʵ���������WinMain���룩
    wc.hIcon = LoadIcon(0, IDC_ARROW);	//ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
    wc.hCursor = LoadCursor(0, IDC_ARROW);	//ʹ�ñ�׼�����ָ����ʽ
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//ָ���˰�ɫ������ˢ���
    wc.lpszMenuName = 0;	//û�в˵���
    wc.lpszClassName = L"MainWnd";	//������
    //������ע��ʧ��
    if (!RegisterClass(&wc)) {
        //��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        return 0;
    }

    //������ע��ɹ�
    RECT R;	//�ü�����
    R.left = 0;
    R.top = 0;
    R.right = 1280;
    R.bottom = 720;
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
    int width = R.right - R.left;
    int hight = R.bottom - R.top;

    //��������,���ز���ֵ
    mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
    //���ڴ���ʧ��
    if (!mhMainWnd) {
        MessageBox(0, L"CreatWindow Failed", 0, 0);
        return 0;
    }
    //���ڴ����ɹ�,����ʾ�����´���
    ShowWindow(mhMainWnd, nShowCmd);
    UpdateWindow(mhMainWnd);

    //��Ϣѭ��
    //������Ϣ�ṹ��
    MSG msg = { 0 };
    BOOL bRet = 0;
    //���GetMessage����������0��˵��û�н��ܵ�WM_QUIT
    while (bRet = GetMessage(&msg, 0, 0, 0) != 0) {
        //�������-1��˵��GetMessage���������ˣ����������
        if (bRet == -1) {
            MessageBox(0, L"GetMessage Failed", L"Errow", MB_OK);
        }
        //�����������ֵ��˵�����յ�����Ϣ
        else {
            TranslateMessage(&msg);	//���̰���ת�������������Ϣת��Ϊ�ַ���Ϣ
            DispatchMessage(&msg);	//����Ϣ���ɸ���Ӧ�Ĵ��ڹ���
        }
    }
    return (int)msg.wParam;
}

#endif

//���ڹ��̺���
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //��Ϣ����
    switch (msg) {
        //�����ڱ�����ʱ����ֹ��Ϣѭ��
    case WM_DESTROY:
        PostQuitMessage(0);	//��ֹ��Ϣѭ����������WM_QUIT��Ϣ
        return 0;
    default:
        break;
    }
    //������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
    //���ڳ�ʼ�������ṹ��(WNDCLASS)
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;	//����������߸ı䣬�����»��ƴ���
    wc.lpfnWndProc = MainWndProc;	//ָ�����ڹ���
    wc.cbClsExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
    wc.cbWndExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
    wc.hInstance = hInstance;	//Ӧ�ó���ʵ���������WinMain���룩
    wc.hIcon = LoadIcon(0, IDC_ARROW);	//ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
    wc.hCursor = LoadCursor(0, IDC_ARROW);	//ʹ�ñ�׼�����ָ����ʽ
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//ָ���˰�ɫ������ˢ���
    wc.lpszMenuName = 0;	//û�в˵���
    wc.lpszClassName = L"MainWnd";	//������
    //������ע��ʧ��
    if (!RegisterClass(&wc))
    {
        //��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        return 0;
    }

    //������ע��ɹ�
    RECT R;	//�ü�����
    R.left = 0;
    R.top = 0;
    R.right = 1280;
    R.bottom = 720;
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
    int width = R.right - R.left;
    int hight = R.bottom - R.top;

    //��������,���ز���ֵ
    //CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
    mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
    //���ڴ���ʧ��
    if (!mhMainWnd)
    {
        MessageBox(0, L"CreatWindow Failed", 0, 0);
        return 0;
    }
    //���ڴ����ɹ�,����ʾ�����´���
    ShowWindow(mhMainWnd, nShowCmd);
    UpdateWindow(mhMainWnd);

    return true;
}

void CreateDevice()
{
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
    //Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    ThrowIfFailed(D3D12CreateDevice(nullptr,    // �˲����������Ϊnullptr����ʹ����������
        D3D_FEATURE_LEVEL_12_0,                 // Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���
        IID_PPV_ARGS(&d3dDevice)));             // ���������豸
}

void CreateFence()
{
    ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void GetDescriptionSize()
{
    rtvDescriptiorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptionSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbv_srv_uavDescriptionSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SetMSAA()
{
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // UNORM�ǹ�һ��������޷�������
    msaaQualityLevels.SampleCount = 1;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;

    // ��ǰͼ��������MSAA���ز�����֧�֣�ע�⣺�ڶ������������������������
    ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
    // NumQualityLevels��check�������������
    // ���֧��MSAA,��check�������ص�NumQualityLevels > 0
    // expression Ϊ�٣�����ֹ�������У�����ӡһ��������Ϣ
    assert(msaaQualityLevels.NumQualityLevels > 0);
}

void CreateCommandObject()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));

    // &cmdAllocator�ȼ�cmdAllocator.GetAddressof
    ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

    ThrowIfFailed(d3dDevice->CreateCommandList(0,   // ����ֵ0����GPU
        D3D12_COMMAND_LIST_TYPE_DIRECT,         // �����б�����
        cmdAllocator.Get(),                 // ����������ӿ�ָ��
        nullptr,                       // ��ˮ��״̬����PSO�����ﲻ���ƣ����Կ�ָ��
        IID_PPV_ARGS(&cmdList)));      // ���ش����������б�
    cmdList->Close();       // ���������б�ǰ���뽫��ر�
}

void CreateSwapChain()
{
    swapChain.Reset();
    DXGI_SWAP_CHAIN_DESC swapChainDesc; // �����������ṹ��
    swapChainDesc.BufferDesc.Width = 1280;
    swapChainDesc.BufferDesc.Height = 720;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;       // ����ɨ��vs����ɨ�裨δָ����
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;   // ͼ�������Ļ�����죨δָ����
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;            // ��������Ⱦ����̨������������Ϊ��ȾĿ�꣩
    swapChainDesc.OutputWindow = mhMainWnd;           // ��Ⱦ���ھ��
    swapChainDesc.SampleDesc.Count = 1;             // ���ز�������
    swapChainDesc.SampleDesc.Quality = 0;           // ���ز�������
    swapChainDesc.Windowed = true;                  // �Ƿ񴰿ڻ�
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       // �̶�д��
    swapChainDesc.BufferCount = 2;                  // ��̨������������˫���壩
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // ����Ӧ����ģʽ���Զ�ѡ�������ڵ�ǰ���ڳߴ����ʾģʽ��

    // ����DXGI�ӿ��µĹ����ഴ��������
    ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));

}

void CreateDescriptorHeap()
{
    // ���ȴ���RTV��
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
    rtvDescriptorHeapDesc.NumDescriptors = 2;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NodeMask = 0;
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    // Ȼ�󴴽�DSV��
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
    dsvDescriptorHeapDesc.NumDescriptors = 3;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.NodeMask = 0;
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvHeap)));
}

void CreateRTV()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < 2; i++) {
        // ��ȡ���ڽ������еĺ�̨��������Դ
        swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf()));
        // ����RTV
        d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(),
            nullptr,        // �ڽ������������Ѿ������˸���Դ�����ݸ�ʽ����������ָ��Ϊ��ָ��
            rtvHeapHandle); // ����������ṹ�壨�����Ǳ��⣬�̳���CD3DX12_CPU_DESCRIPTOR_HANDLE��
        // ƫ�Ƶ����������е���һ��������
        rtvHeapHandle.Offset(1, rtvDescriptiorSize);
    }
}

void CreateDSV()
{
    // ��CPU�д��������ģ��������Դ
    D3D12_RESOURCE_DESC dsvResourceDesc;
    dsvResourceDesc.Alignment = 0;  // ָ������
    dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // ָ����Դά��
    dsvResourceDesc.DepthOrArraySize = 1;   // �������1
    dsvResourceDesc.Width = 1280;
    dsvResourceDesc.Height = 720;
    dsvResourceDesc.MipLevels = 1;
    dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;      // ָ�������֣����ﲻָ����
    dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;    // ���ģ����Դ��Flag
    dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;     // 24λ��ȡ�8λģ�壬���������͵ĸ�ʽ
    dsvResourceDesc.SampleDesc.Count = 4;       // ���ز�������
    dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;
    CD3DX12_CLEAR_VALUE optClear;       // ������Դ���Ż�ֵ��������������ִ���ٶ�
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;    // 24λ��ȣ�8λģ�壬���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
    optClear.DepthStencil.Depth = 1;    // ��ʼ�����ֵΪ1
    optClear.DepthStencil.Stencil = 0;  // ��ʼ��ģ��ֵΪ0

    auto tmp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(d3dDevice->CreateCommittedResource(&tmp, //CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE, // FLAG
        &dsvResourceDesc,      // ���涨���DSV��Դָ��
        D3D12_RESOURCE_STATE_COMMON,   // ��Դ��״̬Ϊ��ʼ״̬
        &optClear,     // ���涨����Ż�ֵָ��
        IID_PPV_ARGS(&depthStencilBuffer)));   // �������ģ����Դ

    d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
        nullptr,      // D3D12_DEPTH_STENCIL_VIEW_DESC ����ָ�룬����&dsvDesc
                      // �����ڴ������ģ����Դʱ�ѽ��������ģ���������ԣ������������ָ��Ϊ��ָ��
        dsvHeap->GetCPUDescriptorHandleForHeapStart());   // DSV���
}

void Transition()
{
    auto tmp = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,	// ת��״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
        D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->ResourceBarrier(1,		// ���ϸ���
        //&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
        //    D3D12_RESOURCE_STATE_COMMON,	// ת��״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
        //    D3D12_RESOURCE_STATE_DEPTH_WRITE)); // ת����״̬Ϊ��д������ͼ������һ��D3D12_RESOURCE_STATE_DEPTH_READ��ֻ�ɶ������ͼ
        &tmp);

    ThrowIfFailed(cmdList->Close());	// ������������ر�
    ID3D12CommandList* cmdLists[] = { cmdList.Get() };	// ���������������б�����
    cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists); // ������������б��д����������	
}

void FlushCmdQueue()
{
    mCurrentFence++;	// CPU��������رպ󣬽���ǰΧ��ֵ+1
    cmdQueue->Signal(fence.Get(), mCurrentFence);
    if (fence->GetCompletedValue() < mCurrentFence) {
        HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
        fence->SetEventOnCompletion(mCurrentFence, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);

        CloseHandle(eventHandle);
    }
}

void CreateViewPortAndScissorRect()
{
    D3D12_VIEWPORT viewPort;
    D3D12_RECT scissorRect;

    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    viewPort.Width = 1280;
    viewPort.Height = 720;
    viewPort.MaxDepth = 1.0f;
    viewPort.MinDepth = 0.0f;

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = 1280;
    scissorRect.bottom = 720;
}

bool InitDirect3D()
{
#if defined(DEBUG) ||defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    CreateDevice();  // ����D3D�豸
    CreateFence();    // ����Χ��
    GetDescriptionSize();   // ����������
    SetMSAA();          // ���ö��ز���
    CreateCommandObject();  // �����������
    CreateSwapChain();  // ����������
    CreateDescriptorHeap(); // ������������
    CreateRTV();    // ������ȾĿ����ͼ
    CreateDSV();    // ���/ģ����ͼ
    //CreateViewPortAndScissorRect();

    return true;
}

void LogOutPutDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& x : modeList) {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;

        std::wstring text = L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) + L"\n";

        OutputDebugString(text.c_str());
    }

}

void LogAdapterOutPuts(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND) {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***outputs:";
        text += desc.DeviceName;
        text += L"\n";

        OutputDebugString(text.c_str());
        LogOutPutDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);

        //delete output;
        ++i;
    }
}

void LogAdapters()
{
    IDXGIFactory* mdxgiFactory = nullptr;
    CreateDXGIFactory(__uuidof(mdxgiFactory), reinterpret_cast<void**>(&mdxgiFactory));

    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
        adapterList.push_back(adapter);
        ++i;
    }
    for (int i = 0; i < adapterList.size(); ++i) {
        LogAdapterOutPuts(adapterList[i]);
        //delete adapterList[i];
    }
}

/// <summary>
/// ����
/// </summary>
void Draw()
{
    ThrowIfFailed(cmdAllocator->Reset());       // �ظ�ʹ�ü�¼���������ڴ�
    ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr)); // ���������б����ڴ�

    UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;

    auto tmp = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);


    cmdList->ResourceBarrier(1,
        //&CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        //D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)); // �ӳ��ֵ���ȾĿ��ת��
        &tmp);
    cmdList->RSSetViewports(1, &viewPort);
    cmdList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                             ref_mCurrentBackBuffer, rtvDescriptiorSize);
    cmdList->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    cmdList->ClearDepthStencilView(dsv_handle,  // DSV���������
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,   // FLAG
        1.0f,   // Ĭ�����ֵ
        0,  // Ĭ��ģ����
        0,  // �ü���������
        nullptr);   // �ü�����ָ��

    cmdList->OMSetRenderTargets(1,      // ���󶨵�RTV����
        &rtv_handle,    // ָ��RTV�����ָ��
        true,      // RTV�����ڶ��ڴ�����������ŵ�
        &dsv_handle);   // ָ��DSV��ָ��
    auto ptmp = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    cmdList->ResourceBarrier(1,
        /*&CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT));*/
        &ptmp);
    ThrowIfFailed(cmdList->Close());

    ID3D12CommandList* commandLists[] = { cmdList.Get() };  // ���������������б�����
    cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);    // ������������б����������

    ThrowIfFailed(swapChain->Present(0, 0)); // ��������������һ��������ʾ�Ƿ�ֱͬ�����ڶ�����ʾ���ַ�ʽ
    mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

    FlushCmdQueue();
}

int Run()
{
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);		// ���̰���ת�������������Ϣת��Ϊ�ַ���Ϣ
            DispatchMessage(&msg);		// ����Ϣ���ɸ���Ӧ�Ĵ��ڹ���
        }
        else
        {
            Draw();
        }
    }
    return (int)msg.wParam;
}

bool Init(HINSTANCE hInstance, int nShowCmd)
{
    if (!InitWindow(hInstance, nShowCmd))
    {
        return false;
    }
    else if (!InitDirect3D())
    {
        return false;
    }
    else
    {
        return true;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try
    {
        if (!Init(hInstance, nShowCmd))
        {
            return 0;
        }
        return Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}