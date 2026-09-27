#include "DummyMesh.hpp"

#include <cmath>
#include <cstdint>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>
#include <wincodec.h>

namespace YimMenu
{
	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;

		struct GltfVertex
		{
			float px, py, pz;
			float nx, ny, nz;
			float u, v;
		};

		struct CbFrame
		{
			float wvp[16];
			float world[16];
			float lightDir[4];
			float useTex;
			float pad[3];
		};

		const char* kVertexShader = R"(
cbuffer CbFrame : register(b0)
{
    row_major float4x4 gWvp;
    row_major float4x4 gWorld;
    float4 gLightDir;
    float gUseTex;
    float3 gPad;
};
struct VSIn { float3 pos : POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float3 nrm : NORMAL0; float2 uv : TEXCOORD0; };
VSOut main(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1.0), gWvp);
    o.nrm = normalize(mul(i.nrm, (float3x3)gWorld));
    o.uv  = i.uv;
    return o;
}
)";

		const char* kPixelShader = R"(
Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);
cbuffer CbFrame : register(b0)
{
    row_major float4x4 gWvp;
    row_major float4x4 gWorld;
    float4 gLightDir;
    float gUseTex;
    float3 gPad;
};
struct PSIn { float4 pos : SV_POSITION; float3 nrm : NORMAL0; float2 uv : TEXCOORD0; };
float4 main(PSIn i) : SV_TARGET
{
    float4 albedo = gUseTex > 0.5 ? gTex.Sample(gSamp, i.uv) : float4(0.72, 0.70, 0.66, 1.0);
    float diff = saturate(dot(normalize(i.nrm), -gLightDir.xyz)) * 0.65 + 0.35;
    return float4(albedo.rgb * diff, 1.0);
}
)";

		bool CompileShader(const char* source, const char* entry, const char* target, ID3DBlob** outBlob)
		{
			ID3DBlob* errors = nullptr;
			const HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entry, target,
			    D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, outBlob, &errors);
			if (errors)
				errors->Release();
			return SUCCEEDED(hr) && *outBlob;
		}

		void Mul44(const float a[16], const float b[16], float out[16])
		{
			for (int r = 0; r < 4; ++r)
				for (int c = 0; c < 4; ++c)
					out[r * 4 + c] = a[r * 4] * b[c] + a[r * 4 + 1] * b[4 + c] + a[r * 4 + 2] * b[8 + c] + a[r * 4 + 3] * b[12 + c];
		}

		const float* ReadFloats(const nlohmann::json& accessor, const std::vector<std::vector<uint8_t>>& buffers,
		    const nlohmann::json& bufferViews, const nlohmann::json& buffersJson, size_t& outCount)
		{
			const size_t viewIdx   = accessor["bufferView"];
			const auto& view       = bufferViews[viewIdx];
			const size_t bufIdx    = view["buffer"];
			const size_t byteOff   = view.value("byteOffset", 0) + accessor.value("byteOffset", 0);
			outCount               = accessor["count"];
			return reinterpret_cast<const float*>(buffers[bufIdx].data() + byteOff);
		}
	}

	bool DummyMesh::LoadTexture(ID3D11Device* device, const std::wstring& pngPath)
	{
		IWICImagingFactory* factory = nullptr;
		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
			return false;

		IWICBitmapDecoder* decoder = nullptr;
		bool ok                    = SUCCEEDED(factory->CreateDecoderFromFilename(pngPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* converter = nullptr;
		if (ok)
			ok = SUCCEEDED(decoder->GetFrame(0, &frame));
		if (ok)
			ok = SUCCEEDED(factory->CreateFormatConverter(&converter));
		if (ok)
			ok = SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom));

		UINT w = 0, h = 0;
		if (ok)
			ok = SUCCEEDED(converter->GetSize(&w, &h)) && w > 0 && h > 0;
		std::vector<uint8_t> pixels;
		if (ok)
		{
			pixels.resize((size_t)w * h * 4);
			ok = SUCCEEDED(converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data()));
		}

		if (ok)
		{
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width          = w;
			desc.Height         = h;
			desc.MipLevels      = 1;
			desc.ArraySize      = 1;
			desc.Format         = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc     = {1, 0};
			desc.Usage          = D3D11_USAGE_DEFAULT;
			desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem     = pixels.data();
			data.SysMemPitch = w * 4;
			ID3D11Texture2D* tex = nullptr;
			ok                   = SUCCEEDED(device->CreateTexture2D(&desc, &data, &tex));
			if (ok)
			{
				ok = SUCCEEDED(device->CreateShaderResourceView(tex, nullptr, &m_Texture));
				tex->Release();
			}
		}

		if (converter)
			converter->Release();
		if (frame)
			frame->Release();
		if (decoder)
			decoder->Release();
		factory->Release();
		return ok;
	}

	bool DummyMesh::CreateShaders(ID3D11Device* device)
	{
		ID3DBlob* vsBlob = nullptr;
		ID3DBlob* psBlob = nullptr;
		if (!CompileShader(kVertexShader, "main", "vs_4_0", &vsBlob))
			return false;
		bool ok = SUCCEEDED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VertexShader));
		if (ok)
		{
			D3D11_INPUT_ELEMENT_DESC layout[] = {
			    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};
			ok = SUCCEEDED(device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_InputLayout));
		}
		vsBlob->Release();
		if (ok && !CompileShader(kPixelShader, "main", "ps_4_0", &psBlob))
			ok = false;
		if (ok)
			ok = SUCCEEDED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PixelShader));
		if (psBlob)
			psBlob->Release();
		if (!ok)
			return false;

		D3D11_BUFFER_DESC cbDesc{};
		cbDesc.ByteWidth      = sizeof(CbFrame);
		cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &m_ConstantBuffer)))
			return false;

		D3D11_SAMPLER_DESC samp{};
		samp.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		if (FAILED(device->CreateSamplerState(&samp, &m_Sampler)))
			return false;

		D3D11_RASTERIZER_DESC rs{};
		rs.FillMode = D3D11_FILL_SOLID;
		rs.CullMode = D3D11_CULL_BACK;
		if (FAILED(device->CreateRasterizerState(&rs, &m_RsSolid)))
			return false;
		rs.FillMode = D3D11_FILL_WIREFRAME;
		rs.CullMode = D3D11_CULL_NONE;
		if (FAILED(device->CreateRasterizerState(&rs, &m_RsWire)))
			return false;
		return true;
	}

	bool DummyMesh::Load(ID3D11Device* device, const std::wstring& gltfPath)
	{
		Shutdown();

		std::filesystem::path base(gltfPath);
		base.remove_filename();

		std::ifstream file(gltfPath);
		if (!file)
			return false;
		nlohmann::json gltf;
		try
		{
			file >> gltf;
		}
		catch (...)
		{
			return false;
		}

		// buffers (external .bin only)
		std::vector<std::vector<uint8_t>> buffers;
		for (const auto& b : gltf["buffers"])
		{
			const std::string uri = b["uri"];
			if (uri.rfind("data:", 0) == 0)
				return false;
			std::ifstream bin(base / uri, std::ios::binary);
			if (!bin)
				return false;
			buffers.emplace_back(std::istreambuf_iterator<char>(bin), std::istreambuf_iterator<char>());
		}

		const auto& bufferViews = gltf["bufferViews"];
		const auto& accessors   = gltf["accessors"];
		const auto& meshes      = gltf["meshes"];
		if (meshes.empty() || meshes[0]["primitives"].empty())
			return false;
		const auto& prim = meshes[0]["primitives"][0];
		if (!prim.contains("indices") || !prim["attributes"].contains("POSITION") || !prim["attributes"].contains("NORMAL")
		    || !prim["attributes"].contains("TEXCOORD_0"))
			return false;

		size_t vtxCount = 0, nrmCount = 0, uvCount = 0;
		const size_t posIdx = prim["attributes"]["POSITION"];
		const size_t nrmIdx = prim["attributes"]["NORMAL"];
		const size_t uvIdx  = prim["attributes"]["TEXCOORD_0"];
		const float* pos = ReadFloats(accessors[posIdx], buffers, bufferViews, gltf["buffers"], vtxCount);
		const float* nrm = ReadFloats(accessors[nrmIdx], buffers, bufferViews, gltf["buffers"], nrmCount);
		const float* uv  = ReadFloats(accessors[uvIdx], buffers, bufferViews, gltf["buffers"], uvCount);
		if (!pos || !nrm || !uv || vtxCount != nrmCount || vtxCount != uvCount || vtxCount == 0)
			return false;

		const size_t idxAccIdx = prim["indices"];
		const auto& idxAcc   = accessors[idxAccIdx];
		const size_t idxView = idxAcc["bufferView"];
		const auto& idxViewJson = bufferViews[idxView];
		const size_t idxBuf     = idxViewJson["buffer"];
		const size_t idxOff     = idxViewJson.value("byteOffset", 0) + idxAcc.value("byteOffset", 0);
		const size_t idxCount   = idxAcc["count"];
		const int idxType       = idxAcc["componentType"]; // 5123 ushort, 5125 uint
		if (idxType != 5123 && idxType != 5125)
			return false;

		// glTF Y-up centimeters -> world Z-up meters, feet on z=0, facing +Y
		std::vector<GltfVertex> verts;
		verts.reserve(vtxCount);
		for (size_t i = 0; i < vtxCount; ++i)
		{
			GltfVertex v{};
			v.px = pos[i * 3] * 0.01f;
			v.py = pos[i * 3 + 2] * 0.01f;
			v.pz = (pos[i * 3 + 1] + 182.f) * 0.01f;
			v.nx = nrm[i * 3];
			v.ny = nrm[i * 3 + 2];
			v.nz = nrm[i * 3 + 1];
			v.u  = uv[i * 2];
			v.v  = uv[i * 2 + 1];
			verts.push_back(v);
		}

		std::vector<uint32_t> indices;
		indices.reserve(idxCount);
		const uint8_t* idxBase = buffers[idxBuf].data() + idxOff;
		for (size_t i = 0; i < idxCount; ++i)
			indices.push_back(idxType == 5125 ? reinterpret_cast<const uint32_t*>(idxBase)[i] : reinterpret_cast<const uint16_t*>(idxBase)[i]);

		D3D11_BUFFER_DESC vbDesc{};
		vbDesc.ByteWidth = (UINT)(verts.size() * sizeof(GltfVertex));
		vbDesc.Usage     = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA vbData{verts.data(), 0, 0};
		if (FAILED(device->CreateBuffer(&vbDesc, &vbData, &m_VertexBuffer)))
			return false;

		D3D11_BUFFER_DESC ibDesc{};
		ibDesc.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
		ibDesc.Usage     = D3D11_USAGE_DEFAULT;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		D3D11_SUBRESOURCE_DATA ibData{indices.data(), 0, 0};
		if (FAILED(device->CreateBuffer(&ibDesc, &ibData, &m_IndexBuffer)))
		{
			Shutdown();
			return false;
		}
		m_IndexCount = (UINT)indices.size();

		// baseColor texture (optional: flat color fallback)
		try
		{
			const auto& mat = gltf["materials"][0]["pbrMetallicRoughness"];
			const int texIdx = mat["baseColorTexture"]["index"];
			const int imgIdx = gltf["textures"][texIdx]["source"];
			const std::string imgUri = gltf["images"][imgIdx]["uri"];
			std::filesystem::path imgPath = base / imgUri;
			LoadTexture(device, imgPath.wstring());
		}
		catch (...)
		{
		}

		if (!CreateShaders(device))
		{
			Shutdown();
			return false;
		}
		return true;
	}

	void DummyMesh::Render(ID3D11DeviceContext* context, const float view[16], const float proj[16], float actorX, float actorY, float actorZ)
	{
		if (!IsLoaded() || !m_VertexShader)
			return;

		const float yawR = m_YawDeg * kPi / 180.f;
		const float c = std::cos(yawR) * m_Scale, s = std::sin(yawR) * m_Scale;
		float world[16] = {c, 0.f, -s, 0.f, 0.f, m_Scale, 0.f, 0.f, s, 0.f, c, 0.f, actorX, actorY, actorZ + m_ZOffset, 1.f};

		float wv[16], wvp[16];
		Mul44(world, view, wv);
		Mul44(wv, proj, wvp);

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(context->Map(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			auto* cb = static_cast<CbFrame*>(mapped.pData);
			memcpy(cb->wvp, wvp, sizeof(wvp));
			memcpy(cb->world, world, sizeof(world));
			cb->lightDir[0] = 0.4f;
			cb->lightDir[1] = -0.5f;
			cb->lightDir[2] = -0.75f;
			cb->lightDir[3] = 0.f;
			cb->useTex      = (m_Textured && m_Texture) ? 1.f : 0.f;
			context->Unmap(m_ConstantBuffer, 0);
		}

		const UINT stride = sizeof(GltfVertex), offset = 0;
		context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->IASetInputLayout(m_InputLayout);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->VSSetShader(m_VertexShader, nullptr, 0);
		context->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);
		context->PSSetShader(m_PixelShader, nullptr, 0);
		context->PSSetConstantBuffers(0, 1, &m_ConstantBuffer);
		if (m_Textured && m_Texture)
		{
			context->PSSetShaderResources(0, 1, &m_Texture);
			context->PSSetSamplers(0, 1, &m_Sampler);
		}
		else
		{
			ID3D11ShaderResourceView* nullSrv = nullptr;
			context->PSSetShaderResources(0, 1, &nullSrv);
		}
		context->RSSetState(m_Wireframe ? m_RsWire : m_RsSolid);
		context->DrawIndexed(m_IndexCount, 0, 0);
	}

	void DummyMesh::Shutdown()
	{
		if (m_VertexBuffer)
		{
			m_VertexBuffer->Release();
			m_VertexBuffer = nullptr;
		}
		if (m_IndexBuffer)
		{
			m_IndexBuffer->Release();
			m_IndexBuffer = nullptr;
		}
		if (m_ConstantBuffer)
		{
			m_ConstantBuffer->Release();
			m_ConstantBuffer = nullptr;
		}
		if (m_VertexShader)
		{
			m_VertexShader->Release();
			m_VertexShader = nullptr;
		}
		if (m_PixelShader)
		{
			m_PixelShader->Release();
			m_PixelShader = nullptr;
		}
		if (m_InputLayout)
		{
			m_InputLayout->Release();
			m_InputLayout = nullptr;
		}
		if (m_Texture)
		{
			m_Texture->Release();
			m_Texture = nullptr;
		}
		if (m_Sampler)
		{
			m_Sampler->Release();
			m_Sampler = nullptr;
		}
		if (m_RsSolid)
		{
			m_RsSolid->Release();
			m_RsSolid = nullptr;
		}
		if (m_RsWire)
		{
			m_RsWire->Release();
			m_RsWire = nullptr;
		}
		m_IndexCount = 0;
	}
}
