//�w�b�_�[�ǂݍ���
/*#include <stdio.h>
#include <windows.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "MESH.h"
//���C�u�����ǂݍ���
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
//�l�[���X�y�[�X��錾
using namespace DirectX;
//�萔��`
#define WINDOW_WIDTH 1280//�E�B���h�E��
#define WINDOW_HEIGHT 720//�E�B���h�E����
#define FRAME_COUNT 3//��ʃo�b�t�@��

//�O���[�o���ϐ�
HWND g_hWnd = NULL;
IDXGISwapChain3* g_pSwapChain;
ID3D12Device* g_pDevice;
ID3D12Resource* g_pRenderTargets[FRAME_COUNT];
ID3D12CommandAllocator* g_pCommandAllocator;
ID3D12CommandQueue* g_pCommandQueue;
ID3D12DescriptorHeap* g_pRtvHeap;
ID3D12PipelineState* g_pPipelineState;
ID3D12GraphicsCommandList* g_pCommandList;
UINT g_RtvDescriptorSize;
UINT g_FrameIndex;
ID3D12Fence* g_pFence;
UINT64 g_FenceValue;
ID3D12RootSignature* g_pRootSignature;
CD3DX12_VIEWPORT g_Viewport;
CD3DX12_RECT g_ScissorRect;
ID3D12Resource* g_ConstantBuffer;
UINT8* g_pCbvDataBegin;
ID3D12DescriptorHeap* g_pCbvHeap;

ID3D12Resource* g_pTexture;
ID3D12Resource* g_pTextureUploadHeap;
ID3D12DescriptorHeap* g_SamplerHeap;
ID3D12DescriptorHeap* g_SRVHeap;

XMFLOAT3 g_Pos[NUM_OBJECT];
CBUFFER g_CB[NUM_OBJECT];

Mesh* g_pMesh;

//�֐��v���g�^�C�v�錾
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitD3D12(HWND);
void Render();
D3D12_SUBRESOURCE_DATA LoadTextureFromFile(char* File);
HRESULT InitLoader();
//
//�G���g���[�֐�
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR szStr, INT iCmdShow)
{
	//���̌X�̈ʒu�����߂�
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		g_Pos[i] = XMFLOAT3((float)i*0.4 - 0.2, 0, 0);
	}
	HWND g_hWnd = NULL;

	//�E�B���h�E������
	static WCHAR szAppName[] = L"D3D12 ������";
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInst,
		LoadIcon(NULL,IDI_APPLICATION),
		LoadCursor(NULL,IDC_ARROW),
		(HBRUSH)GetStockObject(LTGRAY_BRUSH),
		NULL,
		NULL,
		LoadIcon(NULL,IDI_APPLICATION)
	};

	RegisterClassEx(&wc);
	//g_hWnd = CreateWindow(szAppName, szAppName, WS_EX_OVERLAPPEDWINDOW, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInst, 0);
	g_hWnd = CreateWindow(
		TEXT("STATIC"), TEXT("D3D12"),
		WS_EX_OVERLAPPEDWINDOW,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL,
		hInst, NULL
	);

	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);
	//D3D12������
	InitD3D12(g_hWnd);
	// ���b�Z�[�W���[�v
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else Render();
	}
	//�I��
	return (INT)msg.wParam;
}
//
//�E�B���h�E�v���V�[�W���[
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_KEYDOWN:
		switch ((char)wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}
//
//D3D12�������֐�
HRESULT InitD3D12(HWND hWnd)
{
	//D3D12�f�o�C�X�̍쐬
	// �f�o�b�O�o�[�W�����ł̓f�o�b�O���C���[��L��������
	{
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->Release();
		}
	}

	// DirectX12���T�|�[�g���闘�p�\�ȃn�[�h�E�F�A�A�_�v�^���擾
	IDXGIFactory4*	factory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return FALSE;

	bool g_useWarpDevice = true;

	if (g_useWarpDevice) {
		IDXGIAdapter*	warpAdapter;
		if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)))) return FALSE;
		if (FAILED(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pDevice)))) return FALSE;
		warpAdapter->Release();
	}
	else {
		IDXGIAdapter1*	hardwareAdapter;
		IDXGIAdapter1*	adapter;
		hardwareAdapter = nullptr;

		for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
			// �A�_�v�^��DirectX12�ɑΉ����Ă��邩�m�F
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) break;
		}

		hardwareAdapter = adapter;

		if (FAILED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pDevice)))) return FALSE;
		hardwareAdapter->Release();
		adapter->Release();
	}

	//�R�}���h�L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	g_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue));
	//�R�}���h���X�g�̍쐬
	g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));
	g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pCommandAllocator, NULL, IID_PPV_ARGS(&g_pCommandList));
	g_pCommandList->Close();
	//�X���b�v�`�F�[���̍쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	factory->CreateSwapChainForHwnd(g_pCommandQueue, hWnd, &swapChainDesc, NULL, NULL, (IDXGISwapChain1**)&g_pSwapChain);
	g_FrameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
	//�f�B�X�N���v�^�[�q�[�v�̍쐬�i�����_�[�^�[�Q�b�g�r���[�p�j
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	g_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_pRtvHeap));
	g_RtvDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�t���[���o�b�t�@�ƃo�b�N�o�b�t�@���ꂼ��Ƀ����_�[�^�[�Q�b�g�r���[���쐬
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < FRAME_COUNT; n++)
	{
		g_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&g_pRenderTargets[n]));
		g_pDevice->CreateRenderTargetView(g_pRenderTargets[n], NULL, rtvHandle);
		rtvHandle.Offset(1, g_RtvDescriptorSize);
	}
	//���[�g�V�O�l�`���[���쐬	CBV�A�e�N�X�`���[�A�T���v���[�̏ꏊ���m��
	CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
		0, NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_pRootSignature));
	//�V�F�[�_�[�̍쐬
	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	D3DCompileFromFile(L"Simple.hlsl", NULL, NULL, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	D3DCompileFromFile(L"Simple.hlsl", NULL, NULL, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	//���_���C�A�E�g�̍쐬
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = g_pRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pPipelineState));
	//�t�F���X���쐬
	g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));
	g_FenceValue = 1;
	//�R���X�^���g�o�b�t�@�̍쐬
	UINT CBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	//CBV�i�R���X�^���g�o�b�t�@�r���[�j�p�̃f�B�X�N���v�^�[�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = NUM_OBJECT;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	g_pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&g_pCbvHeap));

	g_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(CBSize * NUM_OBJECT),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g_ConstantBuffer));

	//CBV�i�R���X�^���g�o�b�t�@�r���[�j�̍쐬
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = g_ConstantBuffer->GetGPUVirtualAddress() + i * CBSize;
		cbvDesc.SizeInBytes = CBSize;
		UINT Stride = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE cHandle = g_pCbvHeap->GetCPUDescriptorHandleForHeapStart();
		cHandle.ptr += i * Stride;
		g_pDevice->CreateConstantBufferView(&cbvDesc, cHandle);
	}
	CD3DX12_RANGE readRange = CD3DX12_RANGE(0, 0);
	g_ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pCbvDataBegin));

	//���b�V���쐬
	g_pMesh = new Mesh;

	if (FAILED(g_pMesh->Init(g_pDevice, "Chips.obj", g_pCommandAllocator,
		g_pCommandQueue, g_pCommandList, g_pFence, g_FenceValue, g_pPipelineState)))
	{
		return E_FAIL;
	}
	return S_OK;
}
//�@
//�����_�[�֐�
void Render()
{
	//World View Projection �ϊ�
	XMMATRIX Rot, mWorld;
	XMMATRIX mView;
	XMMATRIX mProj;
	//�r���[�g�����X�t�H�[��
	XMFLOAT3 vEyePt(0.0f, 0.0f, -0.5f);//�J���� �ʒu
	XMFLOAT3 vDir(0.0f, 0.0f, 1.0f);//�J�����@����
	XMFLOAT3 vUpVec(0.0f, 1.0f, 0.0f);//����ʒu
	mView = XMMatrixLookToLH(XMLoadFloat3(&vEyePt), XMLoadFloat3(&vDir), XMLoadFloat3(&vUpVec));
	//�v���W�F�N�V�����g�����X�t�H�[��
	mProj = XMMatrixPerspectiveFovLH(3.14159 / 4, (FLOAT)WINDOW_WIDTH / (FLOAT)WINDOW_HEIGHT, 0.001f, 1000.0f);
	//Reset
	g_pCommandAllocator->Reset();
	g_pCommandList->Reset(g_pCommandAllocator, g_pPipelineState);
	g_pCommandList->SetGraphicsRootSignature(g_pRootSignature);

	//�R���X�^���g�o�b�t�@�̓��e���X�V
	static float r = 0;
	Rot = XMMatrixRotationY(r += 0.1);//�P����yaw��]������
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		//���[���h�g�����X�t�H�[��
		mWorld = Rot * XMMatrixTranslation(g_Pos[i].x, g_Pos[i].y, g_Pos[i].z);
		//�|�C���^�[��i�߂Ȃ���X�V���Ă䂭
		char* ptr = reinterpret_cast<char*>(g_pCbvDataBegin) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * i;
		g_CB[i].mWVP = XMMatrixTranspose(mWorld*mView*mProj);
		g_CB[i].mW = XMMatrixTranspose(mWorld);
		g_CB[i].vEye = XMFLOAT4(vEyePt.x, vEyePt.y, vEyePt.z, 1);
		g_CB[i].vLightDir = XMFLOAT4(0, 0, -1, 0);
		g_CB[i].vAmbient = XMFLOAT4(g_pMesh->m_pMaterial[i].Ka.x, g_pMesh->m_pMaterial[i].Ka.y,
			g_pMesh->m_pMaterial[i].Ka.z, g_pMesh->m_pMaterial[i].Ka.w);
		g_CB[i].vDiffuse = XMFLOAT4(g_pMesh->m_pMaterial[i].Kd.x, g_pMesh->m_pMaterial[i].Kd.y,
			g_pMesh->m_pMaterial[i].Kd.z, g_pMesh->m_pMaterial[i].Kd.w);
		g_CB[i].vSpecular = XMFLOAT4(g_pMesh->m_pMaterial[i].Ks.x, g_pMesh->m_pMaterial[i].Ks.y,
			g_pMesh->m_pMaterial[i].Ks.z, g_pMesh->m_pMaterial[i].Ks.w);
		memcpy(ptr, &g_CB[i], sizeof(CBUFFER));
	}
	g_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
	g_ScissorRect = CD3DX12_RECT(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	g_pCommandList->RSSetViewports(1, &g_Viewport);
	g_pCommandList->RSSetScissorRects(1, &g_ScissorRect);

	g_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_pRenderTargets[g_FrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), g_FrameIndex, g_RtvDescriptorSize);
	g_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	//��ʃN���A
	const float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	g_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, NULL);
	g_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//�����̕`��
	int size = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//�e�N�X�`���[���Z�b�g
	ID3D12DescriptorHeap* ppHeaps2[] = { g_pMesh->m_pMaterial[0].pTextureSRVHeap };
	g_pCommandList->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);
	g_pCommandList->SetGraphicsRootDescriptorTable(1, g_pMesh->m_pMaterial[0].pTextureSRVHeap->GetGPUDescriptorHandleForHeapStart());

	for (int j = 0; j < NUM_OBJECT; j++)
	{
		//�R���X�^���g�o�b�t�@���Z�b�g
		ID3D12DescriptorHeap* ppHeaps[] = { g_pCbvHeap };
		g_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		auto cbvSrvUavDescHeap = g_pCbvHeap->GetGPUDescriptorHandleForHeapStart();
		cbvSrvUavDescHeap.ptr += j * size;
		g_pCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvUavDescHeap);
		//�o�[�e�b�N�X�o�b�t�@���Z�b�g
		g_pCommandList->IASetVertexBuffers(0, 1, &g_pMesh->m_VertexBufferView);

		//�}�e���A���̐������A���ꂼ��̃}�e���A���̃C���f�b�N�X�o�b�t�@�|��`��
		for (DWORD i = 0; i < g_pMesh->m_dwNumMaterial; i++)
		{
			//�C���f�b�N�X�o�b�t�@���Z�b�g
			g_pCommandList->IASetIndexBuffer(&g_pMesh->m_pIndexBufferView[i]);
			//draw
			g_pCommandList->DrawIndexedInstanced(g_pMesh->m_pMaterial[i].dwNumFace * 3, 1, 0, 0, 0);
		}
	}
	g_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_pRenderTargets[g_FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	g_pCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { g_pCommandList };
	g_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	g_pSwapChain->Present(1, 0);
	g_FrameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
	//��������
	g_pCommandQueue->Signal(g_pFence, g_FenceValue);
	while (g_pFence->GetCompletedValue() < g_FenceValue);
	g_FenceValue++;
}*/

#include <Windows.h>
#include <tchar.h>
#include "D3D12Manager.h"

namespace {
	constexpr int WINDOW_WIDTH = 640;
	constexpr int WINDOW_HEIGHT = 480;
	//constexpr LPSTR WND_CLASS_NAME = _T("DirectX12");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd) {
	WNDCLASSEX	wc{};
	HWND hwnd{};

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("D3D12");
	wc.hIcon = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIconSm = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	if (RegisterClassEx(&wc) == 0) {
		return -1;
	}

	//�E�C���h�E�쐬
	hwnd = CreateWindowEx(
		WS_EX_COMPOSITED,
		TEXT("D3D12"),
		TEXT("D3D12"),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		NULL,
		NULL,
		hInst,
		NULL);

	{
		int screen_width = GetSystemMetrics(SM_CXSCREEN);
		int screen_height = GetSystemMetrics(SM_CYSCREEN);
		RECT rect{};
		GetClientRect(hwnd, &rect);
		MoveWindow(
			hwnd,
			(screen_width / 2) - ((WINDOW_WIDTH + (WINDOW_WIDTH - rect.right)) / 2), //�E�C���h�E�����
			(screen_height / 2) - ((WINDOW_HEIGHT + (WINDOW_HEIGHT - rect.bottom)) / 2),//�����Ɏ����Ă���
			WINDOW_WIDTH + (WINDOW_WIDTH - rect.right),
			WINDOW_HEIGHT + (WINDOW_HEIGHT - rect.bottom),
			TRUE);
	}

	//�E�C���h�E���J���ăA�b�v�f�[�g
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	//�}�l�[�W���[������
	D3D12Manager direct_3d(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT);

	//���b�Z�[�W���[�v
	while (TRUE) {
		MSG msg{};
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) { break; }
		}

		direct_3d.Render();

	}

	UnregisterClass(TEXT("D3D12"), NULL); hwnd = NULL;

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:PostQuitMessage(0);	return -1;
	default:		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
