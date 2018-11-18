#include <fstream>
#include "D3D12Manager.h"

///�R���X�g���N�^
D3D12Manager::D3D12Manager(HWND hwnd, int window_width, int window_height) :
	window_handle_(hwnd),
	window_width_(window_width),
	window_height_(window_height),
	frames_{},
	rtv_index_{}{

	//�f�o�C�X�̏������ɂ��������̂�
	CreateFactory();
	CreateDevice();


	//�R�}���h�L���[(dx12�Ɏ��s���������R�}���h�����߂Ă�������)
	CreateCommandQueue();
	//�X���b�v�`�F�C��(�t�����g�o�b�t�@�ƃo�b�N�o�b�t�@�̐؂�ւ�)
	CreateSwapChain(hwnd);
	//�����_�[�^�[�Q�b�g�r���[�͂ǂ̃o�b�t�@�ɕ`�悷�邩�̕R�Â���S��
	CreateRenderTargetView();
	//�[�x�X�e���V���o�b�t�@(�����Ȃ��s�N�Z���̏������Ȃ����)
	CreateDepthStencilBuffer();
	//���s����R�}���h���X�g(1�̃R�}���h�L���[�ɑ΂���2�ȂǁA�L���[�Ƒg�ݍ��킹�Ă��悢���s������ڎw��)
	CreateCommandList();
	//���\�[�X�̃o�C���h���Ǘ�����V�O�l�`��
	CreateRootSignature();
	//���̓X�g���[���A�V�F�[�_�A�X�e�[�g�������ׂĂЂƂ̃I�u�W�F�N�g�̂܂Ƃ߂�����
	CreatePipelineStateObject();

	viewport_.TopLeftX = 0.f;
	viewport_.TopLeftY = 0.f;
	viewport_.Width = (FLOAT)window_width_;
	viewport_.Height = (FLOAT)window_height_;
	viewport_.MinDepth = 0.f;
	viewport_.MaxDepth = 1.f;

	scissor_rect_.top = 0;
	scissor_rect_.left = 0;
	scissor_rect_.right = window_width_;
	scissor_rect_.bottom = window_height_;

	plane_.Initialize(device_.Get());
	sphere_.Initialize(device_.Get(), pipeline_state_.Get());
	mesh_.Init(device_.Get(), "Chips.obj", command_allocator_.Get(), command_queue_.Get(), command_list_.Get(), queue_fence_.Get(), 1, pipeline_state_.Get());
}

D3D12Manager::~D3D12Manager() {}


//�t�@�N�g���̍쐬
HRESULT D3D12Manager::CreateFactory() {
	HRESULT hr{};
	UINT flag{};

	//�f�o�b�O���[�h�̏ꍇ�̓f�o�b�O���C���[��L���ɂ���
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));
		if (FAILED(hr)) {
			return hr;
		}
		debug->EnableDebugLayer();
	}

	flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	//�t�@�N�g���̍쐬
	hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(factory_.GetAddressOf()));

	return hr;
}

//�f�o�C�X�̐���
HRESULT D3D12Manager::CreateDevice() {
	HRESULT hr{};

	IDXGIAdapter*	warpAdapter;
	factory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
	D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_));
	warpAdapter->Release();

	return hr;
}

//�R�}���h�L���[�𐶐�
HRESULT D3D12Manager::CreateCommandQueue() {
	HRESULT hr{};
	D3D12_COMMAND_QUEUE_DESC command_queue_desc{};

	// �R�}���h�L���[�𐶐�.
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Priority = 0;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.NodeMask = 0;
	hr = device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(command_queue_.GetAddressOf()));
	if (FAILED(hr)) {
		return hr;
	}

	//�R�}���h�L���[�p�̃t�F���X�̐���
	fence_event_ = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (fence_event_ == NULL) {
		return E_FAIL;
	}
	hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(queue_fence_.GetAddressOf()));

	return hr;
}

//�X���b�v�`�F�C���̍쐬
HRESULT D3D12Manager::CreateSwapChain(HWND hWnd) {
	HRESULT hr{};
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};

	swap_chain_desc.Width = window_width_;
	swap_chain_desc.Height = window_height_;
	swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	//swap_chain_desc.OutputWindow = window_handle_;
	//swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = RTV_NUM;//���ƕ\�łQ����肽���̂�2���w�肷��
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	/*swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;

	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	*/
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;


	hr = factory_->CreateSwapChainForHwnd(command_queue_.Get(), hWnd, &swap_chain_desc, NULL, NULL, (IDXGISwapChain1**)swap_chain_.GetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	/*hr = swap_chain->QueryInterface(swap_chain_.GetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}*/

	//�J�����g�̃o�b�N�o�b�t�@�̃C���f�b�N�X���擾����
	rtv_index_ = swap_chain_->GetCurrentBackBufferIndex();

	return S_OK;
}

//�����_�[�^�[�Q�b�g�r���[�����
HRESULT D3D12Manager::CreateRenderTargetView() {
	HRESULT hr{};
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};

	//RTV�p�f�X�N���v�^�q�[�v�̍쐬
	heap_desc.NumDescriptors = RTV_NUM;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_desc.NodeMask = 0;
	hr = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(dh_rtv_.GetAddressOf()));
	if (FAILED(hr)) {
		return hr;
	}

	//�����_�[�^�[�Q�b�g���쐬���鏈��
	rtvDescriptorSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�t���[���o�b�t�@�ƃo�b�N�o�b�t�@���ꂼ��Ƀ����_�[�^�[�Q�b�g�r���[���쐬
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dh_rtv_->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < RTV_NUM; n++)
	{
		swap_chain_->GetBuffer(n, IID_PPV_ARGS(&render_target_[n]));
		device_->CreateRenderTargetView(render_target_[n].Get(), NULL, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}
	/*for (UINT i = 0; i < RTV_NUM; ++i) {
		//�X���b�v�`�F�C������o�b�t�@���󂯎��
		hr = swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_target_[i]));
		if (FAILED(hr)) {
			return hr;
		}

		//RenderTargetView���쐬���ăq�[�v�f�X�N���v�^�ɓo�^
		rtv_handle_[i] = dh_rtv_->GetCPUDescriptorHandleForHeapStart();
		rtv_handle_[i].ptr += size * i;
		device_->CreateRenderTargetView(render_target_[i].Get(), nullptr, rtv_handle_[i]);
		rtv_handle_[i].Offset(1, size);
	}*/

	return hr;
}

//�[�x�o�b�t�@�쐬
HRESULT D3D12Manager::CreateDepthStencilBuffer() {
	HRESULT hr;

	//�[�x�o�b�t�@�p�̃f�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{};
	descriptor_heap_desc.NumDescriptors = 1;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptor_heap_desc.NodeMask = 0;
	hr = device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&dh_dsv_));
	if (FAILED(hr)) {
		return hr;
	}


	//�[�x�o�b�t�@�̍쐬
	D3D12_HEAP_PROPERTIES heap_properties{};
	D3D12_RESOURCE_DESC resource_desc{};
	D3D12_CLEAR_VALUE clear_value{};

	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_properties.CreationNodeMask = 0;
	heap_properties.VisibleNodeMask = 0;

	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Width = window_width_;
	resource_desc.Height = window_height_;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 0;
	resource_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	clear_value.DepthStencil.Depth = 1.0f;
	clear_value.DepthStencil.Stencil = 0;

	hr = device_->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&depth_buffer_));
	if (FAILED(hr)) {
		return hr;
	}


	//�[�x�o�b�t�@�̃r���[�̍쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};

	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

	dsv_handle_ = dh_dsv_->GetCPUDescriptorHandleForHeapStart();

	device_->CreateDepthStencilView(depth_buffer_.Get(), &dsv_desc, dsv_handle_);

	return hr;
}

//�R�}���h���X�g�쐬
HRESULT D3D12Manager::CreateCommandList() {
	HRESULT hr;

	//�R�}���h�A���P�[�^�̍쐬
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));
	if (FAILED(hr)) {
		return hr;
	}

	//�R�}���h�A���P�[�^�ƃo�C���h���ăR�}���h���X�g���쐬����
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));
	command_list_->Close();

	return hr;
}


//���[�g�V�O�l�`��(�f�B�X�N���v�^�[�e�[�u�����Ǘ��������)�����
HRESULT D3D12Manager::CreateRootSignature() {
	HRESULT hr{};
	CD3DX12_DESCRIPTOR_RANGE1		ranges[3];
	CD3DX12_ROOT_PARAMETER1		root_parameters[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	root_parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC	root_signature_desc;
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters,
		0, NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	//�T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC	sampler_desc{};
	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.ShaderRegister = 0;
	sampler_desc.RegisterSpace = 0;
	sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ComPtr<ID3DBlob> blob{};
	hr = D3DX12SerializeVersionedRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
	if (FAILED(hr)) {
		return hr;
	}
	hr = device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_sugnature_));


	//�T���v���̐ݒ�
	/*
	D3D12_DESCRIPTOR_RANGE		range[1]{};
	D3D12_ROOT_PARAMETER		root_parameters[2]{};
	D3D12_ROOT_SIGNATURE_DESC	root_signature_desc{};
	D3D12_STATIC_SAMPLER_DESC	sampler_desc{};
	ComPtr<ID3DBlob> blob{};

	//�萔�o�b�t�@���f�X�N���v�^������ɓn���悤�ɐݒ�
	root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[0].Descriptor.ShaderRegister = 0;
	root_parameters[0].Descriptor.RegisterSpace = 0;

	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.ShaderRegister = 0;
	sampler_desc.RegisterSpace = 0;
	sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	root_signature_desc.NumParameters = _countof(root_parameters);
	root_signature_desc.pParameters = root_parameters;
	root_signature_desc.NumStaticSamplers = 1;
	root_signature_desc.pStaticSamplers = &sampler_desc;*/

	return hr;
}

//�p�C�v���C���X�e�[�g�I�u�W�F�N�g
HRESULT D3D12Manager::CreatePipelineStateObject() {
	HRESULT hr;
	/*ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	D3DCompileFromFile(L"Simple.hlsl", NULL, NULL, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	D3DCompileFromFile(L"Simple.hlsl", NULL, NULL, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);



	// ���_���C�A�E�g.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{};
	pipeline_state_desc.InputLayout = { InputElementDesc, _countof(InputElementDesc) };
	pipeline_state_desc.pRootSignature = root_sugnature_.Get();;
	pipeline_state_desc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	pipeline_state_desc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	pipeline_state_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	pipeline_state_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipeline_state_desc.DepthStencilState.DepthEnable = FALSE;
	pipeline_state_desc.DepthStencilState.StencilEnable = FALSE;
	pipeline_state_desc.SampleMask = UINT_MAX;
	pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_state_desc.NumRenderTargets = 1;
	pipeline_state_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline_state_desc.SampleDesc.Count = 1;
	*/
	
	ComPtr<ID3DBlob> vertex_shader{};
	ComPtr<ID3DBlob> pixel_shader{};


#if defined(_DEBUG)
	UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flag = 0;
#endif

	hr = D3DCompileFromFile(L"Simple.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compile_flag, 0, vertex_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = D3DCompileFromFile(L"Simple.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compile_flag, 0, pixel_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{};

	// ���_���C�A�E�g.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//�V�F�[�_�[�̐ݒ�
	pipeline_state_desc.VS.pShaderBytecode = vertex_shader->GetBufferPointer();
	pipeline_state_desc.VS.BytecodeLength = vertex_shader->GetBufferSize();
	pipeline_state_desc.PS.pShaderBytecode = pixel_shader->GetBufferPointer();
	pipeline_state_desc.PS.BytecodeLength = pixel_shader->GetBufferSize();


	//�C���v�b�g���C�A�E�g�̐ݒ�
	pipeline_state_desc.InputLayout.pInputElementDescs = InputElementDesc;
	pipeline_state_desc.InputLayout.NumElements = _countof(InputElementDesc);


	//�T���v���n�̐ݒ�
	pipeline_state_desc.SampleDesc.Count = 1;
	pipeline_state_desc.SampleDesc.Quality = 0;
	pipeline_state_desc.SampleMask = UINT_MAX;

	//�����_�[�^�[�Q�b�g�̐ݒ�
	pipeline_state_desc.NumRenderTargets = 1;
	pipeline_state_desc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;

	//�O�p�`�ɐݒ�
	pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;


	//���[�g�V�O�l�`��
	pipeline_state_desc.pRootSignature = root_sugnature_.Get();


	//���X�^���C�U�X�e�[�g�̐ݒ�
	pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	pipeline_state_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipeline_state_desc.RasterizerState.FrontCounterClockwise = FALSE;
	pipeline_state_desc.RasterizerState.DepthBias = 0;
	pipeline_state_desc.RasterizerState.DepthBiasClamp = 0;
	pipeline_state_desc.RasterizerState.SlopeScaledDepthBias = 0;
	pipeline_state_desc.RasterizerState.DepthClipEnable = TRUE;
	pipeline_state_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipeline_state_desc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipeline_state_desc.RasterizerState.MultisampleEnable = FALSE;


	//�u�����h�X�e�[�g�̐ݒ�
	for (int i = 0; i < _countof(pipeline_state_desc.BlendState.RenderTarget); ++i) {
		pipeline_state_desc.BlendState.RenderTarget[i].BlendEnable = FALSE;
		pipeline_state_desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		pipeline_state_desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		pipeline_state_desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		pipeline_state_desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		pipeline_state_desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		pipeline_state_desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		pipeline_state_desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipeline_state_desc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
		pipeline_state_desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
	}
	pipeline_state_desc.BlendState.AlphaToCoverageEnable = FALSE;
	pipeline_state_desc.BlendState.IndependentBlendEnable = FALSE;


	//�f�v�X�X�e���V���X�e�[�g�̐ݒ�
	pipeline_state_desc.DepthStencilState.DepthEnable = TRUE;								//�[�x�e�X�g����
	pipeline_state_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pipeline_state_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipeline_state_desc.DepthStencilState.StencilEnable = FALSE;							//�X�e���V���e�X�g�Ȃ�
	pipeline_state_desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	pipeline_state_desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	pipeline_state_desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipeline_state_desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipeline_state_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	
	hr = device_->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(&pipeline_state_));

	return hr;
}

HRESULT D3D12Manager::WaitForPreviousFrame() {
	HRESULT hr;

	const UINT64 fence = frames_;
	hr = command_queue_->Signal(queue_fence_.Get(), fence);
	if (FAILED(hr)) {
		return -1;
	}
	++frames_;

	if (queue_fence_->GetCompletedValue() < fence) {
		hr = queue_fence_->SetEventOnCompletion(fence, fence_event_);
		if (FAILED(hr)) {
			return -1;
		}

		WaitForSingleObject(fence_event_, INFINITE);
	}
	return S_OK;
}

HRESULT D3D12Manager::SetResourceBarrier(D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState) {
	D3D12_RESOURCE_BARRIER resource_barrier{};

	resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resource_barrier.Transition.pResource = render_target_[rtv_index_].Get();
	resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	resource_barrier.Transition.StateBefore = BeforeState;
	resource_barrier.Transition.StateAfter = AfterState;

	command_list_->ResourceBarrier(1, &resource_barrier);
	return S_OK;
}

HRESULT D3D12Manager::PopulateCommandList() {
	HRESULT hr;

	//Reset
	hr = command_allocator_->Reset();
	if (FAILED(hr)) {
		return hr;
	}
	hr = command_list_->Reset(command_allocator_.Get(), pipeline_state_.Get());
	if (FAILED(hr)) {
		return hr;
	}

	//���[�g�V�O�l�`���̐ݒ�
	command_list_->SetGraphicsRootSignature(root_sugnature_.Get());

	FLOAT clear_color[4] = { 0.0f, 0.0f, 0.20f, 1.0f };
	//�r���[�|�[�g�ƃV�U�[��`�̐ݒ�
	command_list_->RSSetViewports(1, &viewport_);
	command_list_->RSSetScissorRects(1, &scissor_rect_);

	//���\�[�X�̏�Ԃ��v���[���g�p���烌���_�[�^�[�Q�b�g�p�ɕύX
	command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target_[rtv_index_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dh_rtv_->GetCPUDescriptorHandleForHeapStart(), rtv_index_, rtvDescriptorSize);
	//�����_�[�^�[�Q�b�g�̐ݒ�
	command_list_->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsv_handle_);

	//�[�x�o�b�t�@�ƃ����_�[�^�[�Q�b�g�̃N���A
	command_list_->ClearDepthStencilView(dsv_handle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	command_list_->ClearRenderTargetView(rtvHandle, clear_color, 0, nullptr);

	//�|���̕`��
	//plane_.Draw(command_list_.Get());

	//���̕`��
	//sphere_.Draw(command_list_.Get());

	//���b�V���̕`��
	mesh_.Draw(command_list_.Get());

	//���\�[�X�̏�Ԃ������_�[�^�[�Q�b�g����v���[���g�p�ɕύX
	command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target_[rtv_index_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = command_list_->Close();

	return hr;
}

HRESULT D3D12Manager::Render() {
	HRESULT hr;
	//g_pCommandList->SetGraphicsRootSignature(g_pRootSignature);

	PopulateCommandList();

	ID3D12CommandList *const command_lists = command_list_.Get();
	command_queue_->ExecuteCommandLists(1, &command_lists);

	//���s�����R�}���h�̏I���҂�
	WaitForPreviousFrame();

	hr = swap_chain_->Present(1, 0);
	if (FAILED(hr)) {
		return hr;
	}

	//�J�����g�̃o�b�N�o�b�t�@�̃C���f�b�N�X���擾����
	rtv_index_ = swap_chain_->GetCurrentBackBufferIndex();

	//��������
	/*command_queue_->Signal(queue_fence_.Get(), frames_);
	while (queue_fence_->GetCompletedValue() < frames_);
	frames_++;
	*/
	return S_OK;
}
