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

    void CreateRTV();

    void LogAdapters();

    void LogAdapterOutPuts(IDXGIAdapter* adapter);

    void LogOutPutDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

    UINT rtvDescriptiorSize;
    UINT dsvDescriptionSize;
    UINT cbv_srv_uavDescriptionSize;
};

#endif


