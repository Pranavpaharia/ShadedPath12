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

	// set max number of objects allowed in the store (is guarded by assertions)
	void setMaxObjectCount(unsigned int);
	void update();  // update all objects - CBV is complete after this
	void draw();	// draw all objects
	// uploading mesh data to GPU can only be done in GpuUploadPhase
	// this prevents unnecessary waits between upload requests
	void gpuUploadPhaseStart() { inGpuUploadPhase = true; };
	void gpuUploadPhaseEnd();

	// objects
	// load object definition from .b file, save under given hash name
	void loadObject(wstring filename, string id, float scale = 1.0f, XMFLOAT3 *displacement = nullptr);
	// add loaded object to scene
	void addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid = 0);
	//void addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid = 0);
	// object groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	const vector<unique_ptr<WorldObject>> *getGroup(string groupname);
	// draw all objects within a group (all have same mesh), set threadNum > 1 to draw with multiple threads
	void drawGroup(string groupname, size_t threadNum = 0);
	void setWorldObjectEffect(WorldObjectEffect *objectEffect);
	void forAll(std::function<void ()> func);
private:
	unordered_map<string, vector<unique_ptr<MeshObject>>> groups;
	unordered_map<string, Mesh> meshes;

	MeshObjectStore() {};									// prevent creation outside this class
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
	struct CBV {
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 world;  // needed for normal calculations
		XMFLOAT3   cameraPos;
		float    alpha;
	};
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
		VR_Eyes vr_eyesm[2];
		bool initialized;
	};
	MeshObjectFrameResource frameEffectData[XApp::FrameCount];
};