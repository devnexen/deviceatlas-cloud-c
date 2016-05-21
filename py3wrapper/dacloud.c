#include <Python.h>
#if PY_MAJOR_VERSION >= 3
#include "dacloud.h"

typedef struct {
    struct da_cloud_config cfg;
    unsigned int set:1;
} DaCloudObject;

static PyObject *dacloudmod_error;
static PyTypeObject dacloudmod_type;

static PyObject *dacloudmod_load_config(PyObject *, PyObject *);
static PyObject *dacloudmod_detect(PyObject *, PyObject *);

static PyMethodDef dacloudmod_methods[] = {
    { "load_config", dacloudmod_load_config, METH_VARARGS, NULL },
    { "detect", dacloudmod_detect, METH_VARARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef dacloudmod_module = {
    PyModuleDef_HEAD_INIT,
    "dacloudmod",
    0,
    -1,
    NULL
};

PyMODINIT_FUNC
PyInit_dacloudmod(void)
{
    PyObject *m;
    m = PyModule_Create(&dacloudmod_module);
    if (m == NULL)
        return NULL;

    dacloudmod_error = PyErr_NewException("dacloudmod.error", NULL, NULL);
    Py_INCREF(dacloudmod_error);
    PyModule_AddObject(m, "error", dacloudmod_error);
    return m;   
}

static void
dacloudmod_dealloc(PyObject *_self)
{
    DaCloudObject *self = (DaCloudObject *)_self;
    if (self->set == 1)
        da_cloud_fini(&self->cfg);
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

    if (self->set == 0 && da_cloud_init(&self->cfg, configpath) == 0)
        self->set = 1;
}

static PyObject *
dacloudmod_detect(PyObject *_self, PyObject *args)
{
    PyObject *headers, *props, *hkey, *hvalue;
    Py_ssize_t headerssize, i;
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

    da_list_init(&hhead.list);

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
        da_list_foreach(p, &phead.list) {
            switch (p->type) {
            case DA_CLOUD_BOOL: {
                long value = p->value.l;
                PyDict_SetItemString(props, p->name, PyBool_FromLong(value));
                break;
            }
            case DA_CLOUD_LONG: {
                long value = p->value.l;
                PyDict_SetItemString(props, p->name, PyLong_FromLong(value));
                break;
            }
            default: {
                const char *value = p->value.s;
                PyDict_SetItemString(props, p->name, PyUnicode_FromString(value));
                break;
            }    
            }   
        }

        da_cloud_properties_free(&phead);
    }

    da_cloud_header_free(&hhead);

    return props;
}

static PyObject *
dacloudmod_getattro(DaCloudObject *self, PyObject *name)
{
   return PyObject_GenericGetAttr((PyObject *)self, name);
}

static PyTypeObject dacloudmod_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dacloudmod.DaCloud",
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
