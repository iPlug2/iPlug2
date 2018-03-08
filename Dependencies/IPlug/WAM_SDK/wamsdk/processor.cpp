// Web Audio Modules (WAMs)
// WAM::Processor
//
// jari kleimola and oli larkin 2015-2017
// jari@webaudiomodules.org
//
#include "processor.h"
using namespace WAM;

EMSCRIPTEN_KEEPALIVE const char* Processor::init(uint32_t bufsize, uint32_t sr, void* desc)
{
	m_bufsize = bufsize;
	m_sr = sr;
	return 0;	// custom descriptor not defined at DSP side
}

// JavaScript glue
extern "C"
{
	EMSCRIPTEN_KEEPALIVE const char* wam_init(Processor* proc, uint32_t bufsize, uint32_t sr, void* desc) { return proc->init(bufsize,sr,desc); }
	EMSCRIPTEN_KEEPALIVE void wam_terminate(Processor* proc) { proc->terminate(); }
	EMSCRIPTEN_KEEPALIVE void wam_resize(Processor* proc, uint32_t bufsize) { proc->resize(bufsize); }
	EMSCRIPTEN_KEEPALIVE void wam_onparam(Processor* proc, uint32_t idparam, float value) { proc->onParam(idparam, value); }
	EMSCRIPTEN_KEEPALIVE void wam_onmidi(Processor* proc, byte status, byte data1, byte data2) { proc->onMidi(status, data1, data2); }
	EMSCRIPTEN_KEEPALIVE void wam_onsysex(Processor* proc, byte* msg, uint32_t size) { proc->onSysex(msg, size); }
	EMSCRIPTEN_KEEPALIVE void wam_onprocess(Processor* proc, AudioBus* audio, void* data) { proc->onProcess(audio, data); }
	EMSCRIPTEN_KEEPALIVE void wam_onpatch(Processor* proc, void* data, uint32_t size) { proc->onPatch(data, size); }
	EMSCRIPTEN_KEEPALIVE void wam_onmessageN(Processor* proc, char* verb, char* res, double data) { proc->onMessage(verb, res, data); }
	EMSCRIPTEN_KEEPALIVE void wam_onmessageS(Processor* proc, char* verb, char* res, char* data) { proc->onMessage(verb, res, data); }
	EMSCRIPTEN_KEEPALIVE void wam_onmessageA(Processor* proc, char* verb, char* res, void* data, uint32_t size) { proc->onMessage(verb, res, data, size); }
}

// for debugging
extern "C"
{
	void wam_logs(const char* s) { EM_ASM_INT(Module.print(Pointer_stringify($0)), s); }
	void wam_logi(int i) { EM_ASM_INT(Module.print($0), i); }
}
