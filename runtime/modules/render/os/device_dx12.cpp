//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	device_dx12.cpp
///	@brief	device_dx12
#include	"pch.h"
#include	"device_dx12.h"

#if NOX_RENDER_DX12

#include	"definition_dx12.h"
#include	"device_dx12_internal.h"

void nox::render::os::DeviceDX12::Initialize()
{
	//	deviceの初期化
	{
		::UINT dxgi_factory_flags = 0;
#if NOX_DEVELOP
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		::IDXGIFactory4* dxgi_factory = nullptr;
		::HRESULT hr = ::CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory));
		NOX_ASSERT(SUCCEEDED(hr), u"Failed to create DXGI Factory");

		::IDXGIAdapter1* adapter = nullptr;
		for (::UINT adapter_index = 0; dxgi_factory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_index)
		{
			::DXGI_ADAPTER_DESC1 adapter_desc{};
			adapter->GetDesc1(&adapter_desc);
			if ((adapter_desc.Flags & ::DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
			{
				if (SUCCEEDED(::D3D12CreateDevice(adapter, ::D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device_))))
				{
					break;
				}
			}
			adapter->Release();
			adapter = nullptr;
		}
		dxgi_factory->Release();
		NOX_ASSERT(device_ != nullptr, u"Failed to create D3D12 Device");

	}
}

nox::IntrusivePtr<nox::render::VertexBuffer> nox::render::os::DeviceDX12::CreateVertexBuffer(const GraphicBufferDesc& desc, const GraphicBufferSubResourceDesc& sub_resource_desc)
{
	if (device_ == nullptr)
	{
		NOX_ASSERT(false, u"DeviceDX12 is not initialized");
		return nullptr;
	}
	if (desc.byte_size == 0)
	{
		NOX_ASSERT(false, u"Byte size must be greater than 0");
		return nullptr;
	}
	if (desc.usage_type == UsageType::Staging)
	{
		NOX_ASSERT(false, u"Staging usage type is not supported for vertex buffer");
		return nullptr;
	}
	return new VertexBufferDX12(nullptr, desc.byte_size, 0);
}

#endif // NOX_RENDER_DX12