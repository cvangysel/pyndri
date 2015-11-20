#include <Python.h>
#include "structmember.h"

#include <string>
#include <iostream>

#include <indri/CompressedCollection.hpp>
#include <indri/DiskIndex.hpp>
#include <indri/QueryEnvironment.hpp>
#include <indri/Path.hpp>

using std::string;

// IndriRepository

typedef struct {
    PyObject_HEAD

    indri::collection::CompressedCollection* collection_;
    indri::index::DiskIndex* index_;
} IndriRepository;

static void IndriRepository_dealloc(IndriRepository* self) {
    self->ob_type->tp_free((PyObject*) self);

    self->collection_->close();
    self->index_->close();

    delete self->collection_;
    delete self->index_;
}

static PyObject* IndriRepository_new(PyTypeObject* type,
                                     PyObject* args,
                                     PyObject* kwds) {
    IndriRepository* self;

    self = (IndriRepository*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->collection_ = new indri::collection::CompressedCollection();
        self->index_ = new indri::index::DiskIndex();
    }

    return (PyObject*) self;
}

static int IndriRepository_init(IndriRepository* self,
                                PyObject* args,
                                PyObject* kwds) {
    PyObject* repository_path_object = NULL;

    static char* kwlist[] = {"repository_path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S", kwlist,
                                     &repository_path_object)) {
        return -1;
    }

    const char* repository_path = PyString_AsString(repository_path_object);

    const std::string collection_path =
        indri::file::Path::combine(repository_path, "collection");

    try {
        self->collection_->open(collection_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return NULL;
    }

    try {
        self->index_->open(repository_path, "index/0");
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return NULL;
    }

    return 0;
}

static PyMemberDef IndriRepository_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject* IndriRepository_document(IndriRepository* self,
                                          PyObject* args) {
    int int_document_id;

    if (!PyArg_ParseTuple(args, "i", &int_document_id)) {
        return NULL;
    }

    if (int_document_id < self->index_->documentBase() ||
        int_document_id >= self->index_->documentMaximum()) {
        PyErr_SetString(
            PyExc_IndexError,
            "Specified internal document identifier is out of bounds.");

        return NULL;
    }

    const string ext_document_id = self->collection_->retrieveMetadatum(
        int_document_id, "docno");
    const indri::index::TermList* const term_list = self->index_->termList(
        int_document_id);

    PyObject* terms = PyTuple_New(term_list->terms().size());

    indri::utility::greedy_vector<lemur::api::TERMID_T>::const_iterator term_it =
      term_list->terms().begin();

    Py_ssize_t pos = 0;
    for (; term_it != term_list->terms().end(); ++term_it, ++pos) {
      PyTuple_SetItem(terms, pos, PyInt_FromLong(*term_it));
    }

    delete term_list;

    return PyTuple_Pack(2, PyString_FromString(ext_document_id.c_str()), terms);
}

static PyObject* IndriRepository_document_base(IndriRepository* self) {
    return PyInt_FromLong(self->index_->documentBase());
}

static PyObject* IndriRepository_maximum_document(IndriRepository* self) {
    return PyInt_FromLong(self->index_->documentMaximum());
}

static PyMethodDef IndriRepository_methods[] = {
    {"document", (PyCFunction) IndriRepository_document, METH_VARARGS,
     "Return a document (ext_document_id, terms) pair."},
    {"document_base", (PyCFunction) IndriRepository_document_base, METH_NOARGS,
     "Returns the lower bound document identifier (inclusive)."},
    {"maximum_document", (PyCFunction) IndriRepository_maximum_document, METH_NOARGS,
     "Returns the upper bound document identifier (exclusive)."},
    {NULL}  /* Sentinel */
};

static PyTypeObject IndriRepositoryType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyndri.IndriRepository",             /*tp_name*/
    sizeof(IndriRepository),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) IndriRepository_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "IndriRepository objects",           /* tp_doc */
    0,                   /* tp_traverse */
    0,                   /* tp_clear */
    0,                   /* tp_richcompare */
    0,                   /* tp_weaklistoffset */
    0,                   /* tp_iter */
    0,                   /* tp_iternext */
    IndriRepository_methods,             /* tp_methods */
    IndriRepository_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) IndriRepository_init,      /* tp_init */
    0,                         /* tp_alloc */
    IndriRepository_new,                 /* tp_new */
};

// QueryEnvironment

typedef struct {
    PyObject_HEAD

    indri::api::QueryEnvironment* query_env_;
} QueryEnvironment;

static void QueryEnvironment_dealloc(QueryEnvironment* self) {
    self->ob_type->tp_free((PyObject*) self);

    self->query_env_->close();

    delete self->query_env_;
}

static PyObject* QueryEnvironment_new(PyTypeObject* type,
                                     PyObject* args,
                                     PyObject* kwds) {
    QueryEnvironment* self;

    self = (QueryEnvironment*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->query_env_ = new indri::api::QueryEnvironment();
    }

    return (PyObject*) self;
}

static int QueryEnvironment_init(QueryEnvironment* self,
                                 PyObject* args,
                                 PyObject* kwds) {
    PyObject* repository_path_object = NULL;

    static char* kwlist[] = {"repository_path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S", kwlist,
                                     &repository_path_object)) {
        return -1;
    }

    const char* repository_path = PyString_AsString(repository_path_object);

    try {
        self->query_env_->addIndex(repository_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return NULL;
    }

    return 0;
}

static PyMemberDef QueryEnvironment_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject* QueryEnvironment_run_query(QueryEnvironment* self,
                                            PyObject* args) {
    char* query_str;
    long results_requested = 100;

    if (!PyArg_ParseTuple(args, "s|i", &query_str, &results_requested)) {
        return NULL;
    }

    const std::vector<indri::api::ScoredExtentResult> query_results =
        self->query_env_->runQuery(query_str, results_requested);

    PyObject* results = PyTuple_New(query_results.size());

    std::vector<indri::api::ScoredExtentResult>::const_iterator it = query_results.begin();

    Py_ssize_t pos = 0;
    for (; it != query_results.end(); ++it, ++pos) {
      PyTuple_SetItem(
          results,
          pos,
          PyTuple_Pack(2, PyInt_FromLong(it->document), PyFloat_FromDouble(it->score)));
    }

    return results;
}

static PyMethodDef QueryEnvironment_methods[] = {
    {"run_query", (PyCFunction) QueryEnvironment_run_query, METH_VARARGS,
     "Queries an Indri index."},
    {NULL}  /* Sentinel */
};

static PyTypeObject QueryEnvironmentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyndri.QueryEnvironment",             /*tp_name*/
    sizeof(QueryEnvironment),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) QueryEnvironment_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "QueryEnvironment objects",           /* tp_doc */
    0,                   /* tp_traverse */
    0,                   /* tp_clear */
    0,                   /* tp_richcompare */
    0,                   /* tp_weaklistoffset */
    0,                   /* tp_iter */
    0,                   /* tp_iternext */
    QueryEnvironment_methods,             /* tp_methods */
    QueryEnvironment_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) QueryEnvironment_init,      /* tp_init */
    0,                         /* tp_alloc */
    QueryEnvironment_new,                 /* tp_new */
};

// Module methods.

static PyMethodDef PyndriMethods[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyndri(void) {
    if (PyType_Ready(&IndriRepositoryType) < 0) {
        return;
    }

    if (PyType_Ready(&QueryEnvironmentType) < 0) {
        return;
    }

    PyObject* const module = Py_InitModule("pyndri", PyndriMethods);

    Py_INCREF(&IndriRepositoryType);
    PyModule_AddObject(module, "IndriRepository", (PyObject*) &IndriRepositoryType);

    Py_INCREF(&QueryEnvironmentType);
    PyModule_AddObject(module, "QueryEnvironment", (PyObject*) &QueryEnvironmentType);
}
