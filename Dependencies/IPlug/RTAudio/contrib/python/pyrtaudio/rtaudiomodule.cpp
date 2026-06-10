/************************************************************************/
/*  PyRtAudio: a python wrapper around RtAudio
    Copyright (c) 2011 Antoine Lefebvre

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/************************************************************************/

// This software is in the development stage
// Do not expect compatibility with future versions.
// Comments, suggestions, new features, bug fixes, etc. are welcome

#include <Python.h>

#include "RtAudio.h" 

extern "C" {

    typedef struct 
    {
        PyObject_HEAD
#if PY_MAJOR_VERSION < 3
        void *padding; // python 2.7 seems to set dac to bad value
                       // after print_function, causing a crash, no
                       // idea why, but this fixes it.
#endif
        RtAudio *dac;
        RtAudioFormat _format;
        int _bufferSize;
        unsigned int inputChannels;
        PyObject *callback_func;
    } PyRtAudio;

    static PyObject *RtAudioErrorException;

    static int callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
        double streamTime, RtAudioStreamStatus status, void *data )
    {
        PyRtAudio* self = (PyRtAudio*) data;

        if (status == RTAUDIO_OUTPUT_UNDERFLOW)
            printf("underflow.\n");

        if (self == NULL) return -1;

        float* in = (float *) inputBuffer;
        float* out = (float *) outputBuffer;

        PyObject *py_callback_func = self->callback_func;

        int retval = 0;

        if (py_callback_func) {
            PyGILState_STATE gstate = PyGILState_Ensure();

#if PY_MAJOR_VERSION >= 3
            PyObject* iBuffer = PyMemoryView_FromMemory((char*)in, sizeof(float) * self->inputChannels * nBufferFrames, PyBUF_READ);
            PyObject* oBuffer = PyMemoryView_FromMemory((char*)out, sizeof(float) * nBufferFrames, PyBUF_WRITE);
#else
            PyObject* iBuffer = PyBuffer_FromMemory(in, sizeof(float) * self->inputChannels * nBufferFrames);
            PyObject* oBuffer = PyBuffer_FromReadWriteMemory(out, sizeof(float) * nBufferFrames);
#endif
            PyObject *arglist =  Py_BuildValue("(O,O)", oBuffer, iBuffer);

            if (arglist == NULL) {
                printf("error.\n");
                PyErr_Print();
                PyGILState_Release(gstate);
                return 2;
            }

            // Calling the callback
            PyObject *result = PyEval_CallObject(py_callback_func, arglist);

            if (PyErr_Occurred() != NULL) {
                PyErr_Print();
            }
#if PY_MAJOR_VERSION >= 3
            else if (result == NULL)
              retval = 0;
            else if (PyLong_Check(result)) {
              retval = PyLong_AsLong(result);
            }
#else
            else if (PyInt_Check(result)) {
              retval = PyInt_AsLong(result);
            }
#endif
            
            Py_DECREF(arglist);
            Py_DECREF(oBuffer);
            Py_DECREF(iBuffer);
            Py_XDECREF(result);

            PyGILState_Release(gstate);            
        }

        return retval;
    }



    static void RtAudio_dealloc(PyRtAudio *self)
    {
        printf("RtAudio_dealloc.\n");
        if (self == NULL) return;

        if (self->dac) {
            self->dac->closeStream();
            Py_CLEAR(self->callback_func);
            delete self->dac;
        }

        Py_TYPE(self)->tp_free((PyObject *) self);
    }


    static PyObject* RtAudio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
        printf("RtAudio_new.\n");
        PyRtAudio *self;
        char *api = NULL;

        if(!PyArg_ParseTuple(args, "|s", &api))
            return NULL;

        self = (PyRtAudio *) type->tp_alloc(type, 0);

        if(self == NULL) return NULL;

        self->dac = NULL;
        self->callback_func = NULL;

        try {
            if (api == NULL)
                self->dac = new RtAudio;
            else if(!strcmp(api, "jack"))
                self->dac = new RtAudio(RtAudio::UNIX_JACK);
            else if(!strcmp(api, "alsa"))
                self->dac = new RtAudio(RtAudio::LINUX_ALSA);
            else if(!strcmp(api, "oss"))
                self->dac = new RtAudio(RtAudio::LINUX_ALSA);
            else if(!strcmp(api, "core"))
                self->dac = new RtAudio(RtAudio::MACOSX_CORE);
            else if(!strcmp(api, "asio"))
                self->dac = new RtAudio(RtAudio::WINDOWS_ASIO);
            else if(!strcmp(api, "directsound"))
                self->dac = new RtAudio(RtAudio::WINDOWS_DS);
        }
        catch (RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }

        self->dac->showWarnings(false);

        //Py_XINCREF(self);
        return (PyObject *) self;
    }

    static int RtAudio_init(PyRtAudio *self, PyObject *args, PyObject *kwds)
    {
        printf("RtAudio_init.\n");
        //if (self == NULL) return 0;
        return 0;
    }

    // This functions does not yet support all the features of the RtAudio::openStream method.
    // Please send your patches if you improves this.
    static PyObject* RtAudio_openStream(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL) return NULL;

        if (self->dac == NULL) {
            printf("the dac is null.\n");
            Py_RETURN_NONE;
        }

        PyObject *oParamsObj;
        PyObject *iParamsObj;
        int fs;
        unsigned int bf;
        PyObject *pycallback;

        if (!PyArg_ParseTuple(args, "OOiiO", &oParamsObj, &iParamsObj, &fs, &bf, &pycallback)) 
            return NULL;

        RtAudio::StreamParameters oParams;
        oParams.deviceId = 1;
        oParams.nChannels = 1;
        oParams.firstChannel = 0;

        if (PyDict_Check(oParamsObj)) {
#if PY_MAJOR_VERSION >= 3
            if (PyDict_Contains(oParamsObj, PyUnicode_FromString("deviceId"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyUnicode_FromString("deviceId"));
                oParams.deviceId = PyLong_AsLong(value);
            }
            if (PyDict_Contains(oParamsObj, PyUnicode_FromString("nChannels"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyUnicode_FromString("nChannels"));
                oParams.nChannels = PyLong_AsLong(value);
            }
            if (PyDict_Contains(oParamsObj, PyUnicode_FromString("firstChannel"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyUnicode_FromString("firstChannel"));
                oParams.firstChannel = PyLong_AsLong(value);
            }
#else
            if (PyDict_Contains(oParamsObj, PyString_FromString("deviceId"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyString_FromString("deviceId"));
                oParams.deviceId = PyInt_AsLong(value);
            }
            if (PyDict_Contains(oParamsObj, PyString_FromString("nChannels"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyString_FromString("nChannels"));
                oParams.nChannels = PyInt_AsLong(value);
            }
            if (PyDict_Contains(oParamsObj, PyString_FromString("firstChannel"))) {
                PyObject *value = PyDict_GetItem(oParamsObj, PyString_FromString("firstChannel"));
                oParams.firstChannel = PyInt_AsLong(value);
            }
#endif
        }
        else {
            printf("First argument must be a dictionary. Default values will be used.\n");
        }

        RtAudio::StreamParameters iParams;
        iParams.deviceId = 1;
        iParams.nChannels = 2;
        iParams.firstChannel = 0;

        if (PyDict_Check(iParamsObj)) {
#if PY_MAJOR_VERSION >= 3
            if (PyDict_Contains(iParamsObj, PyUnicode_FromString("deviceId"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyUnicode_FromString("deviceId"));
                iParams.deviceId = PyLong_AsLong(value);
            }
            if (PyDict_Contains(iParamsObj, PyUnicode_FromString("nChannels"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyUnicode_FromString("nChannels"));
                iParams.nChannels = PyLong_AsLong(value);
            }
            if (PyDict_Contains(iParamsObj, PyUnicode_FromString("firstChannel"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyUnicode_FromString("firstChannel"));
                iParams.firstChannel = PyLong_AsLong(value);
            }
#else
            if (PyDict_Contains(iParamsObj, PyString_FromString("deviceId"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyString_FromString("deviceId"));
                iParams.deviceId = PyInt_AsLong(value);
            }
            if (PyDict_Contains(iParamsObj, PyString_FromString("nChannels"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyString_FromString("nChannels"));
                iParams.nChannels = PyInt_AsLong(value);
            }
            if (PyDict_Contains(iParamsObj, PyString_FromString("firstChannel"))) {
                PyObject *value = PyDict_GetItem(iParamsObj, PyString_FromString("firstChannel"));
                iParams.firstChannel = PyInt_AsLong(value);
            }
#endif
        }
        else {
            printf("Second argument must be a dictionary. Default values will be used.\n");
        }


        if (!PyCallable_Check(pycallback)) {
            PyErr_SetString(PyExc_TypeError, "Need a callable object!");
            Py_XINCREF(PyExc_TypeError);
            return NULL;
        }

        // sanity check the callback ?


        Py_INCREF(pycallback);         /* Add a reference to new callback */
        self->callback_func = pycallback; /*Remember new callback */

        // add support for other format
        self->_format = RTAUDIO_FLOAT32;

        // add support for other options
        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_NONINTERLEAVED;

        try {
            if (self->dac->isStreamOpen())
                self->dac->closeStream();
            self->dac->openStream(&oParams, &iParams, self->_format, fs, &bf, &callback, self, &options);
        }
        catch ( RtAudioError& error ) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }

        self->inputChannels = iParams.nChannels;

        Py_RETURN_NONE;
    }

    static PyObject* RtAudio_closeStream(PyRtAudio *self, PyObject *args)
    {
        printf("RtAudio_closeStream.\n");
        if (self == NULL || self->dac == NULL) return NULL;

        try {
            self->dac->closeStream();
            Py_CLEAR(self->callback_func);
        }
        catch(RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }

        Py_RETURN_NONE;
    }

    static PyObject* RtAudio_startStream(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;

        try {
            self->dac->startStream();
        }
        catch(RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }

        Py_RETURN_NONE;
    }


    static PyObject* RtAudio_stopStream(PyRtAudio *self, PyObject *args)
    {
        printf("RtAudio_stopStream.\n");
        if (self == NULL || self->dac == NULL) return NULL;

        try {
            self->dac->stopStream();
        }
        catch(RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }

        Py_RETURN_NONE;
    }

    static PyObject* RtAudio_abortStream(PyRtAudio *self, PyObject *args)
    {
        printf("RtAudio_abortStream.\n");
        if (self == NULL || self->dac == NULL) return NULL;

        try {
            self->dac->abortStream();
        }
        catch(RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }
        Py_RETURN_NONE;
    }

    static PyObject* RtAudio_isStreamRunning(PyRtAudio *self, PyObject *args)
    {
       if (self == NULL || self->dac == NULL) return NULL;

       if (self->dac == NULL) {
            Py_RETURN_FALSE;
        }
        if (self->dac->isStreamRunning())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject* RtAudio_isStreamOpen(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;

        if (self->dac == NULL) {
            Py_RETURN_FALSE;
        }
        if (self->dac->isStreamOpen())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;

    }

    static PyObject* RtAudio_getDeviceCount(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;

#if PY_MAJOR_VERSION >= 3
        return PyLong_FromLong(self->dac->getDeviceCount());
#else
        return PyInt_FromLong(self->dac->getDeviceCount());
#endif
    }

    static PyObject* RtAudio_getDeviceInfo(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;

        int device;
        if (!PyArg_ParseTuple(args, "i", &device))
            return NULL;

        try {
            RtAudio::DeviceInfo info = self->dac->getDeviceInfo(device);

            PyObject* info_dict = PyDict_New();

            if (info.probed) {
                Py_INCREF(Py_True);
                PyDict_SetItemString(info_dict, "probed", Py_True);
            }
            else {
                Py_INCREF(Py_False);
                PyDict_SetItemString(info_dict, "probed", Py_False);
            }
            PyObject* obj;

#if PY_MAJOR_VERSION >= 3
            obj = PyUnicode_FromString(info.name.c_str());
            PyDict_SetItemString(info_dict, "name", obj);

            obj = PyLong_FromLong(info.outputChannels);
            PyDict_SetItemString(info_dict, "outputChannels", obj);

            obj = PyLong_FromLong(info.inputChannels);
            PyDict_SetItemString(info_dict, "inputChannels", obj);

            obj = PyLong_FromLong(info.duplexChannels);
            PyDict_SetItemString(info_dict, "duplexChannels", obj);
#else
            obj = PyString_FromString(info.name.c_str());
            PyDict_SetItemString(info_dict, "name", obj);

            obj = PyInt_FromLong(info.outputChannels);
            PyDict_SetItemString(info_dict, "outputChannels", obj);

            obj = PyInt_FromLong(info.inputChannels);
            PyDict_SetItemString(info_dict, "inputChannels", obj);

            obj = PyInt_FromLong(info.duplexChannels);
            PyDict_SetItemString(info_dict, "duplexChannels", obj);
#endif

            if (info.isDefaultOutput) {
                Py_INCREF(Py_True);
                PyDict_SetItemString(info_dict, "isDefaultOutput", Py_True);
            }
            else {
                Py_INCREF(Py_False);
                PyDict_SetItemString(info_dict, "isDefaultOutput", Py_False);
            }

            if (info.isDefaultInput) {
                Py_INCREF(Py_True);
                PyDict_SetItemString(info_dict, "isDefaultInput", Py_True);
            }
            else {
                Py_INCREF(Py_False);
                PyDict_SetItemString(info_dict, "isDefaultInput", Py_False);
            }

            return info_dict;

        }
        catch(RtAudioError &error) {
            PyErr_SetString(RtAudioErrorException, error.getMessage().c_str());
            Py_INCREF(RtAudioErrorException);
            return NULL;
        }
    }

    static PyObject* RtAudio_getDefaultOutputDevice(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;
#if PY_MAJOR_VERSION >= 3
        return PyLong_FromLong(self->dac->getDefaultOutputDevice());
#else
        return PyInt_FromLong(self->dac->getDefaultOutputDevice());
#endif
    }

    static PyObject* RtAudio_getDefaultInputDevice(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;
#if PY_MAJOR_VERSION >= 3
        return PyLong_FromLong(self->dac->getDefaultInputDevice());
#else
        return PyInt_FromLong(self->dac->getDefaultInputDevice());
#endif
    }

    static PyObject* RtAudio_getStreamTime(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;
        return PyFloat_FromDouble( self->dac->getStreamTime() );
    }

    static PyObject* RtAudio_getStreamLatency(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;
#if PY_MAJOR_VERSION >= 3
        return PyLong_FromLong( self->dac->getStreamLatency() );
#else
        return PyInt_FromLong( self->dac->getStreamLatency() );
#endif
    }

    static PyObject* RtAudio_getStreamSampleRate(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;
#if PY_MAJOR_VERSION >= 3
        return PyLong_FromLong( self->dac->getStreamSampleRate() );
#else
        return PyInt_FromLong( self->dac->getStreamSampleRate() );
#endif
    }

    static PyObject* RtAudio_showWarnings(PyRtAudio *self, PyObject *args)
    {
        if (self == NULL || self->dac == NULL) return NULL;

        PyObject *obj;
        if (!PyArg_ParseTuple(args, "O", &obj))
            return NULL;

        if (!PyBool_Check(obj))
            return NULL;

        if (obj == Py_True)
            self->dac->showWarnings(true);
        else if (obj == Py_False)
            self->dac->showWarnings(false);
        else {
            printf("not true nor false\n");
        }
        Py_RETURN_NONE;
    }


    static PyMethodDef RtAudio_methods[] = 
    {
        // TO BE DONE: getCurrentApi(void)
        {"getDeviceCount", (PyCFunction) RtAudio_getDeviceCount, METH_NOARGS,
        "A public function that queries for the number of audio devices available."},
        {"getDeviceInfo", (PyCFunction) RtAudio_getDeviceInfo, METH_VARARGS,
        "Return a dictionary with information for a specified device number."},
        {"getDefaultOutputDevice", (PyCFunction) RtAudio_getDefaultOutputDevice, METH_NOARGS,
        "A function that returns the index of the default output device."},
        {"getDefaultInputDevice", (PyCFunction) RtAudio_getDefaultInputDevice, METH_NOARGS,
        "A function that returns the index of the default input device."},
        {"openStream", (PyCFunction) RtAudio_openStream, METH_VARARGS,
        "A public method for opening a stream with the specified parameters."},
        {"closeStream", (PyCFunction) RtAudio_closeStream, METH_NOARGS,
        "A function that closes a stream and frees any associated stream memory. "},
        {"startStream", (PyCFunction) RtAudio_startStream, METH_NOARGS,
        "A function that starts a stream. "},
        {"stopStream", (PyCFunction) RtAudio_stopStream, METH_NOARGS,
        "Stop a stream, allowing any samples remaining in the output queue to be played. "},
        {"abortStream", (PyCFunction) RtAudio_abortStream, METH_NOARGS,
        "Stop a stream, discarding any samples remaining in the input/output queue."},
        {"isStreamOpen", (PyCFunction) RtAudio_isStreamOpen, METH_NOARGS,
        "Returns true if a stream is open and false if not."},
        {"isStreamRunning", (PyCFunction) RtAudio_isStreamRunning, METH_NOARGS,
        "Returns true if the stream is running and false if it is stopped or not open."},
        {"getStreamTime", (PyCFunction) RtAudio_getStreamTime, METH_NOARGS,
        "Returns the number of elapsed seconds since the stream was started."},
        {"getStreamLatency", (PyCFunction) RtAudio_getStreamLatency, METH_NOARGS,
        "Returns the internal stream latency in sample frames."},
        {"getStreamSampleRate", (PyCFunction) RtAudio_getStreamSampleRate, METH_NOARGS,
        "Returns actual sample rate in use by the stream."},
        {"showWarnings", (PyCFunction) RtAudio_showWarnings, METH_VARARGS,
        "Specify whether warning messages should be printed to stderr."},
        // TO BE DONE: getCompiledApi (std::vector< RtAudio::Api > &apis) throw ()
        {NULL}
    };

    static PyTypeObject RtAudio_type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "rtaudio.RtAudio",             /*tp_name*/
        sizeof(RtAudio), /*tp_basicsize*/
        0,                         /*tp_itemsize*/
        (destructor) RtAudio_dealloc,                         /*tp_dealloc*/
        0,                         /*tp_print*/
        0,                         /*tp_getattr*/
        0,                         /*tp_setattr*/
        0,                         /*tp_compare*/
        0,                         /*tp_repr*/
        0,                         /*tp_as_number*/
        0,                         /*tp_as_sequence*/
        0,                         /*tp_as_mapping*/
        0,                         /*tp_hash */
        0,                         /*tp_call*/
        0,                         /*tp_str*/
        0,                         /*tp_getattro*/
        0,                         /*tp_setattro*/
        0,                         /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,        /*tp_flags*/
        "Audio input device",           /* tp_doc */
        0,               /* tp_traverse */
        0,               /* tp_clear */
        0,               /* tp_richcompare */
        0,               /* tp_weaklistoffset */
        0,               /* tp_iter */
        0,               /* tp_iternext */
        RtAudio_methods,             /* tp_methods */
        0,              /* tp_members */
        0,                         /* tp_getset */
        0,                         /* tp_base */
        0,                         /* tp_dict */
        0,                         /* tp_descr_get */
        0,                         /* tp_descr_set */
        0,                         /* tp_dictoffset */
        (initproc)RtAudio_init,      /* tp_init */
        0,                         /* tp_alloc */
        RtAudio_new,                 /* tp_new */
        0, /* Low-level free-memory routine */
	    0, /* For PyObject_IS_GC */
	    0, // PyObject *tp_bases;
	    0, // PyObject *tp_mro; /* method resolution order */
	    0, //PyObject *tp_cache;
	    0, //PyObject *tp_subclasses;
	    0, //PyObject *tp_weaklist;
	    0, //destructor tp_del;
        //0,	/* Type attribute cache version tag. Added in version 2.6 */
    };

#if PY_MAJOR_VERSION >= 3
    static PyModuleDef RtAudio_module = {
        PyModuleDef_HEAD_INIT,
        "RtAudio",
        "RtAudio wrapper.",
    };
#endif

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
    PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
    PyInit_rtaudio(void)
#else
    initrtaudio(void)
#endif
    {
        if (!PyEval_ThreadsInitialized())
            PyEval_InitThreads();

        if (PyType_Ready(&RtAudio_type) < 0)
#if PY_MAJOR_VERSION >= 3
            return NULL;
#else
            return;
#endif

#if PY_MAJOR_VERSION >= 3
        PyObject* module = PyModule_Create(&RtAudio_module);
        if (module == NULL)
            return NULL;
#else
        PyObject* module = Py_InitModule3("rtaudio", NULL, "RtAudio wrapper.");
        if (module == NULL)
            return;
#endif

        Py_INCREF(&RtAudio_type);
        PyModule_AddObject(module, "RtAudio", (PyObject *)&RtAudio_type);

        RtAudioErrorException = PyErr_NewException("rtaudio.RtError", NULL, NULL);
        Py_INCREF(RtAudioErrorException);
        PyModule_AddObject(module, "RtError", RtAudioErrorException);
#if PY_MAJOR_VERSION >= 3
        return module;
#else
        return;
#endif
    }
}
