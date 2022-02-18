#include <Python.h>

#include <apply.h>
#include <validate.h>
#include <operations.h>

#include <ragged.h>


PyDoc_STRVAR(
    __doc__,
    "\n"
    "Streamlined operations on built-in nested containers of objects.\n"
    "\n"
    "see `plyr.apply`.\n"
);

// apply functions with preset _safe and _star kwargs
// [ts][u_]apply -- t/s tuple or star args, u/_ unsafe or safe
static PyObject* suply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 0, 1, kwargs, NULL, 1);
    Py_DECREF(rest);

    return result;
}


static PyObject* tuply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 0, 0, kwargs, NULL, 1);
    Py_DECREF(rest);

    return result;
}


static PyObject* s_ply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 1, 1, kwargs, NULL, 1);
    Py_DECREF(rest);

    return result;
}


static PyObject* t_ply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 1, 0, kwargs, NULL, 1);
    Py_DECREF(rest);

    return result;
}


static PyMethodDef modplyr_methods[] = {
    def_apply,
    def_validate,
    {
        "suply",
        (PyCFunction) suply,
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(
            "Strict star-apply without safety checks (use at your own risk)."
        ),
    }, {
        "tuply",
        (PyCFunction) tuply,
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(
            "Strict tuple-apply without safety checks (use at your own risk)."
        ),
    }, {
        "s_ply",
        (PyCFunction) s_ply,
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(
            "Strict star-apply with safety checks."
        ),
    }, {
        "t_ply",
        (PyCFunction) t_ply,
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(
            "Strict tuple-apply with safety checks."
        ),
    },
    def_getitem,
    def_setitem,

    def_xgetitem,
    def_xsetitem,

    // XXX debug functions
    def_is_sequence,
    def_is_mapping,
    def_dict_getrefs,
    def_dict_clone,

    def_ragged,
    {
        NULL,
        NULL,
        0,
        NULL,
    }
};


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "plyr",
        __doc__,
        -1,
        modplyr_methods,
};


static PyMethodDef empty_methods[] = {
    {NULL},
};


static PyTypeObject AtomicTuple = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.AtomicTuple",
    .tp_doc = PyDoc_STR(
        "An atomic tuple, NOT considered by plyr as a nested container."
    ),
    .tp_basicsize = sizeof(PyTupleObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = PyTuple_Type.tp_init,
    .tp_methods = empty_methods,
};


static PyTypeObject AtomicList = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.AtomicList",
    .tp_doc = PyDoc_STR(
        "An atomic list, NOT considered by plyr as a nested container."
    ),
    .tp_basicsize = sizeof(PyListObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = PyList_Type.tp_init,
    .tp_methods = empty_methods,
};


static PyTypeObject AtomicDict = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.AtomicDict",
    .tp_doc = PyDoc_STR(
        "An atomic dict, NOT considered by plyr as a nested container."
    ),
    .tp_basicsize = sizeof(PyDictObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = PyDict_Type.tp_init,
    .tp_methods = empty_methods,
};


PyMODINIT_FUNC
PyInit_plyr(void)
{
    bool init_failed = false;

    // prepare the leaf container types (excluding namedtuple)
    AtomicTuple.tp_base = &PyTuple_Type;
    if (PyType_Ready(&AtomicTuple) < 0)
        return NULL;

    AtomicList.tp_base = &PyList_Type;
    if (PyType_Ready(&AtomicList) < 0)
        return NULL;

    AtomicDict.tp_base = &PyDict_Type;
    if (PyType_Ready(&AtomicDict) < 0)
        return NULL;

    PyObject *mod = PyModule_Create(&moduledef);
    if (mod == NULL)
        return NULL;

    /// register custom types (NB AddModule steals refs on success)
    Py_INCREF(&AtomicTuple);
    Py_INCREF(&AtomicList);
    Py_INCREF(&AtomicDict);
    if (PyModule_AddObject(mod, "AtomicTuple", (PyObject *) &AtomicTuple) < 0) {
        Py_DECREF(&AtomicTuple);
        init_failed = true;
    }

    if (PyModule_AddObject(mod, "AtomicList", (PyObject *) &AtomicList) < 0) {
        Py_DECREF(&AtomicList);
        init_failed = true;
    }

    if (PyModule_AddObject(mod, "AtomicDict", (PyObject *) &AtomicDict) < 0) {
        Py_DECREF(&AtomicDict);
        init_failed = true;
    }

    // do not need to decref created types since either thery have been stolen
    // by AddObject on success, or have already been decrefed on failure
    if(init_failed) {
        Py_DECREF(mod);
        return NULL;
    }

    return mod;
}
