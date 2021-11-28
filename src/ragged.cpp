#include <Python.h>

#include <ragged.h>

#include <tools.h>


PyDoc_STRVAR(
    __doc__,
    "\n"
    "`apply`, but with ragged nested objects\n"
);


PyObject* _ragged(
    PyObject *callable,
    PyObject *objects,
    PyObject *kwargs,
    const bool safe,
    const bool star);


PyObject* _ragged_dict(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool safe,
    const bool star,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    // we reuse the same tuple for each item in the dict
    PyObject *args_ = PyTuple_Clone(args);
    if(args_ == NULL)
        return NULL;

    PyObject *output = PyDict_New();
    if(output == NULL) {
        Py_DECREF(args_);

        return NULL;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *item_, *main_;
    while(PyDict_Next(main, &pos, &key, &main_)) {
        // `SetItem` makes sure to decref the existing object, before replacing
        //  it with a new one, see `_apply_dict`
        Py_INCREF(main_);
        PyTuple_SetItem(args_, indices[0], main_);
        for(size_t k = 1; k < indices.size() ; k++) {
            Py_ssize_t j = indices[k];
            item_ = PyDict_GetItem(PyTuple_GET_ITEM(args, j), key);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        PyObject *result = _ragged(callable, args_, kwargs, safe, star);
        if(result == NULL) {
            Py_DECREF(args_);

            Py_DECREF(output);
            return NULL;
        }

        PyDict_SetItem(output, key, result);

        Py_DECREF(result);
    }

    Py_DECREF(args_);

    return output;
}


PyObject* _ragged_list(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool safe,
    const bool star,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    PyObject *args_ = PyTuple_Clone(args);
    if(args_ == NULL)
        return NULL;

    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyList_New(numel), *result;
    if(output == NULL) {
        Py_DECREF(args_);

        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        for(Py_ssize_t j : indices) {
            PyObject *item_ = PyList_GET_ITEM(PyTuple_GET_ITEM(args, j), pos);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        result = _ragged(callable, args_, kwargs, safe, star);
        if(result == NULL) {
            Py_DECREF(args_);

            Py_DECREF(output);
            return NULL;
        }

        PyList_SET_ITEM(output, pos, result);
    }

    Py_DECREF(args_);

    return output;
}


PyObject* _ragged_tuple(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool safe,
    const bool star,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    PyObject *args_ = PyTuple_Clone(args);
    if(args_ == NULL)
        return NULL;

    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyTuple_New(numel), *result;
    if(output == NULL) {
        Py_DECREF(args_);

        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        for(Py_ssize_t j : indices) {
            PyObject *item_ = PyTuple_GET_ITEM(PyTuple_GET_ITEM(args, j), pos);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        result = _ragged(callable, args_, kwargs, safe, star);
        if(result == NULL) {
            Py_DECREF(args_);

            Py_DECREF(output);
            return NULL;
        }

        PyTuple_SET_ITEM(output, pos, result);
    }

    Py_DECREF(args_);

    if(PyTuple_CheckExact(main))
        return output;

    if(!PyObject_HasAttrString(main, "_fields"))
        return output;

    PyObject *namedtuple = Py_TYPE(main)->tp_new(Py_TYPE(main), output, NULL);
    Py_DECREF(output);

    return namedtuple;
}


PyObject* _ragged(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool safe,
    const bool star)
{
    assert(!safe);

    std::vector<Py_ssize_t> indices = {};

    Py_ssize_t len = PyTuple_GET_SIZE(args);
    for(Py_ssize_t j = 0; j < len; j++) {
        PyObject *item_ = PyTuple_GET_ITEM(args, j);
        if(PyDict_Check(item_) || PyTuple_Check(item_) || PyList_Check(item_)) {
            indices.push_back(j);
        }
    }

    if(indices.empty()) {
        if (star) {
            return PyObject_Call(callable, args, kwargs);
        } else {
            return PyObject_CallWithSingleArg(callable, args, kwargs);
        }
    }

    PyObject *result, *main = PyTuple_GET_ITEM(args, indices[0]);

    if(PyDict_Check(main)) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_dict(callable, args, kwargs, safe, star, indices);
        Py_LeaveRecursiveCall();

        return result;

    } else if(PyList_Check(main)) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_list(callable, args, kwargs, safe, star, indices);
        Py_LeaveRecursiveCall();

        return result;

    } else if(PyTuple_Check(main)) {
        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_tuple(callable, args, kwargs, safe, star, indices);
        Py_LeaveRecursiveCall();

        return result;
    }

    char error[160];
    PyOS_snprintf(error, 160, "Unsupported type '%s'", Py_TYPE(main)->tp_name);

    PyErr_SetString(PyExc_TypeError, error);

    return NULL;
}


int parse_ragged_args(
    PyObject *args,
    PyObject **callable,
    PyObject **objects)
{
    PyObject *first = PyTuple_GetSlice(args, 0, 1);
    if (first == NULL)
        return 0;

    int parsed = PyArg_ParseTuple(first, "O|:ragged", callable);
    Py_DECREF(first);

    if (!parsed)
        return 0;

    if(!PyCallable_Check(*callable)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable.");
        return 0;
    }

    Py_ssize_t len = PyTuple_GET_SIZE(args);
    *objects = PyTuple_GetSlice(args, 1, len);

    if(PyTuple_GET_SIZE(*objects) < 1) {
        PyErr_SetString(PyExc_TypeError, "At least one nested object must be provided.");
        return 0;
    }

    return 1;
}


PyObject* ragged(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs)
{
    int safe = 0, star = 1;

    PyObject *callable = NULL, *objects = NULL, *finalizer=NULL;
    if(!parse_ragged_args(args, &callable, &objects))
        return NULL;

    if (kwargs) {
        static char *kwlist[] = {"_safe", "_star", "_finalizer", NULL};

        PyObject* own = PyDict_SplitItemStrings(kwargs, kwlist, true);
        if (own == NULL) {
            Py_DECREF(objects);

            return NULL;
        }

        PyObject *empty = PyTuple_New(0);
        if (empty == NULL) {
            Py_DECREF(objects);
            Py_DECREF(own);

            return NULL;
        }

        int parsed = PyArg_ParseTupleAndKeywords(
            empty, own, "|$ppO:ragged", kwlist, &safe, &star, &finalizer);

        Py_DECREF(empty);
        if (!parsed) {
            Py_DECREF(objects);
            Py_DECREF(own);

            return NULL;
        }

        // while `own` is alive we can be sure the finalizer it alive too
        if(finalizer != NULL && !PyCallable_Check(finalizer)) {
            Py_DECREF(objects);
            Py_DECREF(own);

            PyErr_SetString(PyExc_TypeError, "The finalizer must be a callable.");
            return NULL;
        }

        // incref `finalizer` PRIOR to decrefing the temporary subdict `own`
        Py_XINCREF(finalizer);  // incref unless NULL
        Py_DECREF(own);
    }

    // make the call, then decref everything we might own
    PyObject *result = _ragged(
        callable, objects, kwargs, safe, star);

    Py_XDECREF(finalizer);
    Py_DECREF(objects);

    return result;
}

const PyMethodDef def_ragged = {
    "ragged",
    (PyCFunction) ragged,
    METH_VARARGS | METH_KEYWORDS,
    __doc__,
};
