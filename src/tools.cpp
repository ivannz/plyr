#include <Python.h>


PyObject *PyObject_CallWithSingleArg(
    PyObject *callable,
    PyObject *arg,
    PyObject *kwargs)
{
    // much like `PyObject_CallOneArg`, but with optional kwargs:
    //   create a one-element tuple, then call with it and kwargs
    PyObject *single = PyTuple_New(1);
    if(single == NULL) return NULL;

    // `PyTuple_SET_ITEM` steals reference to `arg`, but we borrowed it from
    //  the caller! Thus we first become its independent owner.
    Py_INCREF(arg);
    PyTuple_SET_ITEM(single, 0, arg);

    // the called object increfs its returned value and transfers the ownership
    //  to the caller, i.e. the act of RETURNING is in itself another reference
    //  see https://docs.python.org/3/extending/extending.html#ownership-rules
    //      https://stackoverflow.com/questions/57661466/
    // (we may check refcounts of `output` and `arg` for `lambda x: x`)
    PyObject *output = PyObject_Call(callable, single, kwargs);

    // No need to relinquish ownership of `arg` (decref), since decrefing
    //  a tuple decrefs all non-NULL items (tuple steals/assumes ownership,
    //  instead of borrowing).
    Py_DECREF(single);

    return output;
}


PyObject *PyDict_SplitItemStrings(
    PyObject *dict,
    const char *keys[],
    const bool pop=false)
{
    // Pop the specified keys in a NULL-terminated str key list `keys` from
    //  `dict` and put them into a new dict
    // XXX warning, this manipulates the `dict` inplace unless `pop=false`!
    PyObject* subdict = PyDict_New();
    if (subdict == NULL)
        return NULL;

    // transfer (unless pop=false) objects' ownership from one dict into another
    for(int p = 0; keys[p] != NULL; p++) {
        PyObject* item = PyDict_GetItemString(dict, keys[p]);
        if (item == NULL)
            continue;

        // PyDict_SetItem uses `Py_INCREF() to become an independent owner`
        //  see https://docs.python.org/3/extending/extending.html#ownership-rules
        PyDict_SetItemString(subdict, keys[p], item);  // increfs `item`

        if(pop)
            PyDict_DelItemString(dict, keys[p]);  // decrefs `item`
    }

    // could be an empty dict
    return subdict;
}


PyObject* PyTuple_Clone(PyObject *tuple)
{
    // we cannot use `PyTuple_GetSlice` to clone a tuple, since in this case
    //  the function returns the original increfed tuple.
    Py_ssize_t len = PyTuple_GET_SIZE(tuple);

    PyObject *clone = PyTuple_New(len);
    if(clone == NULL)
        return NULL;

    for(Py_ssize_t j = 0; j < len; j++) {
        PyObject *item = PyTuple_GET_ITEM(tuple, j);

        Py_INCREF(item);
        PyTuple_SET_ITEM(clone, j, item);
    }

    return clone;
}


int PyNamedTuple_CheckExact(PyObject *p)
{
    // "isinstance(o, tuple) and hasattr(o, '_fields')" is the recommended way
    // to check if an object is a namedtuple, however we also verify that the
    // object inherits directly from `tuple` and nothing else, by checking if
    // its `.mro()` is [`<nt-name>`, tuple, object] (first and last are
    // guaranteed).
    //     https://mail.python.org/pipermail//python-ideas/2014-January/024886.html
    //     https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_mro
    if(!PyTuple_Check(p))
        return 0;

    if(PyTuple_GET_SIZE(Py_TYPE(p)->tp_mro) != 3)
        return 0;

    if(
        (const PyTypeObject*) PyTuple_GET_ITEM(Py_TYPE(p)->tp_mro, 1)
            != &PyTuple_Type
    )
        return 0;

    if(!PyObject_HasAttrString(p, "_fields"))
        return 0;

    return 1;
}


int PyTupleNamedTuple_CheckExact(PyObject *p)
{
    // tuple and namedtuple are __almost__ identical, since the latter
    // /is a syntactic convenience for accessing tuples data through named
    // fields.
    if(PyTuple_CheckExact(p))
        return 1;

    return PyNamedTuple_CheckExact(p);
}
