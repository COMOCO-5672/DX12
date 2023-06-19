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

// 创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
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

HWND mhMainWnd = 0;	//某个窗口的句柄，ShowWindow和UpdateWindow函数均要调用此句柄
//窗口过程函数的声明,HWND是主窗口句柄
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

#if 0

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    //窗口初始化描述结构体(WNDCLASS)
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;	//当工作区宽高改变，则重新绘制窗口
    wc.lpfnWndProc = MainWndProc;	//指定窗口过程
    wc.cbClsExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
    wc.cbWndExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
    wc.hInstance = hInstance;	//应用程序实例句柄（由WinMain传入）
    wc.hIcon = LoadIcon(0, IDC_ARROW);	//使用默认的应用程序图标
    wc.hCursor = LoadCursor(0, IDC_ARROW);	//使用标准的鼠标指针样式
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//指定了白色背景画刷句柄
    wc.lpszMenuName = 0;	//没有菜单栏
    wc.lpszClassName = L"MainWnd";	//窗口名
    //窗口类注册失败
    if (!RegisterClass(&wc)) {
        //消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        return 0;
    }

    //窗口类注册成功
    RECT R;	//裁剪矩形
    R.left = 0;
    R.top = 0;
    R.right = 1280;
    R.bottom = 720;
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//根据窗口的客户区大小计算窗口的大小
    int width = R.right - R.left;
    int hight = R.bottom - R.top;

    //创建窗口,返回布尔值
    mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
    //窗口创建失败
    if (!mhMainWnd) {
        MessageBox(0, L"CreatWindow Failed", 0, 0);
        return 0;
    }
    //窗口创建成功,则显示并更新窗口
    ShowWindow(mhMainWnd, nShowCmd);
    UpdateWindow(mhMainWnd);

    //消息循环
    //定义消息结构体
    MSG msg = { 0 };
    BOOL bRet = 0;
    //如果GetMessage函数不等于0，说明没有接受到WM_QUIT
    while (bRet = GetMessage(&msg, 0, 0, 0) != 0) {
        //如果等于-1，说明GetMessage函数出错了，弹出错误框
        if (bRet == -1) {
            MessageBox(0, L"GetMessage Failed", L"Errow", MB_OK);
        }
        //如果等于其他值，说明接收到了消息
        else {
            TranslateMessage(&msg);	//键盘按键转换，将虚拟键消息转换为字符消息
            DispatchMessage(&msg);	//把消息分派给相应的窗口过程
        }
    }
    return (int)msg.wParam;
}

#endif

//窗口过程函数
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //消息处理
    switch (msg) {
        //当窗口被销毁时，终止消息循环
    case WM_DESTROY:
        PostQuitMessage(0);	//终止消息循环，并发出WM_QUIT消息
        return 0;
    default:
        break;
    }
    //将上面没有处理的消息转发给默认的窗口过程
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
    //窗口初始化描述结构体(WNDCLASS)
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;	//当工作区宽高改变，则重新绘制窗口
    wc.lpfnWndProc = MainWndProc;	//指定窗口过程
    wc.cbClsExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
    wc.cbWndExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
    wc.hInstance = hInstance;	//应用程序实例句柄（由WinMain传入）
    wc.hIcon = LoadIcon(0, IDC_ARROW);	//使用默认的应用程序图标
    wc.hCursor = LoadCursor(0, IDC_ARROW);	//使用标准的鼠标指针样式
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//指定了白色背景画刷句柄
    wc.lpszMenuName = 0;	//没有菜单栏
    wc.lpszClassName = L"MainWnd";	//窗口名
    //窗口类注册失败
    if (!RegisterClass(&wc))
    {
        //消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        return 0;
    }

    //窗口类注册成功
    RECT R;	//裁剪矩形
    R.left = 0;
    R.top = 0;
    R.right = 1280;
    R.bottom = 720;
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//根据窗口的客户区大小计算窗口的大小
    int width = R.right - R.left;
    int hight = R.bottom - R.top;

    //创建窗口,返回布尔值
    //CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
    mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
    //窗口创建失败
    if (!mhMainWnd)
    {
        MessageBox(0, L"CreatWindow Failed", 0, 0);
        return 0;
    }
    //窗口创建成功,则显示并更新窗口
    ShowWindow(mhMainWnd, nShowCmd);
    UpdateWindow(mhMainWnd);

    return true;
}

void CreateDevice()
{
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
    //Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    ThrowIfFailed(D3D12CreateDevice(nullptr,    // 此参数如果设置为nullptr，则使用主适配器
        D3D_FEATURE_LEVEL_12_0,                 // 应用程序需要硬件所支持的最低功能级别
        IID_PPV_ARGS(&d3dDevice)));             // 返回所建设备
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
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // UNORM是归一化处理的无符号整数
    msaaQualityLevels.SampleCount = 1;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;

    // 当前图像驱动对MSAA多重采样的支持（注意：第二个参数既是输入又是输出）
    ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
    // NumQualityLevels在check函数会进行设置
    // 如果支持MSAA,则check函数返回的NumQualityLevels > 0
    // expression 为假，则终止程序运行，并打印一条出错信息
    assert(msaaQualityLevels.NumQualityLevels > 0);
}

void CreateCommandObject()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));

    // &cmdAllocator等价cmdAllocator.GetAddressof
    ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

    ThrowIfFailed(d3dDevice->CreateCommandList(0,   // 掩码值0，单GPU
        D3D12_COMMAND_LIST_TYPE_DIRECT,         // 命令列表类型
        cmdAllocator.Get(),                 // 命令分配器接口指针
        nullptr,                       // 流水线状态对象PSO，这里不绘制，所以空指针
        IID_PPV_ARGS(&cmdList)));      // 返回创建的命令列表
    cmdList->Close();       // 重置命令列表前必须将其关闭
}

void CreateSwapChain()
{
    swapChain.Reset();
    DXGI_SWAP_CHAIN_DESC swapChainDesc; // 交换链描述结构体
    swapChainDesc.BufferDesc.Width = 1280;
    swapChainDesc.BufferDesc.Height = 720;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;       // 逐行扫描vs隔行扫描（未指定）
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;   // 图像相对屏幕的拉伸（未指定）
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;            // 将数据渲染至后台缓冲区（即作为渲染目标）
    swapChainDesc.OutputWindow = mhMainWnd;           // 渲染窗口句柄
    swapChainDesc.SampleDesc.Count = 1;             // 多重采样数量
    swapChainDesc.SampleDesc.Quality = 0;           // 多重采样质量
    swapChainDesc.Windowed = true;                  // 是否窗口化
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       // 固定写法
    swapChainDesc.BufferCount = 2;                  // 后台缓冲区数量（双缓冲）
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // 自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）

    // 利用DXGI接口下的工厂类创建交换链
    ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));

}

void CreateDescriptorHeap()
{
    // 首先创建RTV堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
    rtvDescriptorHeapDesc.NumDescriptors = 2;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NodeMask = 0;
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    // 然后创建DSV堆
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
        // 获取存于交换链中的后台缓冲区资源
        swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf()));
        // 创建RTV
        d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(),
            nullptr,        // 在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
            rtvHeapHandle); // 描述符句柄结构体（这里是辩题，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
        // 偏移到描述符堆中的下一个缓冲区
        rtvHeapHandle.Offset(1, rtvDescriptiorSize);
    }
}

void CreateDSV()
{
    // 在CPU中创建好深度模板数据资源
    D3D12_RESOURCE_DESC dsvResourceDesc;
    dsvResourceDesc.Alignment = 0;  // 指定对齐
    dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 指定资源维度
    dsvResourceDesc.DepthOrArraySize = 1;   // 纹理深度1
    dsvResourceDesc.Width = 1280;
    dsvResourceDesc.Height = 720;
    dsvResourceDesc.MipLevels = 1;
    dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;      // 指定纹理布局（这里不指定）
    dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;    // 深度模板资源的Flag
    dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;     // 24位深度、8位模板，还有无类型的格式
    dsvResourceDesc.SampleDesc.Count = 4;       // 多重采样数量
    dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;
    CD3DX12_CLEAR_VALUE optClear;       // 消除资源的优化值，提高清除操作的执行速度
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;    // 24位深度，8位模板，还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
    optClear.DepthStencil.Depth = 1;    // 初始化深度值为1
    optClear.DepthStencil.Stencil = 0;  // 初始化模板值为0

    auto tmp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(d3dDevice->CreateCommittedResource(&tmp, //CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE, // FLAG
        &dsvResourceDesc,      // 上面定义的DSV资源指针
        D3D12_RESOURCE_STATE_COMMON,   // 资源的状态为初始状态
        &optClear,     // 上面定义的优化值指针
        IID_PPV_ARGS(&depthStencilBuffer)));   // 返回深度模板资源

    d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
        nullptr,      // D3D12_DEPTH_STENCIL_VIEW_DESC 类型指针，可填&dsvDesc
                      // 由于在创建深度模板资源时已将定义深度模板数据属性，所有这里可以指定为空指针
        dsvHeap->GetCPUDescriptorHandleForHeapStart());   // DSV句柄
}

void Transition()
{
    auto tmp = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,	// 转换状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
        D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->ResourceBarrier(1,		// 屏障个数
        //&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
        //    D3D12_RESOURCE_STATE_COMMON,	// 转换状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
        //    D3D12_RESOURCE_STATE_DEPTH_WRITE)); // 转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图
        &tmp);

    ThrowIfFailed(cmdList->Close());	// 命令添加完后将其关闭
    ID3D12CommandList* cmdLists[] = { cmdList.Get() };	// 声明并定义命令列表数组
    cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists); // 将命令从命令列表中传至命令队列	
}

void FlushCmdQueue()
{
    mCurrentFence++;	// CPU传完命令并关闭后，将当前围栏值+1
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

    CreateDevice();  // 创建D3D设备
    CreateFence();    // 创建围栏
    GetDescriptionSize();   // 创建描述符
    SetMSAA();          // 设置多重采样
    CreateCommandObject();  // 创建命令对象
    CreateSwapChain();  // 创建交换链
    CreateDescriptorHeap(); // 创建描述符堆
    CreateRTV();    // 创建渲染目标视图
    CreateDSV();    // 深度/模板视图
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
/// 绘制
/// </summary>
void Draw()
{
    ThrowIfFailed(cmdAllocator->Reset());       // 重复使用记录命令的相关内存
    ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr)); // 复用命令列表及其内存

    UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;

    auto tmp = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);


    cmdList->ResourceBarrier(1,
        //&CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        //D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)); // 从呈现到渲染目标转换
        &tmp);
    cmdList->RSSetViewports(1, &viewPort);
    cmdList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                             ref_mCurrentBackBuffer, rtvDescriptiorSize);
    cmdList->ClearRenderTargetView(rtv_handle, DirectX::Colors::DarkRed, 0, nullptr);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    cmdList->ClearDepthStencilView(dsv_handle,  // DSV描述符句柄
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,   // FLAG
        1.0f,   // 默认深度值
        0,  // 默认模板量
        0,  // 裁剪矩形数量
        nullptr);   // 裁剪矩形指针

    cmdList->OMSetRenderTargets(1,      // 待绑定的RTV数量
        &rtv_handle,    // 指向RTV数组的指针
        true,      // RTV对象在堆内存中是连续存放的
        &dsv_handle);   // 指向DSV的指针
    auto ptmp = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    cmdList->ResourceBarrier(1,
        /*&CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT));*/
        &ptmp);
    ThrowIfFailed(cmdList->Close());

    ID3D12CommandList* commandLists[] = { cmdList.Get() };  // 声明并定义命令列表数组
    cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);    // 将命令从命令列表传至命令队列

    ThrowIfFailed(swapChain->Present(0, 0)); // 交换缓冲器，第一个参数表示是否垂直同步，第二个表示呈现方式
    mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

    FlushCmdQueue();
}

int Run()
{
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);		// 键盘按键转换，将虚拟键消息转换为字符信息
            DispatchMessage(&msg);		// 把消息分派给相应的窗口过程
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