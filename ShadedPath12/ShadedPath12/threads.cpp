#include "stdafx.h"

void Command::task(XApp * xapp)
{
	//Log("execute command t = " << this_thread::get_id() << " xapp = " << xapp << endl);
	Log("execute command t = " << ThreadInfo::thread_osid() << " xapp = " << xapp << endl);
	try {
		//Sleep(5000);
		WorkerQueue &worker = xapp->workerQueue;
		RenderQueue &render = xapp->renderQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			WorkerCommand *command = worker.pop();
			command->perform();
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}

void Command::renderQueueTask(XApp * xapp)
{
	//Log("execute render queue command t = " << this_thread::get_id() << " xapp = " << xapp << endl);
	Log("execute render queue command t = " << ThreadInfo::thread_osid() << " xapp = " << xapp << endl);
	try {
		RenderQueue &render = xapp->renderQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			RenderCommand command = render.pop();
			ID3D12CommandList* ppCommandLists[] = { command.commandList };
			unsigned int current_frame = xapp->getCurrentBackBufferIndex();
			unsigned int render_command_frame = command.frameNum;
			//assert(current_frame == render_command_frame);
			//Log("render queue t = " << this_thread::get_id() << " frame: " << current_frame << " render command frame: " << render_command_frame << endl);
			Log("render queue t = " << ThreadInfo::thread_osid() << " frame: " << current_frame << " render command frame: " << render_command_frame << endl);
			// only render if swapchain and command list operate on same frame
			if (true || current_frame == render_command_frame) {
				//xapp->dxmanager.waitGPU(*command.frameResource, xapp->appWindow.commandQueue);
				xapp->appWindow.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
				ThrowIfFailedWithDevice(xapp->appWindow.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), xapp->device.Get());
				Log("render queue t = " << ThreadInfo::thread_osid() << " frame: " << current_frame << " presented " << endl);
			}
			//command->perform();
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}