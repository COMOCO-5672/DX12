#include "dxtool.h"

#include <string>
#include <vector>

dxtool::dxtool()
{

}

dxtool::~dxtool()
{

}

void dxtool::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	//Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
	ThrowIfFailed(D3D12CreateDevice(nullptr,    // 此参数如果设置为nullptr，则使用主适配器
									D3D_FEATURE_LEVEL_12_0,                 // 应用程序需要硬件所支持的最低功能级别
									IID_PPV_ARGS(&d3dDevice)));             // 返回所建设备
}

void dxtool::CreateFence()
{
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void dxtool::GetDescriptionSize()
{
	rtvDescriptiorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptionSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uavDescriptionSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void dxtool::SetMSAA()
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

void dxtool::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
	// &cmdAllocator等价cmdAllocator.GetAddressof
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	ThrowIfFailed(d3dDevice->CreateCommandList(0,   // 掩码值0，单GPU
											   D3D12_COMMAND_LIST_TYPE_DIRECT,         // 命令列表类型
											   cmdAllocator.Get(),                 // 命令分配器接口指针
											   nullptr,                       // 流水线状态对象PSO，这里不绘制，所以空指针
											   IID_PPV_ARGS(&cmdList)));      // 返回创建的命令列表
	cmdList->Close();       // 重置命令列表前必须将其关闭
}

void dxtool::CreateSwapChain()
{
	swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc; // 交换链描述结构体
	swapChainDesc.BufferDesc = {
		1280,               // 缓冲区分辨率宽度
		720,                // 缓冲区分辨率高度
		60,                 // 刷新率的分母
		1,                  // 刷新率的分子
	};
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;       // 逐行扫描vs隔行扫描（未指定）
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;   // 图像相对屏幕的拉伸（未指定）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;            // 将数据渲染至后台缓冲区（即作为渲染目标）

	//TODO:获取窗口句柄
	swapChainDesc.OutputWindow = nullptr;           // 渲染窗口句柄
	swapChainDesc.SampleDesc.Count = 1;             // 多重采样数量
	swapChainDesc.SampleDesc.Quality = 0;           // 多重采样质量
	swapChainDesc.Windowed = true;                  // 是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       // 固定写法
	swapChainDesc.BufferCount = 2;                  // 后台缓冲区数量（双缓冲）
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // 自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）

	// 利用DXGI接口下的工厂类创建交换链
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));

}

void dxtool::CreateDescriptorHeap()
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

void dxtool::CreateRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[2];
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

void dxtool::CreateDSV()
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

	auto ptmp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(d3dDevice->CreateCommittedResource(&ptmp, //CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

void dxtool::Transition()
{
	auto tmp = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,	// 转换状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cmdList->ResourceBarrier(1,		// 屏障个数
							 //&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
							 //D3D12_RESOURCE_STATE_COMMON,	// 转换状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
							 //D3D12_RESOURCE_STATE_DEPTH_WRITE)); // 转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图
		&tmp
		);

	ThrowIfFailed(cmdList->Close());	// 命令添加完后将其关闭
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };	// 声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists); // 将命令从命令列表中传至命令队列	
}

void dxtool::FlushCmdQueue()
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

void dxtool::CreateViewPortAndScissorRect()
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

bool dxtool::InitDirect3D()
{
#if defined(DEBUG) ||defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	CreateDevice();
	CreateFence();
	GetDescriptionSize();
	SetMSAA();
	CreateCommandObject();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateRTV();
	CreateDSV();
	CreateViewPortAndScissorRect();

	return true;
}


void dxtool::LogAdapters()
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

void dxtool::LogAdapterOutPuts(IDXGIAdapter* adapter)
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

void dxtool::LogOutPutDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
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
