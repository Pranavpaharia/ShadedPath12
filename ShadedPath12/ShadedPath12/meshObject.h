/*
 * MeshObject and Store - central classes for rendering and handling meshed based objects inside the virtual world
 *
 * Design principles: 
 *  Use on global constant buffer for all objects, allocated only once at effect intialization
 *  You cannot use more objects that specfied at effect initialization (guarded by assertions)
 *  Constant buffer should be fully set after update() phase
 *  all objects are updated and drawn through the store - no update() and draw() calls on the individial objects
 *  store is also the effect - using different classes like in WorldObject only caused overhead
 *  constant buffer is duplicated in size for VR mode, with first left eye constants, followed by right eye constants
 *  cbv[0] is always reserved for global data the shader code might need, actual objects start with index == 1
 */

class MeshObject {
public:
	MeshObject();
	virtual ~MeshObject();
	//XMFLOAT4 pos;
	XMFLOAT3& pos();
	XMFLOAT3& rot();
	XMFLOAT4 rot_quaternion;
	float scale = 1.0f;
	Material material;
	BoundingBox aabb;
	bool drawBoundingBox = false;
	bool drawNormals = false;
	bool useQuaternionRotation = false;
	XMMATRIX calcToWorld();
	static XMMATRIX calcToWorld(XMFLOAT3 pos, XMFLOAT3 rot, bool useQuaternionRotation = false, XMFLOAT4 *rot_quaternion = nullptr);
	float alpha = 1.0f;
	unsigned int objectNum = 0;	// 0 indicates not properly added object
	Mesh *mesh;
	TextureID textureID;
	ComPtr<ID3D12GraphicsCommandList> bundleCommandLists[XApp::FrameCount];
	ComPtr<ID3D12CommandAllocator> bundleCommandAllocators[XApp::FrameCount];
	bool uploadedToGPU[XApp::FrameCount]; // keep track if object needs upload
	// mark object as needing upload on next draw cycles (for all frames)
	void flagUploadToGPU() {
		for (int i = 0; i < XApp::FrameCount; i++) {
			uploadedToGPU[i] = false;
		}
	};
	bool drawBundleAvailable = false;
private:
	XMFLOAT3 _pos;
	XMFLOAT3 _rot;
};

// MeshObject Store:
class MeshObjectStore : public EffectBase {
public:
	//singleton:
	static MeshObjectStore *getStore() {
		static MeshObjectStore singleton;
		return &singleton;
	};

	struct CBV {
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 world;  // needed for normal calculations
		XMFLOAT3   cameraPos;
		float    alpha;
		Material material;
		float fill[20];
	};
	struct CBVDebug {
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 world;  // needed for normal calculations
		XMFLOAT4X4 vp;         // 128
		XMFLOAT3   cameraPos;
		float    alpha;
		Material material;
		float fill[4];
	};

	// compute shader constant buffer ==> keep in sync with MObjectCS.hlsl/cbvCS
	struct CBV_CS {
		XMFLOAT4X4	vp;
		UINT        num_objects;
		UINT        start_objects;
		float		fill[2];
	};

	struct BulkDivideInfoExt : BulkDivideInfo {
		MeshObject *mo;
	};

	// set max number of objects allowed in the store (is guarded by assertions)
	void setMaxObjectCount(unsigned int);
	void update();  // update all objects - CBV is complete after this
	void draw();	// draw all objects
	void updateOne(CBV const *cbv, MeshObject * mo, XMMATRIX vp, int frameIndex, int eyeNum = 0);
	void updatePart(BulkDivideInfo bi, CBV *cbv, vector<unique_ptr<MeshObject>> *mov, XMMATRIX vp, int frameIndex, int eyeNum = 0);
	// uploading mesh data to GPU can only be done in GpuUploadPhase
	// this prevents unnecessary waits between upload requests
	void gpuUploadPhaseStart() { inGpuUploadPhase = true; };
	void gpuUploadPhaseEnd();
	void updateMaterialOnNextFrame();

	// objects
	// load object definition from .b file, save under given hash name
	void loadObject(wstring filename, string id, float scale = 1.0f, XMFLOAT3 *displacement = nullptr);
	// add loaded object to scene
	MeshObject* addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid = 0);
	//void addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid = 0);
	// object groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	const vector<unique_ptr<MeshObject>> *getGroup(string groupname);
	// draw all objects within a group (all have same mesh), set threadNum > 1 to draw with multiple threads
	void drawGroup(string groupname, size_t threadNum = 0);
	void setWorldObjectEffect(WorldObjectEffect *objectEffect);
	// lambda for acting on all objects in the store
	// call like this: 		forAll([](MeshObject *mo) { Log("  elem: " << mo->pos().x << endl);});
	void forAll(std::function<void (MeshObject*)> func);
	void createDrawBundle(MeshObject *meshObject);
	// check that objects are well sorted (with regards to used mesh) and no holes exist with unused object ids
	bool isSorted();
private:
	unordered_map<string, vector<unique_ptr<MeshObject>>> groups;
	unordered_map<string, Mesh> meshes;

	MeshObjectStore() : dxManager(XApp::FrameCount) {};		// prevent creation outside this class
	MeshObjectStore(const MeshObjectStore&);				// prevent creation via copy-constructor
	MeshObjectStore & operator = (const MeshObjectStore &);	// prevent instance copies

	bool inGpuUploadPhase = false;	// signal that this effect is uploading data to GPU
	unsigned int used_objects = 0;	// counter to get total number of used objects and next available index

// effect methods / members:
public:
	void init();
	void createAndUploadVertexAndIndexBuffer(Mesh *mesh);
	void createRootSigAndPSO(ComPtr<ID3D12RootSignature>& sig, ComPtr<ID3D12PipelineState>& pso);
	XMMATRIX calcWVP(XMMATRIX & toWorld, XMMATRIX & vp);
	// algorithm to divide a any number of problems into equally sized sub-problems
	// the result vector holds BulkDivideInfos that give start and end positions for each sub problem
	void divideBulk(size_t numObjects, size_t numParallel, vector<BulkDivideInfo> &subProblems);
	// prepare drawing same meshes together, needs to be called after objects have been added to the store (ideally at end of init)
	// auto called at gpuUploadPhaseEnd()
	// not synchronized: if called during update cycle frame synchronization needs to be done outside
	void divideDrawBulks();
private:
	CBV cbv;	// only used for convenience - all CBVs are in buffer array
	// globally enable wireframe display of objects
	bool wireframe = false;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	FrameResource updateFrameData;
	UINT cbvAlignedSize = 0;	// aligned size of cbv for using indexes into larger buffers (256 byte alignment)

	// all data that needs to be frame local. No sync calls should be necessary
	class MeshObjectFrameResource {
	public:
		//ComPtr<ID3D12GraphicsCommandList> commandList; //already defined in effectbase
		//ComPtr<ID3D12CommandAllocator> commandAllocator; //already defined in effectbase
		Camera camera;
		Camera cameram[2];
		CBV cbv;
		CBV cbvm[2];
		CBV_CS cbvCS;
		VR_Eyes vr_eyesm[2];
		bool initialized;
		bool updateMaterial = false;  // set to true once for first frame, then only after material changes
		bool updateConstBuffers = true; // usually const buffers need to be updated only once, after that the compute shader will do all the calculations
	};
	MeshObjectFrameResource frameEffectData[XApp::FrameCount];
	void preDraw();
	void postDraw();
	void drawInternal(MeshObject *mo, int eyeNum = 0);
	void drawInternal(BulkDivideInfoExt *bi, int eyeNum = 0);
	vector<BulkDivideInfo> bulkInfos;
	vector<BulkDivideInfoExt> drawBulkInfos;

	// indirect drawing
	//ComPtr<ID3D12CommandSignature> commandSignature;
	//// Data structure to match the command signature used for ExecuteIndirect.
	//struct IndirectCommand
	//{
	//	D3D12_GPU_VIRTUAL_ADDRESS cbv;  // matrices
	//	D3D12_GPU_VIRTUAL_ADDRESS srv;	// texture
	//	D3D12_DRAW_ARGUMENTS drawArguments;
	//};
	//// Graphics root signature parameter offsets.
	//// counted 0, 1, 2...
	//enum GraphicsRootParameters
	//{
	//	Cbv,		// cbv with matrices
	//	CbvLights,  // cbv with light info
	//	Srv,		// srv with texture
	//	GraphicsRootParametersCount
	//};
	// Compute objects.
	ComPtr<ID3D12PipelineState> computePipelineState;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12CommandAllocator> computeAllocator[XApp::FrameCount];
	ComPtr<ID3D12CommandQueue> computeCommandQueue[XApp::FrameCount];
	ComPtr<ID3D12GraphicsCommandList> computeCommandList[XApp::FrameCount];
	void computeMethod(UINT frameNum);
	DXManager dxManager;
	ResourceStateHelper *resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
};