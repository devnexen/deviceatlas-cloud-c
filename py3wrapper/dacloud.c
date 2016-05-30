#include <Python.h>
#if PY_MAJOR_VERSION >= 3
#include "dacloud.h"

typedef struct {
    PyObject_HEAD;
    struct da_cloud_config cfg;
    PyObject *cache_id;
    unsigned int set:1;
} DaCloudObject;

static PyObject *dacloudmod_error;
static PyTypeObject dacloudmod_type;

static PyObject *dacloudmod_load_config(PyObject *, PyObject *);
static PyObject *dacloudmod_detect(PyObject *, PyObject *);
static PyObject *dacloudmod_cache_id(PyObject *, PyObject *);

static PyMethodDef dacloudmod_methods[] = {
    { "load_config", dacloudmod_load_config, METH_VARARGS, NULL },
    { "detect", dacloudmod_detect, METH_VARARGS, NULL },
    { "cache_id", dacloudmod_cache_id, METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef dacloudmod_module = {
    PyModuleDef_HEAD_INIT,
    "dacloud",
    0,
    -1,
    NULL
};

PyMODINIT_FUNC
PyInit_dacloud(void)
{
    PyObject *m;
    m = PyModule_Create(&dacloudmod_module);
    if (m == NULL)
        return NULL;

    dacloudmod_type.tp_new = PyType_GenericNew;
    Py_INCREF(&dacloudmod_type);
    if (PyType_Ready(&dacloudmod_type) < 0)
        return NULL;

    dacloudmod_error = PyErr_NewException("dacloudmod.error", NULL, NULL);
    Py_INCREF(dacloudmod_error);
    PyModule_AddObject(m, "error", dacloudmod_error);
    PyModule_AddObject(m, "DaCloud", (PyObject *)&dacloudmod_type);
    return m;   
}

static void
dacloudmod_dealloc(PyObject *_self)
{
    DaCloudObject *self = (DaCloudObject *)_self;
    if (self->set == 1) {
        Py_DECREF(self->cache_id);
        da_cloud_fini(&self->cfg);
    }
    Py_DECREF(dacloudmod_error);
    Py_DECREF(&dacloudmod_type);
    PyObject_Del(self);
}

static PyObject *
dacloudmod_load_config(PyObject *_self, PyObject *args)
{
    const char *configpath;
    DaCloudObject *self = (DaCloudObject *)_self;

    if (!PyArg_ParseTuple(args, "s:load_config", &configpath)) {
        PyErr_SetString(dacloudmod_error, "load_config call error");
        return NULL;
    }

    if (self->set == 0 && da_cloud_init(&self->cfg, configpath) == 0) {
        const char *cache_id = da_cloud_cache_id(&self->cfg);
        self->cache_id = PyUnicode_FromString(cache_id);
        self->set = 1;
    }

    return PyBool_FromLong((self->set == 1));
}

static PyObject *
dacloudmod_detect(PyObject *_self, PyObject *args)
{
    PyObject *headers, *props, *hkey, *hvalue;
    Py_ssize_t headerssize, i = 0;
    DaCloudObject *self = (DaCloudObject *)_self;

    struct da_cloud_header_head hhead;
    struct da_cloud_property_head phead;
    struct da_cloud_property *p;

    if (self->set == 0) {
        PyErr_SetString(dacloudmod_error, "detect call, configuration not set");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:detect", &headers)) {
        PyErr_SetString(dacloudmod_error, "detect call error");
        return NULL;
    }

    props = PyDict_New();

    if ((headerssize = PyDict_Size(headers)) == 0)
        return props;

    da_cloud_header_init(&hhead);

    while (PyDict_Next(headers, &i, &hkey, &hvalue) == 1) {
        char *hn = NULL, *hv = NULL;
        if (PyBytes_CheckExact(hkey))
            hn = PyBytes_AsString(hkey);
        else
            hn = _PyUnicode_AsString(hkey);

        if (PyBytes_CheckExact(hvalue))
            hv = PyBytes_AsString(hvalue);
        else
            hv = _PyUnicode_AsString(hvalue);

        if (hn == NULL || hv == NULL)
            continue;

        da_cloud_header_add(&hhead, hn, hv);
    }

    if (da_cloud_detect(&self->cfg, &hhead, &phead) == 0) {
        PyObject *cs, *ck;
        da_list_foreach(p, &phead.list) {
            PyObject *v;
            switch (p->type) {
            case DA_CLOUD_BOOL: {
                long value = p->value.l;
                v = PyBool_FromLong(value);
                PyDict_SetItemString(props, p->name, v);
                break;
            }
            case DA_CLOUD_LONG: {
                long value = p->value.l;
                v = PyLong_FromLong(value);
                PyDict_SetItemString(props, p->name, v);
                break;
            }
            default: {
                const char *value = p->value.s;
                v = PyUnicode_FromString(value);
                PyDict_SetItemString(props, p->name, v);
                break;
            }    
            }   

            PyDict_SetItemString(props, p->name, v);
            Py_DECREF(v);
        }

        cs = PyUnicode_FromString(phead.cachesource);
        ck = PyUnicode_FromString(hhead.cachekey);

        PyDict_SetItemString(props, "__cachesource", cs);
        PyDict_SetItemString(props, "__cachekey", ck);

        Py_DECREF(cs);
        Py_DECREF(ck);

        da_cloud_properties_free(&phead);
    }

    da_cloud_header_free(&hhead);

    return props;
}

static PyObject *
dacloudmod_cache_id(PyObject *_self, PyObject *args)
{
    DaCloudObject *self = (DaCloudObject *)_self;
    if (self->set == 0) {
        PyErr_SetString(dacloudmod_error, "cache_id call, configuration not set");
        return NULL;
    } else {
    }
    return self->cache_id;
}

static PyObject *
dacloudmod_getattro(DaCloudObject *self, PyObject *name)
{
   return PyObject_GenericGetAttr((PyObject *)self, name);
}

static PyTypeObject dacloudmod_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dacloud.DaCloud",
    sizeof(DaCloudObject),
    0,
    (destructor)dacloudmod_dealloc,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (getattrofunc)dacloudmod_getattro,
    0,
    0,
    Py_TPFLAGS_DEFAULT,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    dacloudmod_methods
};

#endif
