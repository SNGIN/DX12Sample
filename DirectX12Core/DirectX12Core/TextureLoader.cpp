#include <windows.h>
#include <d3d11.h>
#include <d3dx10.h>
#include <d3dx11.h>
#include <d3d12.h>
#include <d3dx12.h>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dx11.lib")

ID3D11Device* g_pDevice;
ID3D11DeviceContext *g_pContext;
//
//
HRESULT InitLoader()
{
	D3D_FEATURE_LEVEL pLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;
	if (FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, pLevels,
		ARRAYSIZE(pLevels), D3D11_SDK_VERSION, &g_pDevice, &featureLevel, &g_pContext)))
	{
		return E_FAIL;
	}
	return S_OK;
}
//
//
D3D12_SUBRESOURCE_DATA LoadTextureFromFile(char* File)
{
	D3D12_SUBRESOURCE_DATA ret = {};
	ID3D11Texture2D* pTex = NULL;

	D3DX11_IMAGE_LOAD_INFO li = {};
	li.Width = D3DX11_FROM_FILE;
	li.Height = D3DX11_FROM_FILE;
	li.Depth = D3DX11_FROM_FILE;
	li.FirstMipLevel = 0;
	li.MipLevels = D3DX11_FROM_FILE;
	li.Usage = D3D11_USAGE_STAGING;
	li.Format = DXGI_FORMAT_FROM_FILE;
	li.BindFlags = 0;
	li.CpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	li.MiscFlags = 0;
	li.Filter = D3DX11_FILTER_NONE;
	li.MipFilter = D3DX11_FILTER_LINEAR;;
	li.pSrcInfo = 0;
	if (FAILED(D3DX11CreateTextureFromFileA(g_pDevice, File,
		&li, NULL, (ID3D11Resource **)&pTex, NULL)))
	{
		return ret;
	}

	D3D11_MAPPED_SUBRESOURCE Subres;
	if (FAILED(g_pContext->Map(pTex, 0, D3D11_MAP_READ_WRITE, 0, &Subres)))
	{
		return ret;
	}

	ret.pData = Subres.pData;
	ret.RowPitch = Subres.RowPitch;
	ret.SlicePitch = Subres.DepthPitch;

	return ret;
}