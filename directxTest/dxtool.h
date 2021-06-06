#ifndef __DXTOOL__
#define __DXTOOL__


#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include "CommonFunc.h"
#include "DxException.h"

class dxtool
{
public:

    dxtool();

    ~dxtool();

    /// <summary>
    /// 创建D3D设备
    /// </summary>
    void CreateDevice();

    /// <summary>
    /// 创建围栏fence
    /// </summary>
    void CreateFence();

    /// <summary>
    /// 获取描述符大小
    /// </summary>
    void GetDescriptionSize();

    /// <summary>
    /// 设置多重采样
    /// </summary>
    void SetMSAA();

    /// <summary>
    /// 创建命令对象
    /// </summary>
    void CreateCommandObject();

    /// <summary>
    /// 创建交换链
    /// </summary>
    void CreateSwapChain();

    /// <summary>
    /// 创建描述符堆
    /// </summary>
    void CreateDescriptorHeap();

    /// <summary>
    /// 创建DSV句柄
    /// </summary>
    void CreateDSV();

    /// <summary>
    /// 创建RTV句柄
    /// </summary>
    void CreateRTV();

    /// <summary>
    /// 转换资源状态
    /// </summary>
    void Transition();

    /// <summary>
    /// 刷新命令队列
    /// </summary>
    void FlushCmdQueue();

    void CreateViewPortAndScissorRect();

    /// <summary>
    /// 初始化D3D
    /// </summary>
    bool InitDirect3D();

    void LogAdapters();

    void LogAdapterOutPuts(IDXGIAdapter* adapter);

    void LogOutPutDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;

    // 创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

    UINT rtvDescriptiorSize;
    UINT dsvDescriptionSize;
    UINT cbv_srv_uavDescriptionSize;

    int mCurrentFence = 0;
};

#endif


