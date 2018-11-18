#pragma once
#include <stdio.h>
#include <windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"d3dCompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace DirectX;

#define SAFE_RELEASE(x) if(x){x->Release(); x=0;}
#define SAFE_DELETE(x) if(x){delete x; x=0;}
#define SAFE_DELETE_ARRAY(x) if(x){delete[] x; x=0;}

#define NUM_OBJECT 1 //物体数

//コンスタントバッファ用構造体
struct CBUFFER {
	XMMATRIX mW;	//ワールド行列
	XMMATRIX mWVP;	//ワールドから射影までの変換行列
	XMFLOAT4 vLightDir;	//ライト方向
	XMFLOAT4 vEye;	//カメラ位置
	XMFLOAT4 vAmbient;	//アンビエント光
	XMFLOAT4 vDiffuse;	//ディフューズ色
	XMFLOAT4 vSpecular;	//鏡面反射

	CBUFFER() {
		ZeroMemory(this, sizeof(CBUFFER));
	}
};

struct MY_VERTEX
{
	XMFLOAT3 vPos;
	XMFLOAT3 vNorm;
	XMFLOAT2 vTex;
};

//オリジナル　マテリアル構造体
struct MY_MATERIAL
{
	CHAR szName[110];
	XMFLOAT4 Ka;//アンビエント
	XMFLOAT4 Kd;//ディフューズ
	XMFLOAT4 Ks;//スペキュラー
	CHAR szTextureName[110];//テクスチャーファイル名

	ID3D12Resource* pTexture;
	ID3D12DescriptorHeap* pTextureSRVHeap;

	DWORD dwNumFace;//そのマテリアルであるポリゴン数
	MY_MATERIAL()
	{
		ZeroMemory(this, sizeof(MY_MATERIAL));
	}
	~MY_MATERIAL()
	{
		SAFE_RELEASE(pTexture);
	}
};

class Mesh
{
public:
	DWORD m_dwNumVert;
	DWORD m_dwNumFace;
	ID3D12Resource* m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	ID3D12Resource** m_ppIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW* m_pIndexBufferView;
	DWORD m_dwNumMaterial;
	MY_MATERIAL* m_pMaterial;
	XMFLOAT3 m_vPos;
	float m_fYaw, m_fPitch, m_fRoll;
	float m_fScale;

	Mesh();
	~Mesh();
	HRESULT Init(ID3D12Device* pDevice, const char* FileName, ID3D12CommandAllocator* pAlloc,
		ID3D12CommandQueue* pQueue, ID3D12GraphicsCommandList* pList, ID3D12Fence* pFence,
		UINT64 FenceValue, ID3D12PipelineState*);
	HRESULT Draw(ID3D12GraphicsCommandList *command_list);

private:
	ID3D12Device* m_pDevice;
	ID3D12CommandAllocator* m_pAlloc;
	ID3D12CommandQueue* m_pQueue;
	ID3D12GraphicsCommandList* m_pList;
	ID3D12Fence* m_pFence;
	UINT64 m_FenceValue;
	ID3D12PipelineState* m_pPipeline;
	ID3D12Resource* m_constantBuffer;
	UINT8* m_pCbvDataBegin;
	ID3D12DescriptorHeap* m_pCbvHeap;

	HRESULT LoadStaticMesh(const char* FileName);
	HRESULT LoadMaterialFromFile(const char* FileName, MY_MATERIAL** ppMaterial);
};