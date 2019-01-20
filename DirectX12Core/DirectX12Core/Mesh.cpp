#include "Mesh.h"
#include <iostream>

HRESULT InitLoader();
D3D12_SUBRESOURCE_DATA LoadTextureFromFile(char* File);

#define WINDOW_WIDTH 640//ウィンドウ幅
#define WINDOW_HEIGHT 480//ウィンドウ高さ

XMFLOAT3 g_Pos[NUM_OBJECT];
CBUFFER g_CB[NUM_OBJECT];

Mesh::Mesh()
{
	ZeroMemory(this, sizeof(Mesh));
	m_fScale = 1.0f;
}


Mesh::~Mesh()
{
	SAFE_DELETE_ARRAY(m_pMaterial);
	SAFE_DELETE_ARRAY(m_ppIndexBuffer);
	SAFE_RELEASE(m_pVertexBuffer);
}

HRESULT Mesh::Init(ID3D12Device* pDevice, const char* FileName, ID3D12CommandAllocator* pAlloc,
	ID3D12CommandQueue* pQueue, ID3D12GraphicsCommandList* pList, ID3D12Fence* pFence,
	UINT64 FenceValue, ID3D12PipelineState* pPipeline) {

	HRESULT hr{};
	D3D12_HEAP_PROPERTIES heap_properties{};
	D3D12_RESOURCE_DESC   resource_desc{};

	//物体個々の位置を決める
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		//g_Pos[i] = XMFLOAT3((float)i*0.4 - 0.2, 0, 0.0);
		g_Pos[i] = XMFLOAT3(0, 0, 0.0);
	}

	m_pDevice = pDevice;
	m_pAlloc = pAlloc;
	m_pQueue = pQueue;
	m_pList = pList;
	m_pFence = pFence;
	m_FenceValue = FenceValue;
	m_pPipeline = pPipeline;

	//コンスタントバッファの作成
	UINT CBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	//CBV（コンスタントバッファビュー）用のディスクリプターヒープ
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = NUM_OBJECT;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap));

	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(CBSize * NUM_OBJECT),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer));

	//CBV（コンスタントバッファビュー）の作成
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress() + i * CBSize;
		cbvDesc.SizeInBytes = CBSize;
		UINT Stride = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE cHandle = m_pCbvHeap->GetCPUDescriptorHandleForHeapStart();
		cHandle.ptr += i * Stride;
		m_pDevice->CreateConstantBufferView(&cbvDesc, cHandle);
	}
	CD3DX12_RANGE readRange = CD3DX12_RANGE(0, 0);
	m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));

	if (FAILED(LoadStaticMesh(FileName)))
	{
		MessageBox(0, "メッシュ作成失敗", NULL, MB_OK);
		return E_FAIL;
	}

	return hr;
}

HRESULT Mesh::LoadMaterialFromFile(const char* FileName, MY_MATERIAL** ppMaterial) {
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");
	char key[110] = { 0 };
	XMFLOAT4 v(0, 0, 0, 1);

	//マテリアル数を調べる
	m_dwNumMaterial = 0;
	while (!feof(fp)) {
		//キーワード読み込み
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル名
		if (strcmp(key, "newmtl") == 0) {
			m_dwNumMaterial++;
		}
	}
	MY_MATERIAL* pMaterial = new MY_MATERIAL[m_dwNumMaterial];
	//本読み込み	
	fseek(fp, SEEK_SET, 0);
	INT iMCount = -1;

	while (!feof(fp)) {
		//キーワード
		fscanf_s(fp, "%s", key, sizeof(key));
		//マテリアル
		if (strcmp(key, "newmtl") == 0) {
			iMCount++;
			fscanf_s(fp, "%s ", key, sizeof(key));
			strcpy_s(pMaterial[iMCount].szName, key);
		}
		//Ka　アンビエント
		if (strcmp(key, "Ka") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[iMCount].Ka = v;
		}
		//Kd　ディフューズ
		if (strcmp(key, "Kd") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[iMCount].Kd = v;
		}
		//Ks　スペキュラー
		if (strcmp(key, "Ks") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[iMCount].Ks = v;
		}
		//map_Kd	テクスチャ
		if (strcmp(key, "map_Kd") == 0) {
			fscanf_s(fp, "%s", &pMaterial[iMCount].szTextureName, sizeof(pMaterial[iMCount].szTextureName));

			//ファイルからテクセルやサイズ情報を読み出す(この処理は完全にユーザー次第）
			InitLoader();
			D3D12_SUBRESOURCE_DATA Subres;
			Subres = LoadTextureFromFile(pMaterial[iMCount].szTextureName);

			//データをもとにテクスチャを作成
			D3D12_RESOURCE_DESC tdesc;
			ZeroMemory(&tdesc, sizeof(tdesc));
			tdesc.MipLevels = 1;
			tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tdesc.Width = Subres.RowPitch / 4;
			tdesc.Height = Subres.SlicePitch / Subres.RowPitch;
			tdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			tdesc.DepthOrArraySize = 1;
			tdesc.SampleDesc.Count = 1;
			tdesc.SampleDesc.Quality = 0;
			tdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &tdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pMaterial[iMCount].pTexture));

			//CPUからGPUへテクスチャデータを渡すときの中継(UploadHeap)
			ID3D12Resource* pTextureUploadHeap;
			DWORD BufferSize = GetRequiredIntermediateSize(pMaterial[iMCount].pTexture, 0, 1);

			m_pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pTextureUploadHeap));

			//コマンドリストに書き込む前にコマンドアロけーたーをリセット
			m_pAlloc->Reset();
			//コマンドリストをリセット
			m_pList->Reset(m_pAlloc, m_pPipeline);

			UpdateSubresources(m_pList, pMaterial[iMCount].pTexture, pTextureUploadHeap, 0, 0, 1, &Subres);
			m_pList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pMaterial[iMCount].pTexture,
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			//ここで一旦コマンドを閉じる。テクスチャーの転送を開始するため
			m_pList->Close();

			//コマンドリストの実行　
			ID3D12CommandList* ppCommandLists[] = { m_pList };
			m_pQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
			//同期（待機）　テクスチャーの転送が終わるまで待機
			{
				m_pQueue->Signal(m_pFence, m_FenceValue);
				//上でセットしたシグナルがGPUから帰ってくるまでストール（この行で待機）
				while (m_pFence->GetCompletedValue() < m_FenceValue);
			}

			//このテクスチャーのビュー（SRV)のディスクリプターヒープの作成
			D3D12_DESCRIPTOR_HEAP_DESC shdesc = {};
			shdesc.NumDescriptors = 1;
			shdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			shdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			m_pDevice->CreateDescriptorHeap(&shdesc, IID_PPV_ARGS(&pMaterial[iMCount].pTextureSRVHeap));
			//このテクスチャーのビュー（SRV)を作る
			D3D12_SHADER_RESOURCE_VIEW_DESC sdesc = {};
			sdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			sdesc.Format = tdesc.Format;
			sdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			sdesc.Texture2D.MipLevels = 1;
			m_pDevice->CreateShaderResourceView(pMaterial[iMCount].pTexture,
				&sdesc, pMaterial[iMCount].pTextureSRVHeap->GetCPUDescriptorHandleForHeapStart());
		}
	}

	fclose(fp);

	*ppMaterial = pMaterial;

	return S_OK;
}

HRESULT Mesh::LoadStaticMesh(const char* FileName)
{
	float x, y, z;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	DWORD dwVCount = 0;//読み込みカウンター
	DWORD dwVNCount = 0;//読み込みカウンター
	DWORD dwVTCount = 0;//読み込みカウンター
	DWORD dwFCount = 0;//読み込みカウンター

	char key[200] = { 0 };
	//OBJファイルを開いて内容を読み込む
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");

	//事前に頂点数、ポリゴン数を調べる
	while (!feof(fp))
	{
		//キーワード読み込み
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル読み込み
		if (strcmp(key, "mtllib") == 0)
		{
			fscanf_s(fp, "%s ", key, sizeof(key));
			LoadMaterialFromFile(key, &m_pMaterial);
		}
		//頂点
		if (strcmp(key, "v") == 0)
		{
			m_dwNumVert++;
		}
		//法線
		if (strcmp(key, "vn") == 0)
		{
			dwVNCount++;
		}
		//テクスチャー座標
		if (strcmp(key, "vt") == 0)
		{
			dwVTCount++;
		}
		//フェイス（ポリゴン）
		if (strcmp(key, "f") == 0)
		{
			m_dwNumFace++;
		}
	}

	//一時的なメモリ確保（頂点バッファとインデックスバッファ）
	MY_VERTEX* pvVertexBuffer = new MY_VERTEX[m_dwNumFace * 3];
	XMFLOAT3* pvCoord = new XMFLOAT3[m_dwNumVert];
	XMFLOAT3* pvNormal = new XMFLOAT3[dwVNCount];
	XMFLOAT2* pvTexture = new XMFLOAT2[dwVTCount];

	//本読み込み	
	fseek(fp, SEEK_SET, 0);
	dwVCount = 0;
	dwVNCount = 0;
	dwVTCount = 0;
	dwFCount = 0;

	while (!feof(fp))
	{
		//キーワード 読み込み
		ZeroMemory(key, sizeof(key));
		fscanf_s(fp, "%s ", key, sizeof(key));

		//頂点 読み込み
		if (strcmp(key, "v") == 0)
		{
			fscanf_s(fp, "%f %f %f", &x, &y, &z);
			pvCoord[dwVCount].x = -x;
			pvCoord[dwVCount].y = y;
			pvCoord[dwVCount].z = z;
			dwVCount++;
		}

		//法線 読み込み
		if (strcmp(key, "vn") == 0)
		{
			fscanf_s(fp, "%f %f %f", &x, &y, &z);
			pvNormal[dwVNCount].x = -x;
			pvNormal[dwVNCount].y = y;
			pvNormal[dwVNCount].z = z;
			dwVNCount++;
		}

		//テクスチャー座標 読み込み
		if (strcmp(key, "vt") == 0)
		{
			fscanf_s(fp, "%f %f", &x, &y);
			pvTexture[dwVTCount].x = x;
			pvTexture[dwVTCount].y = 1 - y;//OBJファイルはY成分が逆なので合わせる
			dwVTCount++;
		}
	}

	//マテリアルの数だけインデックスバッファーを作成
	m_ppIndexBuffer = new ID3D12Resource*[m_dwNumMaterial];
	m_pIndexBufferView = new D3D12_INDEX_BUFFER_VIEW[m_dwNumMaterial];

	//フェイス　読み込み　バラバラに収録されている可能性があるので、マテリアル名を頼りにつなぎ合わせる
	bool boFlag = false;
	int* piFaceBuffer = new int[m_dwNumFace * 3];//３頂点ポリゴンなので、1フェイス=3頂点(3インデックス)
	dwFCount = 0;
	DWORD dwPartFCount = 0;
	for (DWORD i = 0; i < m_dwNumMaterial; i++)
	{
		dwPartFCount = 0;
		fseek(fp, SEEK_SET, 0);

		while (!feof(fp))
		{
			//キーワード 読み込み
			ZeroMemory(key, sizeof(key));
			fscanf_s(fp, "%s ", key, sizeof(key));

			//フェイス 読み込み→頂点インデックスに
			if (strcmp(key, "usemtl") == 0)
			{
				fscanf_s(fp, "%s ", key, sizeof(key));
				if (strcmp(key, m_pMaterial[i].szName) == 0)
				{
					boFlag = true;
				}
				else
				{
					boFlag = false;
				}
			}
			if (strcmp(key, "f") == 0 && boFlag == true)
			{
				if (m_pMaterial[i].pTexture != NULL)//テクスチャーありサーフェイス
				{
					fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				}
				else//テクスチャー無しサーフェイス
				{
					fscanf_s(fp, "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				}
				//インデックスバッファー
				piFaceBuffer[dwPartFCount * 3] = dwFCount * 3;
				piFaceBuffer[dwPartFCount * 3 + 1] = dwFCount * 3 + 1;
				piFaceBuffer[dwPartFCount * 3 + 2] = dwFCount * 3 + 2;
				//頂点構造体に代入
				pvVertexBuffer[dwFCount * 3].vPos = pvCoord[v1 - 1];
				pvVertexBuffer[dwFCount * 3].vNorm = pvNormal[vn1 - 1];
				pvVertexBuffer[dwFCount * 3].vTex = pvTexture[vt1 - 1];
				pvVertexBuffer[dwFCount * 3 + 1].vPos = pvCoord[v2 - 1];
				pvVertexBuffer[dwFCount * 3 + 1].vNorm = pvNormal[vn2 - 1];
				pvVertexBuffer[dwFCount * 3 + 1].vTex = pvTexture[vt2 - 1];
				pvVertexBuffer[dwFCount * 3 + 2].vPos = pvCoord[v3 - 1];
				pvVertexBuffer[dwFCount * 3 + 2].vNorm = pvNormal[vn3 - 1];
				pvVertexBuffer[dwFCount * 3 + 2].vTex = pvTexture[vt3 - 1];

				dwPartFCount++;
				dwFCount++;
			}
		}
		if (dwPartFCount == 0)//使用されていないマテリアル対策
		{
			m_ppIndexBuffer[i] = NULL;
			continue;
		}
		//インデックスバッファーを作成
		const UINT indexBufferSize = sizeof(int) * dwPartFCount * 3;
		m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ppIndexBuffer[i]));
		//インデックスバッファにインデックスデータを詰め込む
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange2(0, 0);
		m_ppIndexBuffer[i]->Map(0, &readRange2, reinterpret_cast<void**>(&pIndexDataBegin));
		memcpy(pIndexDataBegin, piFaceBuffer, indexBufferSize);
		m_ppIndexBuffer[i]->Unmap(0, NULL);
		//インデックスバッファビューを初期化
		m_pIndexBufferView[i].BufferLocation = m_ppIndexBuffer[i]->GetGPUVirtualAddress();
		m_pIndexBufferView[i].Format = DXGI_FORMAT_R32_UINT;
		m_pIndexBufferView[i].SizeInBytes = indexBufferSize;

		m_pMaterial[i].dwNumFace = dwPartFCount;

	}
	delete[] piFaceBuffer;
	fclose(fp);

	//バーテックスバッファー作成
	const UINT vertexBufferSize = sizeof(MY_VERTEX) *m_dwNumFace * 3;
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pVertexBuffer));
	//バーテックスバッファに頂点データを詰め込む
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, pvVertexBuffer, vertexBufferSize);
	m_pVertexBuffer->Unmap(0, NULL);
	//バーテックスバッファビューを初期化
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(MY_VERTEX);
	m_VertexBufferView.SizeInBytes = vertexBufferSize;

	//一時的な入れ物は、もはや不要
	delete pvCoord;
	delete pvNormal;
	delete pvTexture;
	delete[] pvVertexBuffer;

	return S_OK;
}

HRESULT Mesh::Draw(ID3D12GraphicsCommandList *command_list) {
	HRESULT hr;
	static int Cnt{};
	++Cnt;

	command_list->SetPipelineState(m_pPipeline);

	//World View Projection 変換
	XMMATRIX Rot, mWorld;
	XMMATRIX mView;
	XMMATRIX mProj;
	//ビュートランスフォーム
	XMFLOAT3 vEyePt(0.0f, 0.0f, -0.5f);//カメラ 位置
	XMFLOAT3 vDir(0.0f, 0.0f, 1.0f);//カメラ　方向
	XMFLOAT3 vUpVec(0.0f, 1.0f, 0.0f);//上方位置
	mView = XMMatrixLookToLH(XMLoadFloat3(&vEyePt), XMLoadFloat3(&vDir), XMLoadFloat3(&vUpVec));
	//プロジェクショントランスフォーム
	mProj = XMMatrixPerspectiveFovLH(3.14159 / 4, (FLOAT)WINDOW_WIDTH / (FLOAT)WINDOW_HEIGHT, 0.001f, 1000.0f);

	//コンスタントバッファの内容を更新
	static float r = 0;
	Rot = XMMatrixRotationY(r += 0.01);//単純にyaw回転させる	
	for (int i = 0; i < NUM_OBJECT; i++)
	{
		//ワールドトランスフォーム	
		mWorld = Rot * XMMatrixTranslation(g_Pos[i].x, g_Pos[i].y, g_Pos[i].z);

		//ポインターを進めながら更新してゆく
		//Initの中でコンスタントバッファビューメモリの場所をマッピングしているので、そこの始まりの場所を読みだしてマッピングしている
		char* ptr = reinterpret_cast<char*>(m_pCbvDataBegin) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * i;

		g_CB[i].mWVP = XMMatrixTranspose(mWorld*mView*mProj);
		g_CB[i].mW = XMMatrixTranspose(mWorld);
		g_CB[i].vEye = XMFLOAT4(vEyePt.x, vEyePt.y, vEyePt.z, 1);
		g_CB[i].vLightDir = XMFLOAT4(0, 0, -1, 0);
		g_CB[i].vAmbient = XMFLOAT4(m_pMaterial[i].Ka.x, m_pMaterial[i].Ka.y,
			m_pMaterial[i].Ka.z, m_pMaterial[i].Ka.w);
		g_CB[i].vDiffuse = XMFLOAT4(m_pMaterial[i].Kd.x, m_pMaterial[i].Kd.y,
			m_pMaterial[i].Kd.z, m_pMaterial[i].Kd.w);
		g_CB[i].vSpecular = XMFLOAT4(m_pMaterial[i].Ks.x, m_pMaterial[i].Ks.y,
			m_pMaterial[i].Ks.z, m_pMaterial[i].Ks.w);
		memcpy(ptr, &g_CB[i], sizeof(CBUFFER));
	}

	//三角形で描画
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//複数の描画
	int size = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < m_dwNumMaterial; i++) {
		//テクスチャーをセット
		ID3D12DescriptorHeap* ppHeaps2[] = { m_pMaterial[i].pTextureSRVHeap.Get() };
		command_list->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);
		command_list->SetGraphicsRootDescriptorTable(1, m_pMaterial[i].pTextureSRVHeap->GetGPUDescriptorHandleForHeapStart());
	}

	for (int j = 0; j < NUM_OBJECT; j++)
	{
		//コンスタントバッファをセット
		ID3D12DescriptorHeap* ppHeaps[] = { m_pCbvHeap };
		command_list->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		auto cbvSrvUavDescHeap = m_pCbvHeap->GetGPUDescriptorHandleForHeapStart();
		cbvSrvUavDescHeap.ptr += j * size;
		command_list->SetGraphicsRootDescriptorTable(0, cbvSrvUavDescHeap);
		//command_list->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

		//バーテックスバッファをセット
		command_list->IASetVertexBuffers(0, 1, &m_VertexBufferView);

		//マテリアルの数だけ、それぞれのマテリアルのインデックスバッファ－を描画
		for (DWORD i = 0; i < m_dwNumMaterial; i++)
		{
			//インデックスバッファをセット
			command_list->IASetIndexBuffer(&m_pIndexBufferView[i]);
			//draw
			command_list->DrawIndexedInstanced(m_pMaterial[i].dwNumFace * 3, 1, 0, 0, 0);
		}
	}

	std::cout << Cnt << std::endl;

	return S_OK;
}