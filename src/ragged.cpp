#include <Python.h>

#include <ragged.h>
#include <validate.h>
#include <tools.h>


PyDoc_STRVAR(
    __doc__,
    "\n"
    "ragged(callable, *objects, _star=True, _finalizer=None, **kwargs)\n"
    "\n"
    "Safe `apply` that allows ragged-edge nested objects.\n"
    "See docs for `.apply` for parameters.\n"
    "\n"
    "Details\n"
    "-------\n"
    "This version of apply implicitly recursively broadcasts any object, that\n"
    "is not a built-in container to deeper levels of nested built-in containers.\n"
    "\n"
);


PyObject* _ragged(
    PyObject *callable,
    PyObject *objects,
    PyObject *kwargs,
    const bool star,
    PyObject *finalizer);


PyObject* _ragged_dict(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool star,
    PyObject *finalizer,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    PyObject *output = PyDict_New();
    if(output == NULL)
        return NULL;

    Py_ssize_t pos = 0;
    PyObject *key, *item_, *main_;
    while(PyDict_Next(main, &pos, &key, &main_)) {
        // clone the args and replace containers with their respecitve items
        // We used to reuse the same single tuple object, but it doesn't work
        //  correctly, since the object calling protocol implies that the args
        //  can be simply increfed instead of being copied
        PyObject *args_ = PyTuple_Clone(args);
        if(args_ == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        Py_INCREF(main_);
        PyTuple_SetItem(args_, indices[0], main_);
        for(size_t k = 1; k < indices.size() ; k++) {
            Py_ssize_t j = indices[k];
            item_ = PyDict_GetItem(PyTuple_GET_ITEM(args_, j), key);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        PyObject *result = _ragged(callable, args_, kwargs, star, finalizer);
        Py_DECREF(args_);

        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        PyDict_SetItem(output, key, result);
        Py_DECREF(result);
    }

    return output;
}


int _validate_dict(
    PyObject *args, 
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    Py_ssize_t numel = PyDict_Size(main);
    for(size_t k = 1; k < indices.size() ; k++) {
        Py_ssize_t j = indices[k];

        PyObject *obj = PyTuple_GET_ITEM(args, j), *key, *item;

        if(!Py_IS_TYPE(obj, Py_TYPE(main)))
            return _raise_TypeError(j, main, obj, NULL);

        if(numel != PyDict_Size(obj))
            return _raise_SizeError(j, main, NULL);

        Py_ssize_t pos = 0;
        while (PyDict_Next(main, &pos, &key, &item)) {
            if(!PyDict_Contains(obj, key)) {
                PyErr_SetObject(PyExc_KeyError, key);

                return 0;
            }
        }
    }

    return 1;
}


PyObject* _ragged_list(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool star,
    PyObject *finalizer,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyList_New(numel), *result;
    if(output == NULL)
        return NULL;

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        PyObject *args_ = PyTuple_Clone(args);
        if(args_ == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        for(Py_ssize_t j : indices) {
            PyObject *item_ = PyList_GET_ITEM(PyTuple_GET_ITEM(args_, j), pos);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        result = _ragged(callable, args_, kwargs, star, finalizer);
        Py_DECREF(args_);

        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        // not decrefing `result`, since List steals ref
        PyList_SET_ITEM(output, pos, result);
    }

    return output;
}


int _validate_list(
    PyObject *args, 
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    Py_ssize_t numel = PyList_GET_SIZE(main);
    for(size_t k = 1; k < indices.size() ; k++) {
        Py_ssize_t j = indices[k];

        PyObject *obj = PyTuple_GET_ITEM(args, j);

        if(!Py_IS_TYPE(obj, Py_TYPE(main)))
            return _raise_TypeError(j, main, obj, NULL);

        if(numel != PyList_GET_SIZE(obj))
            return _raise_SizeError(j, main, NULL);
    }

    return 1;
}


PyObject* _ragged_tuple(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool star,
    PyObject *finalizer,
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyTuple_New(numel), *result;
    if(output == NULL)
        return NULL;

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        PyObject *args_ = PyTuple_Clone(args);
        if(args_ == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        for(Py_ssize_t j : indices) {
            PyObject *item_ = PyTuple_GET_ITEM(PyTuple_GET_ITEM(args_, j), pos);

            Py_INCREF(item_);
            PyTuple_SetItem(args_, j, item_);
        }

        result = _ragged(callable, args_, kwargs, star, finalizer);
        Py_DECREF(args_);

        if(result == NULL) {
            Py_DECREF(output);
            return NULL;
        }

        PyTuple_SET_ITEM(output, pos, result);
    }

    if(PyTuple_CheckExact(main))
        return output;

    if(!PyObject_HasAttrString(main, "_fields"))
        return output;

    PyObject *namedtuple = Py_TYPE(main)->tp_new(Py_TYPE(main), output, NULL);
    Py_DECREF(output);

    return namedtuple;
}


int _validate_tuple(
    PyObject *args, 
    const std::vector<Py_ssize_t> &indices)
{
    PyObject *main = PyTuple_GET_ITEM(args, indices[0]);

    Py_ssize_t numel = PyTuple_GET_SIZE(main);
    for(size_t k = 1; k < indices.size() ; k++) {
        Py_ssize_t j = indices[k];

        PyObject *obj = PyTuple_GET_ITEM(args, j);

        if(!Py_IS_TYPE(obj, Py_TYPE(main)))
            return _raise_TypeError(j, main, obj, NULL);

        if(numel != PyTuple_GET_SIZE(obj))
            return _raise_SizeError(j, main, NULL);
    }

    return 1;
}


PyObject* _ragged(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs,
    const bool star,
    PyObject *finalizer)
{
    std::vector<Py_ssize_t> indices = {};
    for(Py_ssize_t j = 0; j < PyTuple_GET_SIZE(args); j++) {
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

    PyObject *result = NULL, *main = PyTuple_GET_ITEM(args, indices[0]);

    if(PyDict_Check(main)) {
        if(!_validate_dict(args, indices))
            return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_dict(callable, args, kwargs, star, finalizer, indices);
        Py_LeaveRecursiveCall();

    }
    else if(PyList_Check(main)) {
        if(!_validate_list(args, indices))
            return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_list(callable, args, kwargs, star, finalizer, indices);
        Py_LeaveRecursiveCall();

    }
    else if(PyTuple_Check(main)) {
        if(!_validate_tuple(args, indices))
            return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _ragged_tuple(callable, args, kwargs, star, finalizer, indices);
        Py_LeaveRecursiveCall();
    }
    else {
        char error[160];
        PyOS_snprintf(
            error, 160, "Unsupported type '%s'", Py_TYPE(main)->tp_name);

        PyErr_SetString(PyExc_TypeError, error);
    }

    if(finalizer == NULL || result == NULL)
        return result;

    PyObject *output = PyObject_CallWithSingleArg(finalizer, result, NULL);
    Py_DECREF(result);

    return output;
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
    int star = 1;

    PyObject *callable = NULL, *objects = NULL, *finalizer=NULL;
    if(!parse_ragged_args(args, &callable, &objects))
        return NULL;

    if (kwargs) {
        static char *kwlist[] = {"_star", "_finalizer", NULL};

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
            empty, own, "|$pO:ragged", kwlist, &star, &finalizer);

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
    PyObject *result = _ragged(callable, objects, kwargs, star, finalizer);

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
