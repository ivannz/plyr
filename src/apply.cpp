#include <Python.h>

#include <tools.h>

#include <apply.h>
#include <validate.h>
// https://edcjones.tripod.com/refcount.html
// https://pythonextensionpatterns.readthedocs.io/en/latest/refcount.html


PyDoc_STRVAR(
    __doc__,
    "\n"
    "apply(\n"
    "    callable,\n"
    "    *objects,\n"
    "    _safe=True,\n"
    "    _star=True,\n"
    "    _finalizer=None,\n"
    "    _committer=None,\n"
    "    _strict=True,\n"
    "    **kwargs,\n"
    ")\n"
    "\n"
    "Compute the function using the leaf data of the nested objects as arguments.\n"
    "\n"
    "A `nested object` is either a python object (object, str, numpy array, torch\n"
    "tensor, etc.) or one of python's built-in containers (dict, list, or tuple),\n"
    "that consists of other nested objects. The `leaf data` is any non-container\n"
    "python object at the bottom of the nested structure.\n"
    "\n"
    "Parameters\n"
    "----------\n"
    "callable : callable\n"
    "    A callable object to be applied to the leaf data.\n"
    "\n"
    "*objects : nested objects\n"
    "    All remaining positionals to `apply` are assumed to be nested objects,\n"
    "    that supply arguments for the callable from their leaf data.\n"
    "\n"
    "_safe : bool, default=True\n"
    "    Disables structural safety checks when more than one nested object has\n"
    "    been supplied.\n"
    "\n"
    "    Switching safety off SEGFAULTs if the nested objects do not have\n"
    "    IDENTICAL STRUCTURE, or if `minimality' is violated (see the caveat).\n"
    "\n"
    "_star : bool, default=True\n"
    "    Determines how to pass the leaf data to the callable.\n"
    "    If `True` (star-apply), then we call\n"
    "        `callable(d_1, d_2, ..., d_n, **kwargs)`,\n"
    "\n"
    "    otherwise packages the leaf data into a tuple (tuple-apply) and calls\n"
    "        `callable((d_1, d_2, ..., d_n), **kwargs)`\n"
    "\n"
    "    even for `n=1`.\n"
    "\n"
    "_finalizer : callable, optional\n"
    "    The finalizer object to be called when a nested container has been\n"
    "    rebuilt. It is NEVER called on the output of `callable` computed on\n"
    "    the leaf python objects, ONLY on upon finishing the containers.\n"
    "\n"
    "    OMIT the `_finalizer` kwarg if finalization is NOT REQUIRED.\n"
    "\n"
    "_committer : callable, optional\n"
    "    The committer object to be called on the result of `callable`, computed\n"
    "    on the leaf python objects. The returned value of `_committer` is put\n"
    "    into the rebuilt structure, instead of the original result.\n"
    "\n"
    "    OMIT the `_committer` kwarg if no postprocessing is NOT REQUIRED.\n"
    "\n"
    "_strict : bool, default=True\n"
    "    Whether to treat the subtypes of built-in containers as non-leaf nested\n"
    "    containers and descend into them. NOTE, that when being rebuilt, subtypes\n"
    "    REGRESS to their built-in base types.\n"
    "\n"
    "    NOTE `_strict` does not affect treatment of namedtuples (SEE caveat).\n"
    "\n"
    "**kwargs : variable keyword arguments\n"
    "   The optional keyword arguments passed AS IS to the `callable` every\n"
    "   time it is invoked on the leaf data.\n"
    "\n"
    "Returns\n"
    "-------\n"
    "result : a new nested object\n"
    "    The nested object that contains the values returned by `callable`.\n"
    "    Guaranteed to have IDENTICAL structure as the first nested object\n"
    "    in objects.\n"
    "\n"
    "Caveat on `safe=False`\n"
    "----------------------\n"
    "The FIRST object in `*objects` plays a special role: its nested structure\n"
    "determines how all objects are jointly traversed and dictates the structure\n"
    "of the computed result. If safety checks are off, its structure is ALLOWED\n"
    "to be ``minimal'' among the structures of all objects, i.e. lists and tuples\n"
    "of the first object are allowed to be shorter, its dicts' keys may be strict\n"
    "subsets of the corresponding dicts in other objects.\n"
    "\n"
    "    The unsafe procedure SEGFAULTs if this `minimality' is violated,\n"
    "    however safety checks enforce STRICTLY IDENTICAL STRUCTURE.\n"
    "\n"
    "    NOTE: namedtuples are compared as tuples and not as dicts, due to them\n"
    "          being runtime-constructed sub-classes of tuples. Hence for them\n"
    "          only the order matters and not their fields' names.\n"
    "\n"
    "Caveat on namedtuples\n"
    "---------------------\n"
    "`apply` treats namedtuples as nested containers regardless of the `_strict`\n"
    "flag. This was designed intentionally, since NTs are tuples with attributes\n"
    "identifying the items within, and as such can be viewed as immutable dict-like\n"
    "structures. Detection of NTs, however is somewhat duck-typed and non-robust:\n"
    "we check if a container's type ONLY from a `tuple` AND the object itself has\n"
    "`_fields` attribute as suggested in this discussion:\n"
    "\n"
    "    https://mail.python.org/pipermail//python-ideas/2014-January/024886.html\n"
    "\n"
    "Details\n"
    "-------\n"
    "For a single container `apply` with `_star=True` and a specified\n"
    "`_finalizer` callable is roughly equivalent to\n"
    "\n"
    ">>> def apply(fn, container, *, _finalizer, **kwargs):\n"
    ">>>     if isinstance(container, dict):\n"
    ">>>         result = {k: apply(fn, v, **kwargs)\n"
    ">>>                   for k, v in container.items()}\n"
    ">>>         return _finalizer(result)\n"
    ">>>\n"
    ">>>     if isinstance(container, (tuple, list)):\n"
    ">>>         result = [apply(fn, v, **kwargs) for v in container]\n"
    ">>>         return _finalizer(type(container)(result))\n"
    ">>>\n"
    ">>>     return fn(container, **kwargs)\n"
    "\n"
);


PyObject* _apply(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer);


static PyObject* _apply_dict(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer)
{
    Py_ssize_t len = PyTuple_GET_SIZE(rest);
    PyObject *key, *main_, *item_, *rest_ = PyTuple_New(len);
    if(rest_ == NULL)
        return NULL;

    PyObject *output = PyDict_New(), *result = NULL;
    if(output == NULL) {
        Py_DECREF(rest_);
        return NULL;
    }

    Py_ssize_t pos = 0;
    // Any references returned by `PyDict_Next` are borrowed from the dict
    //     https://docs.python.org/3/c-api/dict.html#c.PyDict_Next
    while (PyDict_Next(main, &pos, &key, &main_)) {
        for(Py_ssize_t j = 0; j < len; j++) {
            // `PyDict_GetItem` and `PyTuple_GET_ITEM` return a borrowed reference
            //     https://docs.python.org/3/c-api/dict.html#c.PyDict_GetItem
            item_ = PyDict_GetItem(PyTuple_GET_ITEM(rest, j), key);

            // `PyTuple_SetItem` decrefs any non-NULL item already in the tuple
            //  at the affected position. In contrast, `PyTuple_SET_ITEM` does
            //  NOT discard references to items being replaced!
            //    https://docs.python.org/3/c-api/tuple.html#c.PyTuple_SetItem
            Py_XDECREF(PyTuple_GET_ITEM(rest_, j));

            // a tuple assumes ownership of, or 'steals', the reference, owned
            //  by a dict from `rest`, so we incref it for protection.
            Py_INCREF(item_);
            PyTuple_SET_ITEM(rest_, j, item_);
        }

        // `result` is a new object, for which we are now responsible
        result = _apply(callable, main_, rest_, safe, star, kwargs, finalizer, strict, committer);
        if(result == NULL) {
            Py_DECREF(rest_);

            // decrefing a dict also applies decref to its contents
            Py_DECREF(output);
            return NULL;
        }

        // dict's setitem DOES NOT steal references to `val` and, apparently,
        //  to `key`, i.e. does an incref of its own (both value and the key),
        //  which is why `_apply_dict` logic is different from `_tuple`.
        //     https://docs.python.org/3/c-api/dict.html#c.PyDict_SetItem
        PyDict_SetItem(output, key, result);

        // decref the result, so that only `output` owns a ref
        Py_DECREF(result);
    }

    // decrefing a tuple with only one reference, as is the case here, also
    //  decrefs all its items.
    Py_DECREF(rest_);

    return output;
}


static PyObject* _apply_tuple(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer)
{
    Py_ssize_t len = PyTuple_GET_SIZE(rest);
    PyObject *main_, *item_, *rest_ = PyTuple_New(len);
    if(rest_ == NULL)
        return NULL;

    Py_ssize_t numel = PyTuple_GET_SIZE(main);
    PyObject *output = PyTuple_New(numel), *result = NULL;
    if(output == NULL) {
        Py_DECREF(rest_);
        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        main_ = PyTuple_GET_ITEM(main, pos);
        for(Py_ssize_t j = 0; j < len; j++) {
            // `PyTuple_GET_ITEM` returns a borrowed reference (from the tuple)
            //     https://docs.python.org/3/c-api/tuple.html#c.PyTuple_GET_ITEM
            item_ = PyTuple_GET_ITEM(PyTuple_GET_ITEM(rest, j), pos);

            Py_XDECREF(PyTuple_GET_ITEM(rest_, j));

            Py_INCREF(item_);
            PyTuple_SET_ITEM(rest_, j, item_);
        }

        result = _apply(callable, main_, rest_, safe, star, kwargs, finalizer, strict, committer);
        if(result == NULL) {
            Py_DECREF(rest_);
            Py_DECREF(output);
            return NULL;
        }

        // `PyTuple_SET_ITEM` steals references and does NOT discard refs
        // of displaced objects.
        //     https://docs.python.org/3/c-api/tuple.html#c.PyTuple_SetItem
        PyTuple_SET_ITEM(output, pos, result);
    }

    Py_DECREF(rest_);

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


static PyObject* _apply_list(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer)
{
    Py_ssize_t len = PyTuple_GET_SIZE(rest);
    PyObject *main_, *item_, *rest_ = PyTuple_New(len);
    if(rest_ == NULL)
        return NULL;

    Py_ssize_t numel = PyList_GET_SIZE(main);
    PyObject *output = PyList_New(numel), *result = NULL;
    if(output == NULL) {
        Py_DECREF(rest_);
        return NULL;
    }

    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        main_ = PyList_GET_ITEM(main, pos);
        for(Py_ssize_t j = 0; j < len; j++) {
            // `PyList_GET_ITEM` returns a borrowed reference (from the list)
            //     https://docs.python.org/3/c-api/list.html#c.PyList_GET_ITEM
            item_ = PyList_GET_ITEM(PyTuple_GET_ITEM(rest, j), pos);

            Py_XDECREF(PyTuple_GET_ITEM(rest_, j));

            Py_INCREF(item_);
            PyTuple_SET_ITEM(rest_, j, item_);
        }

        result = _apply(callable, main_, rest_, safe, star, kwargs, finalizer, strict, committer);
        if(result == NULL) {
            Py_DECREF(rest_);
            Py_DECREF(output);
            return NULL;
        }

        // Like `PyList_SetItem`, `PyList_SET_ITEM` steals the reference from
        // us. However, unlike it `_SET_ITEM` DOES NOT discard refs of
        // displaced objects. We're ok, because `output` is a NEW list.
        //     https://docs.python.org/3/c-api/list.html#c.PyList_SET_ITEM
        PyList_SET_ITEM(output, pos, result);
    }

    Py_DECREF(rest_);

    return output;
}


static PyObject* _apply_mapping(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer)
{
    // XXX it's unlikely that we will ever use this branch, because as docs say
    //  it is impossible to know the type of keys of a mapping at runtime, hence
    //  lists, tuples, dicts and any objects with `__getitem__` are mappings
    //  according to `PyMapping_Check`.
    Py_ssize_t len = PyTuple_GET_SIZE(rest);
    PyObject *key, *main_, *item_, *rest_ = PyTuple_New(len);
    if(rest_ == NULL)
        return NULL;

    PyObject *output = PyDict_New(), *result = Py_None;
    if(output == NULL) {
        Py_DECREF(rest_);
        return NULL;
    }

    PyObject *items = PyMapping_Items(main);
    if(items == NULL) {
        Py_DECREF(rest_);
        Py_DECREF(output);
        return NULL;
    }

    Py_INCREF(result);

    Py_ssize_t numel = PyList_GET_SIZE(items);
    for(Py_ssize_t pos = 0; pos < numel; pos++) {
        item_ = PyList_GET_ITEM(items, pos);
        key = PyTuple_GET_ITEM(item_, 0);
        main_ = PyTuple_GET_ITEM(item_, 1);

        for(Py_ssize_t j = 0; j < len; j++) {
            // `PyObject_GetItem` yields a new reference
            //     https://docs.python.org/3/c-api/object.html#c.PyObject_GetItem
            item_ = PyObject_GetItem(PyTuple_GET_ITEM(rest, j), key);

            Py_XDECREF(PyTuple_GET_ITEM(rest_, j));
            PyTuple_SET_ITEM(rest_, j, item_);
        }

        Py_DECREF(result);

        result = _apply(callable, main_, rest_, safe, star, kwargs, finalizer, strict, committer);
        if(result == NULL) break;

        PyDict_SetItem(output, key, result);
    }

    Py_DECREF(items);
    Py_DECREF(rest_);

    if(result == NULL) return NULL;
    Py_DECREF(result);

    return output;
}


static PyObject* _apply_base(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool star,
    PyObject *kwargs,
    PyObject *committer)
{
    PyObject *output;

    Py_ssize_t len = PyTuple_GET_SIZE(rest);
    PyObject *item_, *args = PyTuple_New(1+len);
    if(args == NULL) return NULL;

    // reusing `rest` works in our case since the base call actually reassembles
    //  the full tuple: (main,) + rest
    Py_INCREF(main);
    PyTuple_SET_ITEM(args, 0, main);
    for(Py_ssize_t j = 0; j < len; j++) {
        item_ = PyTuple_GET_ITEM(rest, j);

        Py_INCREF(item_);
        PyTuple_SET_ITEM(args, j + 1, item_);
    }

    if (star) {
        output = PyObject_Call(callable, args, kwargs);
    } else {
        output = PyObject_CallWithSingleArg(callable, args, kwargs);
    }
    Py_DECREF(args);

    // bypass the finalizer if _apply_* failed and bubble up the exception
    if(committer == NULL || output == NULL)
        return output;

    // The committer is only called on the leaf data
    PyObject *result = PyObject_CallWithSingleArg(committer, output, NULL);
    Py_DECREF(output);

    return result;
}


PyObject* _apply(
    PyObject *callable,
    PyObject *main,
    PyObject *rest,
    const bool safe,
    const bool star,
    PyObject *kwargs,
    PyObject *finalizer,
    const bool strict,
    PyObject *committer)
{
    PyObject *result;

    if(PyDict_CheckExact(main) || (!strict && PyDict_Check(main))) {
        if(safe)
            if(!_validate_dict(main, rest))
                return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _apply_dict(callable, main, rest, safe, star, kwargs, finalizer, strict, committer);
        Py_LeaveRecursiveCall();

    } else if(
        PyTupleNamedTuple_CheckExact(main) || (!strict && PyTuple_Check(main))
    ) {
        if(safe)
            if(!_validate_tuple(main, rest))
                return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _apply_tuple(callable, main, rest, safe, star, kwargs, finalizer, strict, committer);
        Py_LeaveRecursiveCall();

    } else if(PyList_CheckExact(main) || (!strict && PyList_Check(main))) {
        if(safe)
            if(!_validate_list(main, rest))
                return NULL;

        if(Py_EnterRecursiveCall("")) return NULL;
        result = _apply_list(callable, main, rest, safe, star, kwargs, finalizer, strict, committer);
        Py_LeaveRecursiveCall();

    } else {
        // The base case, i.e. having reached the leaf objects (non containers)
        // is non recursive
        return _apply_base(callable, main, rest, star, kwargs, committer);
    }

    // bypass the finalizer if _apply_* failed and bubble up the exception
    if(finalizer == NULL || result == NULL)
        return result;

    // The finalizer is only called on the inner/nested containers, and never
    //  on the leaf data
    PyObject *output = PyObject_CallWithSingleArg(finalizer, result, NULL);
    Py_DECREF(result);

    return output;
}


int parse_apply_args(
    PyObject *args,
    PyObject **callable,
    PyObject **main,
    PyObject **rest)
{
    // docs imply that `PyTuple_GetSlice` does not steal references to the elements
    //  in the slice, meaning that the objects in it are increfed. However, when
    //  slicing yields the full original tuple, in this case the returned object
    //  is exactly the original tuple, but with its own refcount incremented.
    PyObject *first = PyTuple_GetSlice(args, 0, 2);
    if (first == NULL)
        return 0;

    int parsed = PyArg_ParseTuple(first, "OO|:apply", callable, main);
    Py_DECREF(first);

    if (!parsed)
        return 0;

    if(!PyCallable_Check(*callable)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable.");
        return 0;
    }

    // always increfs the items in the slice, since it always creates a new tuple
    Py_ssize_t len = PyTuple_GET_SIZE(args);
    *rest = PyTuple_GetSlice(args, 2, len);

    return 1;
}


PyObject* apply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    // from the URL at the top: {API 1.2.1} the call mechanism guarantees
    //  to hold a reference to every argument for the duration of the call.
    int safe = 1, star = 1, strict=1;
    PyObject *callable = NULL, *main = NULL, *rest = NULL;
    PyObject *finalizer=NULL, *committer=NULL;

    // handle `apply(fn, main, *rest, ...)`
    // XXX args remains the owner of the extracted objects, but it is guaranteed
    // to stay alive during the call of `apply`, so no need to incref anything.
    if(!parse_apply_args(args, &callable, &main, &rest))
        return NULL;

    // handle `apply(..., *, _star, _safe, _finalizer, **kwargs)`
    // XXX if the finalizer is not required, the kwarg must be omitted.
    if (kwargs) {
        static const char *kwlist[] = {
            "_safe",
            "_star",
            "_finalizer",
            "_committer",
            "_strict",
            NULL,
        };

        // Pop apply's kwargs from `kwargs` so that it could be passed along to
        //  `_apply_base`.
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

        // PyArg_ParseTupleAndKeywords does not do anything with the ownership
        //  of `PyObject`, https://docs.python.org/3/c-api/arg.html#other-objects
        // Thus we hold on to the `finalizer` in case its only ref was
        //  the `kwargs`, which we tinkered with just above.
        int parsed = PyArg_ParseTupleAndKeywords(
            empty, own, "|$ppOOp:apply", (char**) kwlist,
            &safe, &star, &finalizer, &committer, &strict
        );

        Py_DECREF(empty);
        if (!parsed) {
            Py_DECREF(own);
            Py_DECREF(rest);
            return NULL;
        }

        // while `own` is alive we can be sure the finalizer it alive too
        if(finalizer != NULL && !PyCallable_Check(finalizer)) {
            PyErr_SetString(PyExc_TypeError, "The finalizer must be a callable.");

            Py_DECREF(own);
            Py_DECREF(rest);
            return NULL;
        }

        if(committer != NULL && !PyCallable_Check(committer)) {
            PyErr_SetString(PyExc_TypeError, "The committer must be a callable.");

            Py_DECREF(own);
            Py_DECREF(rest);
            return NULL;
        }

        // incref callables PRIOR to decrefing the temporary subdict `own`
        Py_XINCREF(finalizer);  // incref unless NULL
        Py_XINCREF(committer);
        Py_DECREF(own);
    }

    // make the call, then decref everything we might own
    PyObject *result = _apply(
        callable, main, rest, safe, star, kwargs, finalizer, strict, committer);

    Py_XDECREF(finalizer);
    Py_XDECREF(committer);
    Py_DECREF(rest);

    return result;
}


const PyMethodDef def_apply = {
    "apply",
    (PyCFunction) apply,
    METH_VARARGS | METH_KEYWORDS,
    __doc__,
};
