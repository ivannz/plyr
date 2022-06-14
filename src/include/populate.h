PyObject* _populate(
    PyObject *iter,
    PyObject *main,
    const bool strict,
    PyObject *committer);

PyObject* populate(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs);

extern const PyMethodDef def_populate;
