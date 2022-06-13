#include <Python.h>

#include <apply.h>
#include <validate.h>
#include <operations.h>

#include <ragged.h>
#include <tools.h>


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

    PyObject *result = _apply(callable, main, rest, 0, 1, kwargs, NULL, 1, NULL);
    Py_DECREF(rest);

    return result;
}


static PyObject* tuply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 0, 0, kwargs, NULL, 1, NULL);
    Py_DECREF(rest);

    return result;
}


static PyObject* s_ply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 1, 1, kwargs, NULL, 1, NULL);
    Py_DECREF(rest);

    return result;
}


static PyObject* t_ply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    PyObject *result = _apply(callable, main, rest, 1, 0, kwargs, NULL, 1, NULL);
    Py_DECREF(rest);

    return result;
}


static PyObject* flatapply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int star = 1;
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    if (kwargs) {
        static const char *kwlist[] = {
            "_star",
            NULL,
        };

        PyObject* own = PyDict_SplitItemStrings(kwargs, kwlist, true);
        if (own == NULL) {
            Py_DECREF(rest);
            return NULL;
        }

        PyObject *empty = PyTuple_New(0);
        if (empty == NULL) {
            Py_DECREF(own);
            Py_DECREF(rest);
            return NULL;
        }

        int parsed = PyArg_ParseTupleAndKeywords(
            empty, own, "|$p:apply", (char**) kwlist, &star);

        Py_DECREF(empty);
        if (!parsed) {
            Py_DECREF(own);
            Py_DECREF(rest);
            return NULL;
        }

        Py_DECREF(own);
    }

    // get the `.append` method of a new list to which the leaves are added
    PyObject *list = PyList_New(0);
    if(list == NULL) {
        Py_DECREF(rest);
        return NULL;
    }

    PyObject *append = PyObject_GetAttrString(list, "append");
    if(append == NULL) {
        Py_DECREF(list);
        Py_DECREF(rest);
        return NULL;
    }

    // force safe and strict flags
    PyObject *result = _apply(callable, main, rest, 1, star, kwargs, NULL, 1, append);
    Py_DECREF(append);
    Py_DECREF(rest);

    // value builder creates new references
    PyObject *tuple = Py_BuildValue("(OO)", list, result);
    Py_XDECREF(result);
    Py_DECREF(list);

    return tuple;
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
    }, {
        "flatapply",
        (PyCFunction) flatapply,
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(
            "flatapply(callable, *objects, _star=True, **kwargs)\n"
            "\n"
            "Compute the function on the nested objects' leaves and return\n"
            "a depth-first flattened list of results and the nested structure.\n"
            "\n"
            "Parameters\n"
            "----------\n"
            "callable : callable\n"
            "    A callable to be applied to the leaf data.\n"
            "\n"
            "*objects : nested objects\n"
            "    Like `.apply`, all remaining positionals to `.flatapply` are\n"
            "    assumed to be nested objects, leaves of which supply arguments\n"
            "    for the callable.\n"
            "\n"
            "_star : bool, default=True\n"
            "    Determines whether to pass the leaf data to the callable as\n"
            "    positionals or as a tuple. See `.apply`.\n"
            "\n"
            "**kwargs : variable keyword arguments\n"
            "   Optional keyword arguments passed AS IS to the `callable`.\n"
            "\n"
            "Returns\n"
            "-------\n"
            "flat : list\n"
            "    The list with the results in depth-first order.\n"
            "\n"
            "struct : nested object\n"
            "    The skeletal structure of the nested object.\n"
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
        "base",
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
PyInit_base(void)
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
