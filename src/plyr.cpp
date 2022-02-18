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


static PyTypeObject LeafTuple = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.Tuple",
    .tp_doc = PyDoc_STR("Non-container tuple."),
    .tp_basicsize = sizeof(PyTupleObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = PyTuple_Type.tp_init,
    .tp_methods = empty_methods,
};


static PyTypeObject LeafList = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.List",
    .tp_doc = PyDoc_STR("Non-container List."),
    .tp_basicsize = sizeof(PyListObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = PyList_Type.tp_init,
    .tp_methods = empty_methods,
};


static PyTypeObject LeafDict = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "plyr.Dict",
    .tp_doc = PyDoc_STR("Non-container Dict."),
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
    LeafTuple.tp_base = &PyTuple_Type;
    if (PyType_Ready(&LeafTuple) < 0)
        return NULL;

    LeafList.tp_base = &PyList_Type;
    if (PyType_Ready(&LeafList) < 0)
        return NULL;

    LeafDict.tp_base = &PyDict_Type;
    if (PyType_Ready(&LeafDict) < 0)
        return NULL;

    PyObject *mod = PyModule_Create(&moduledef);
    if (mod == NULL)
        return NULL;

    /// register custom types (NB AddModule steals refs on success)
    Py_INCREF(&LeafTuple);
    Py_INCREF(&LeafList);
    Py_INCREF(&LeafDict);
    if (PyModule_AddObject(mod, "Tuple", (PyObject *) &LeafTuple) < 0) {
        Py_DECREF(&LeafTuple);
        init_failed = true;
    }

    if (PyModule_AddObject(mod, "List", (PyObject *) &LeafList) < 0) {
        Py_DECREF(&LeafList);
        init_failed = true;
    }

    if (PyModule_AddObject(mod, "Dict", (PyObject *) &LeafDict) < 0) {
        Py_DECREF(&LeafDict);
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
