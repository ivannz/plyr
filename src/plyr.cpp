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


static PyTypeObject AtomicTuple = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "plyr.AtomicTuple",             /* tp_name */
    sizeof(PyTupleObject),          /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    (Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE),     /* tp_flags */
    PyDoc_STR(
        "An atomic tuple, NOT considered by plyr as a nested container."
    ),                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,  // &PyTuple_Type,           /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */
};


static PyTypeObject AtomicList = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "plyr.AtomicList",              /* tp_name */
    sizeof(PyListObject),           /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    (Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE),     /* tp_flags */
    PyDoc_STR(
        "An atomic list, NOT considered by plyr as a nested container."
    ),                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,  // &PyList_Type,            /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */
};


static PyTypeObject AtomicDict = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "plyr.AtomicDict",              /* tp_name */
    sizeof(PyDictObject),           /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    (Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE),     /* tp_flags */
    PyDoc_STR(
        "An atomic dict, NOT considered by plyr as a nested container."
    ),                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,  // &PyDict_Type,            /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */
};


PyMODINIT_FUNC
PyInit_plyr(void)
{
    // prepare the atomic leaf container types (excluding namedtuple)
    // XXX see the note on `.tp_base` at
    //     https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_base
    AtomicTuple.tp_base = &PyTuple_Type;
    AtomicList.tp_base = &PyList_Type;
    AtomicDict.tp_base = &PyDict_Type;
    if (
        PyType_Ready(&AtomicTuple) < 0 ||
        PyType_Ready(&AtomicList) < 0 ||
        PyType_Ready(&AtomicDict) < 0
    )
        return NULL;

    PyObject *mod = PyModule_Create(&moduledef);
    if (mod == NULL)
        return NULL;

    bool init_failed = false;

    // register custom types (NB AddModule steals refs on success)
    Py_INCREF(&AtomicTuple);
    if (
        PyModule_AddObject(mod, "AtomicTuple", (PyObject *) &AtomicTuple) < 0
    ) {
        Py_DECREF(&AtomicTuple);
        init_failed = true;
    }

    Py_INCREF(&AtomicList);
    if (
        PyModule_AddObject(mod, "AtomicList", (PyObject *) &AtomicList) < 0
    ) {
        Py_DECREF(&AtomicList);
        init_failed = true;
    }

    Py_INCREF(&AtomicDict);
    if (
        PyModule_AddObject(mod, "AtomicDict", (PyObject *) &AtomicDict) < 0
    ) {
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
