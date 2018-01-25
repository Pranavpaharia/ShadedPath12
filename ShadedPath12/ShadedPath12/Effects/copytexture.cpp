#include "stdafx.h"

void WorkerCopyTextureCommand::perform()
{
	auto res = frameResource;
	auto dxmanager = xapp->dxmanager;
	//Log("perform() copy texture command t = " << this_thread::get_id() << " frame " << res->frameNum << endl);
	Log("perform() copy texture command t = " << ThreadInfo::thread_osid() << " frame " << res->frameNum << endl);
	TextureInfo *tex = xapp->textureStore.getTexture(textureName);
	assert(tex->available);
	xapp->dxmanager.waitGPU(*frameResource, xapp->appWindow.commandQueue);
	resourceStateHelper->addOrKeep(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON);
	resourceStateHelper->addOrKeep(tex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// wait until GPU has finished with previous commandList
	//Sleep(30);
	//dxmanager->waitGPU(res, commandQueue); done in applicationwindow
	ID3D12GraphicsCommandList *commandList = res->commandList.Get();
	ThrowIfFailed(res->commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(res->commandAllocator.Get(), res->pipelineState.Get()));

	CD3DX12_TEXTURE_COPY_LOCATION src(tex->texSRV.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest(res->renderTarget.Get(), 0);
	CD3DX12_BOX box(0, 0, 512, 512);

	//commandList->CopyResource(renderTargets[frameNum].Get(), HouseTex->texSRV.Get());
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(res->rtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, res->rtvDescriptorSize);
	commandList->ClearRenderTargetView(rtvHandle, xapp->clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(res->dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	resourceStateHelper->toState(tex->texSRV.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandList);
	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &box);
	resourceStateHelper->toState(tex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, commandList);
	ThrowIfFailed(commandList->Close());
	RenderCommand rc;
	rc.commandList = commandList;
	rc.writesToSwapchain = true;
	rc.frameNum = res->frameNum; // TODO rename!!!
	rc.framenum = this->framenum;
	rc.frameResource = res;
	xapp->renderQueue.push(rc);
	//Log(" copy texture command finished, t = " << this_thread::get_id() << " queue size: " << xapp->renderQueue.size() << " frame " << res->frameNum << endl);
	Log(" copy texture command finished, t = " << ThreadInfo::thread_osid() << " queue size: " << xapp->renderQueue.size() << " frame " << res->frameNum << endl);
}

void CopyTextureEffect::init()
{
	xapp = XApp::getInstance();
	assert(xapp->inInitPhase() == true);
	setThreadCount(xapp->getMaxThreadCount());
}

void CopyTextureEffect::setThreadCount(int max)
{
	assert(xapp != nullptr);
	assert(xapp->inInitPhase() == true);
	worker.resize(max);
}

void CopyTextureEffect::draw(string texName)
{
	assert(xapp->inInitPhase() == false);
	// get ref to current command: (here just the frame number, may be more complicated in other effects)
	int index = xapp->getCurrentApp()->draw_slot;//xapp->getCurrentBackBufferIndex();
	long long framenum = xapp->getCurrentApp()->framenum;
	WorkerCopyTextureCommand *c = &worker.at(index);
	c->draw_slot = index;
	c->framenum = framenum;
	c->type = CommandType::WorkerCopyTexture;
	c->textureName = texName;
	c->xapp = xapp;
	c->resourceStateHelper = xapp->appWindow.resourceStateHelper;
	c->frameResource = xapp->appWindow.getFrameResource(index);
	assert(index == c->frameResource->frameNum);
	//c->commandDetails = c;
	xapp->workerQueue.push(c);
}

