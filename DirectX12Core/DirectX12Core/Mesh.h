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

#define NUM_OBJECT 1 //���̐�

//�R���X�^���g�o�b�t�@�p�\����
struct CBUFFER {
	XMMATRIX mW;	//���[���h�s��
	XMMATRIX mWVP;	//���[���h����ˉe�܂ł̕ϊ��s��
	XMFLOAT4 vLightDir;	//���C�g����
	XMFLOAT4 vEye;	//�J�����ʒu
	XMFLOAT4 vAmbient;	//�A���r�G���g��
	XMFLOAT4 vDiffuse;	//�f�B�t���[�Y�F
	XMFLOAT4 vSpecular;	//���ʔ���

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

//�I���W�i���@�}�e���A���\����
struct MY_MATERIAL
{
	CHAR szName[110];
	XMFLOAT4 Ka;//�A���r�G���g
	XMFLOAT4 Kd;//�f�B�t���[�Y
	XMFLOAT4 Ks;//�X�y�L�����[
	CHAR szTextureName[110];//�e�N�X�`���[�t�@�C����

	ID3D12Resource* pTexture;
	ID3D12DescriptorHeap* pTextureSRVHeap;

	DWORD dwNumFace;//���̃}�e���A���ł���|���S����
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