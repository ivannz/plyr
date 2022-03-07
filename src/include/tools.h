PyObject *PyObject_CallWithSingleArg(
    PyObject *callable,
    PyObject *arg,
    PyObject *kwargs);

PyObject *PyDict_SplitItemStrings(
    PyObject *dict,
    const char *keys[],
    const bool pop);

PyObject* PyTuple_Clone(
    PyObject *tuple);

int PyNamedTuple_CheckExact(
    PyObject *p);

int PyTupleNamedTuple_CheckExact(
    PyObject *p);