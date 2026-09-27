#pragma once

#include <d3d11.h>
#include <string>

namespace YimMenu
{
	// Test-only crash-dummy mesh (E:/Downloads/crash_dummy/scene.gltf).
	// Parsed by hand for that exact file (single indexed mesh + baseColor PNG),
	// rendered flat/textured in DX11 behind the ESP overlay. Textures beyond
	// baseColor and skeletal animation are intentionally out of scope:
	// the mesh is a posing backdrop, the ESP skeleton carries the pose.
	class DummyMesh
	{
	public:
		DummyMesh()  = default;
		~DummyMesh() { Shutdown(); }

		DummyMesh(const DummyMesh&)            = delete;
		DummyMesh& operator=(const DummyMesh&) = delete;

		bool Load(ID3D11Device* device, const std::wstring& gltfPath);
		void Shutdown();

		bool IsLoaded() const
		{
			return m_IndexCount > 0;
		}

		// calibration, tweaked from the ESP Lab panel
		float m_Scale    = 1.f;
		float m_YawDeg   = 0.f;
		float m_ZOffset  = 0.f;
		bool m_Wireframe = false;
		bool m_Textured  = true;

		void Render(ID3D11DeviceContext* context, const float view[16], const float proj[16], float actorX, float actorY, float actorZ);

	private:
		bool LoadTexture(ID3D11Device* device, const std::wstring& pngPath);
		bool CreateShaders(ID3D11Device* device);

		ID3D11Buffer* m_VertexBuffer      = nullptr;
		ID3D11Buffer* m_IndexBuffer       = nullptr;
		ID3D11Buffer* m_ConstantBuffer    = nullptr;
		ID3D11VertexShader* m_VertexShader = nullptr;
		ID3D11PixelShader* m_PixelShader   = nullptr;
		ID3D11InputLayout* m_InputLayout   = nullptr;
		ID3D11ShaderResourceView* m_Texture = nullptr;
		ID3D11SamplerState* m_Sampler       = nullptr;
		ID3D11RasterizerState* m_RsSolid     = nullptr;
		ID3D11RasterizerState* m_RsWire      = nullptr;
		UINT m_IndexCount                   = 0;
	};
}
