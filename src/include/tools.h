PyObject *PyObject_CallWithSingleArg(
    PyObject *callable,
    PyObject *arg,
    PyObject *kwargs);

PyObject *PyDict_SplitItemStrings(
    PyObject *dict,
    char *keys[],
    const bool pop);

PyObject* PyTuple_Clone(
    PyObject *tuple);