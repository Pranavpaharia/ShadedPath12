// world object effect
class WorldObjectEffect : public EffectBase {
public:
	struct ConstantBufferFixed {
		XMFLOAT4 color;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
		float    alpha;
	};

	void init();
	void prepare();
	// update cbuffer and vertex buffer
	void update();
	//void draw();
	void draw(ComPtr<ID3D11Buffer> &vertexBuffer, ComPtr<ID3D11Buffer> indexBuffer, XMFLOAT4X4 wvp, long numIndexes, ID3D11ShaderResourceView **srv, float alpha = 1.0f);

private:
	ConstantBufferFixed cb;
	// globally enable wireframe display of objects
	bool wireframe = false;

private:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw();
	void postDraw();
	CBV cbv;
	mutex mutex_Billboard;
	void drawInternal();
	void updateTask();
	atomic<bool> updateRunning = false;
	future<void> objecteffectFuture;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	FrameResource updateFrameData;
	ComPtr<ID3D12Resource> vertexBufferX;
	ComPtr<ID3D12Resource> vertexBufferUploadX;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewX;
};
