#include "../../build/tinypy.c"
#include <Python.h>
#include "structmember.h"

typedef struct {
    PyObject_HEAD
    tp_vm *vm;
} TinypyObject;

static PyObject *
Tinypy_ConvertObj(tp_obj obj)
{
    int i;
    
    if(obj.type == TP_NUMBER) {
        tp_num v = obj.number.val;
        if ((fabs(v)-fabs((long)v)) < 0.000001) {
            return Py_BuildValue("i", (int) v);
        }
        else {
            return Py_BuildValue("d", v);
        }
    }
    else if(obj.type == TP_STRING) {
        return Py_BuildValue("s#", obj.string.val, obj.string.len);
    }
    else if(obj.type == TP_NONE) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else if(obj.type == TP_LIST) {
        PyObject *plist = PyList_New(0);
        for(i = 0; i < obj.list.val->len; ++i) {
            PyList_Append(plist, Tinypy_ConvertObj(obj.list.val->items[i]));
        }
        return plist;
    }
    else if(obj.type == TP_DICT) {
        PyObject *pdict = PyDict_New();
        for(i = 0; i < obj.dict.val->alloc; ++i) {
            tp_item cur = obj.dict.val->items[i];
            if(cur.used) {
                PyDict_SetItemString(pdict, cur.key.string.val, Tinypy_ConvertObj(cur.val));
            }
        }
        return pdict;
    }
    else {
        return NULL;
    }
}

static int
Tinypy_init(TinypyObject *self, PyObject *args, PyObject *kwds)
{
    self->vm = tp_init(0, NULL);
    double time = 5*1000;       /* 5 seconds default */
    long mem = 16*1024*1024;    /* 16 megabytes default */
    
    if (!PyArg_ParseTuple(args, "|dl", &time, &mem)) {
        return 0;
    }
    tp_sandbox(self->vm, time, mem);

    return 0;
}

static void
Tinypy_destruct(PyObject *self)
{
    tp_deinit(((TinypyObject *) self)->vm);
    PyMem_DEL(self);
}

static PyObject *
Tinypy_exec(TinypyObject *self, PyObject *args)
{    
    tp_obj obj;
    tp_vm *tp = self->vm;
    PyObject *ret, *TinypyError, *TinypyModule, *TinypyDict;
    const char *code;
    
    if (!PyArg_ParseTuple(args, "s|d", &code, &tp->time_limit)) {
        return NULL;
    }
    
    tp->time_elapsed = 0;
    tp->mem_exceeded = 0;
    TinypyModule = PyImport_ImportModule("tinypy");
    TinypyDict = PyModule_GetDict(TinypyModule);
    TinypyError = PyDict_GetItemString(TinypyDict, "error");
    if(setjmp(tp->nextexpr)) {
        --(tp->cur);
        PyErr_SetString(TinypyError, TP_CSTR(tp->ex));      
        return NULL;
    }
    tp->clocks = clock();
    obj = tp_eval(tp, code, tp->builtins);
    ret = Tinypy_ConvertObj(obj);
    return ret;
}

static PyMethodDef Tinypy_methods[] = {
    {"execute", (PyCFunction)Tinypy_exec, METH_VARARGS,
     "Execute code supplied as the argument on the tinypy interpreter"
    },
    {NULL}  /* Sentinel */
};

static PyMethodDef tinypy_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject TinypyType = {
    PyObject_HEAD_INIT(NULL)
    0,                              /*ob_size*/
    "tinypy.Tinypy",                /*tp_name*/
    sizeof(TinypyObject),           /*tp_basicsize*/
    0,                              /*tp_itemsize*/
    Tinypy_destruct,                /*tp_dealloc*/
    0,                              /*tp_print*/
    0,                              /*tp_getattr*/
    0,                              /*tp_setattr*/
    0,                              /*tp_compare*/
    0,                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    0,                              /*tp_hash */
    0,                              /*tp_call*/
    0,                              /*tp_str*/
    0,                              /*tp_getattro*/
    0,                              /*tp_setattro*/
    0,                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,             /*tp_flags*/
    "Tinypy VM instance",           /* tp_doc */
    0,		                        /* tp_traverse */
    0,		                        /* tp_clear */
    0,		                        /* tp_richcompare */
    0,		                        /* tp_weaklistoffset */
    0,		                        /* tp_iter */
    0,		                        /* tp_iternext */
    Tinypy_methods,                 /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)Tinypy_init,          /* tp_init */
};

PyMODINIT_FUNC
inittinypy(void) 
{
    PyObject *m;
    PyObject *TinypyError;

    TinypyType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&TinypyType) < 0)
        return;

    m = Py_InitModule3("tinypy", tinypy_methods,
                       "tinypy - a small Python subset interpreter");
    if (m == NULL) {
      return;
    }
    
    Py_INCREF(&TinypyType);
    PyModule_AddObject(m, "Tinypy", (PyObject *)&TinypyType);
    
    TinypyError = PyErr_NewException("tinypy.error", NULL, NULL);
    Py_INCREF(TinypyError);
    PyModule_AddObject(m, "error", TinypyError);
}
