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
    char *keys[],
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
