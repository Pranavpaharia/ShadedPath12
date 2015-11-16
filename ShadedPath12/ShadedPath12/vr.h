#pragma once

enum EyePos { EyeLeft, EyeRight };

class XApp;

class VR {
public:
	VR(XApp *xapp);
	~VR();
	void init();
	void initFrame();
	void startFrame();
	void endFrame();
	EyePos getCurrentEye() { return curEye; };

	void prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect);
	D3D12_VIEWPORT *getViewport() { return &viewports[curEye]; };
	D3D12_RECT *getScissorRect() { return &scissorRects[curEye]; };
	// prepare VR draw: save current camera pos/look/up, viewport and scissor rect
	void prepareDraw();
	// undo the chages to the camera made by prepareDraw
	void endDraw();
	// adjust the MVP matrix according to current eye position
	void adjustEyeMatrix(XMMATRIX &m);
	// move to 2nd eye
	void nextEye();
	// return if this run is for the first eye - some initializations are not needed for 2nd eye
	bool isFirstEye();

	bool enabled = false;  // default: VR is off, switch on by command line option -vr
protected:
	EyePos curEye = EyeLeft;
private:
	D3D12_VIEWPORT viewports[2];
	D3D12_RECT scissorRects[2];
	//D3D12_VIEWPORT orig_viewport;
	//D3D12_RECT orig_scissorRect;
	XApp* xapp;
	XMFLOAT4 cam_look, cam_up, cam_pos;
	bool firstEye = false;
};
