#include <Python.h>

#include <populate.h>
#include <tools.h>

PyDoc_STRVAR(
    __doc__,
    "Populate the specified struct from an iterator.\n"
);


PyObject* _populate(
    PyObject *iter,
    PyObject *main,
    PyObject *filler,
    const bool strict,
    PyObject *committer);


static PyObject* _populate_dict(
    PyObject *iter,
    PyObject *main,
    PyObject *filler,
    const bool strict,
    PyObject *committer)
{
    PyObject *key, *main_;
    PyObject *output = PyDict_New(), *result = NULL;
    if(output == NULL) {
        return NULL;
    }

    Py_ssize_t pos = 0;
    while (PyDict_Next(main, &pos, &key, &main_)) {
        result = _populate(iter, main_, filler, strict, committer);
        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        PyDict_SetItem(output, key, result);

        Py_DECREF(result);
    }

    return output;
}


static PyObject* _populate_tuple(
    PyObject *iter,
    PyObject *main,
    PyObject *filler,
    const bool strict,
    PyObject *committer)
{
    PyObject *main_;
    Py_ssize_t numel = PyTuple_GET_SIZE(main);
    PyObject *output = PyTuple_New(numel), *result = NULL;
    if(output == NULL) {
        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        main_ = PyTuple_GET_ITEM(main, pos);
        result = _populate(iter, main_, filler, strict, committer);
        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        PyTuple_SET_ITEM(output, pos, result);
    }

    if(PyTuple_CheckExact(main))
        return output;

    // Preserve namedtuple, devolve others to built-in tuples
    if(!PyNamedTuple_CheckExact(main))
        return output;

    // since `namedtuple`-s are immutable and derived from `tuple`, we can
    //  just call `tp_new` on them
    // XXX fix this if the namedtuple's implementation changes
    // https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_new
    PyObject *namedtuple = Py_TYPE(main)->tp_new(Py_TYPE(main), output, NULL);
    Py_DECREF(output);

    return namedtuple;
}


static PyObject* _populate_list(
    PyObject *iter,
    PyObject *main,
    PyObject *filler,
    const bool strict,
    PyObject *committer)
{
    PyObject *main_;
    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyList_New(numel), *result = NULL;
    if(output == NULL) {
        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        main_ = PyList_GET_ITEM(main, pos);
        result = _populate(iter, main_, filler, strict, committer);
        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        PyList_SET_ITEM(output, pos, result);
    }

    return output;
}


static PyObject* _populate_base(
    PyObject *iter,
    PyObject *filler,
    PyObject *committer)
{
    // If there are no remaining values, _next returns NULL with no exception set,
    PyObject *output = PyIter_Next(iter);
    if(output == NULL && !PyErr_Occurred()) {
        if(filler == NULL) {
            // indicate that we've been exhausted with the StopIteration
            PyErr_SetNone(PyExc_StopIteration);

        } else {
            // continue populating with the provided filler value
            output = filler;
            Py_INCREF(output);
        }
    }

    // bypass the finalizer if _populate_* failed and bubble up the exception
    if(committer == NULL || output == NULL)
        return output;

    // The committer is only called on the leaf data
    PyObject *result = PyObject_CallWithSingleArg(committer, output, NULL);
    Py_DECREF(output);

    return result;
}


PyObject* _populate(
    PyObject *iter,
    PyObject *main,
    PyObject *filler,
    const bool strict,
    PyObject *committer)
{
    PyObject *result;

    if(PyDict_CheckExact(main) || (!strict && PyDict_Check(main))) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _populate_dict(iter, main, filler, strict, committer);
        Py_LeaveRecursiveCall();

    } else if(
        PyTupleNamedTuple_CheckExact(main) || (!strict && PyTuple_Check(main))
    ) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _populate_tuple(iter, main, filler, strict, committer);
        Py_LeaveRecursiveCall();

    } else if(PyList_CheckExact(main) || (!strict && PyList_Check(main))) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _populate_list(iter, main, filler, strict, committer);
        Py_LeaveRecursiveCall();

    } else {
        return _populate_base(iter, filler, committer);
    }

    return result;
}


PyObject* populate(PyObject *self, PyObject *args, PyObject *kwargs)
{

    PyObject *iter = NULL, *main = NULL, *committer=NULL, *filler=NULL;
    int strict=1;

    static const char *kwlist[] = {"", "", "default", "_committer", "_strict", NULL};
    if(!PyArg_ParseTupleAndKeywords(
        args, kwargs,
        "OO|$OOp:populate", (char**) kwlist,
        &main, &iter, &filler, &committer, &strict
    ))
        return NULL;

    if(!PyIter_Check(iter)) {
        PyErr_SetString(PyExc_TypeError, "The second argument must be an iterator.");
        return NULL;
    }

    if(committer != NULL && !PyCallable_Check(committer)) {
        PyErr_SetString(PyExc_TypeError, "The committer must be a callable.");
        return NULL;
    }

    return _populate(iter, main, filler, strict, committer);
}


const PyMethodDef def_populate = {
    "populate",
    (PyCFunction) populate,
    METH_VARARGS | METH_KEYWORDS,
    __doc__,
};
