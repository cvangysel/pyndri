#include <Python.h>
#include "structmember.h"

#include <cassert>
#include <string>
#include <iostream>

#include <indri/CompressedCollection.hpp>
#include <indri/DiskIndex.hpp>
#include <indri/KrovetzStemmer.hpp>
#include <indri/QueryEnvironment.hpp>
#include <indri/Path.hpp>

using std::string;

#define CHECK_EQ(first, second) assert(first == second)
#define CHECK_GT(first, second) assert(first > second)

// Index

typedef struct {
    PyObject_HEAD

    indri::api::Parameters* parameters_;

    indri::collection::CompressedCollection* collection_;
    indri::index::DiskIndex* index_;

    indri::api::QueryEnvironment* query_env_;
} Index;

static void Index_dealloc(Index* self) {
    self->ob_type->tp_free((PyObject*) self);

    self->collection_->close();
    self->index_->close();
    self->query_env_->close();

    delete self->parameters_;

    delete self->collection_;
    delete self->index_;
    delete self->query_env_;
}

static PyObject* Index_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    Index* self;

    self = (Index*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->parameters_ = new indri::api::Parameters();

        self->collection_ = new indri::collection::CompressedCollection();
        self->index_ = new indri::index::DiskIndex();

        self->query_env_ = new indri::api::QueryEnvironment();
    }

    return (PyObject*) self;
}

static int Index_init(Index* self, PyObject* args, PyObject* kwds) {
    PyObject* repository_path_object = NULL;

    static char* kwlist[] = {"repository_path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S", kwlist,
                                     &repository_path_object)) {
        return -1;
    }

    const char* repository_path = PyString_AsString(repository_path_object);

    // Load parameters.
    self->parameters_->loadFile(indri::file::Path::combine(repository_path, "manifest"));

    const std::string collection_path =
        indri::file::Path::combine(repository_path, "collection");

    try {
        self->collection_->open(collection_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

    // Load index.
    std::string index_path = "index";

    indri::api::Parameters container = (*self->parameters_)["indexes"];

    if(container.exists("index")) {
        indri::api::Parameters indexes = container["index"];

        if (indexes.size() != 1) {
            PyErr_SetString(PyExc_IOError, "Indri repository contain more than one index.");

            return -1;
        }

        index_path = indri::file::Path::combine(
            index_path, (std::string) indexes[static_cast<size_t>(0)]);
    } else {
        PyErr_SetString(PyExc_IOError, "Indri repository does not contain an index.");

        return -1;
    }

    try {
        self->index_->open(repository_path, index_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

    try {
        self->query_env_->addIndex(repository_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

    return 0;
}

static PyMemberDef Index_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject* Index_document(Index* self, PyObject* args) {
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

static PyObject* Index_document_base(Index* self) {
    return PyInt_FromLong(self->index_->documentBase());
}

static PyObject* Index_maximum_document(Index* self) {
    return PyInt_FromLong(self->index_->documentMaximum());
}

static PyObject* Index_run_query(Index* self, PyObject* args) {
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

static PyObject* Index_get_dictionary(Index* self, PyObject* args) {
    indri::index::VocabularyIterator* const vocabulary_it = self->index_->vocabularyIterator();

    PyObject* const token2id = PyDict_New();
    PyObject* const id2token = PyDict_New();
    PyObject* const id2df = PyDict_New();

    vocabulary_it->startIteration();

    while (!vocabulary_it->finished()) {
        indri::index::DiskTermData* const term_data = vocabulary_it->currentEntry();

        const lemur::api::TERMID_T term_id = term_data->termID;
        const string term = term_data->termData->term;

        const unsigned int document_frequency = term_data->termData->corpus.documentCount;
        CHECK_GT(document_frequency, 0);

        PyDict_SetItemString(token2id,
                             term.c_str(),
                             PyInt_FromLong(term_id));

        PyDict_SetItem(id2token,
                       PyInt_FromLong(term_id),
                       PyString_FromString(term.c_str()));

        PyDict_SetItem(id2df,
                       PyInt_FromLong(term_id),
                       PyInt_FromLong(document_frequency));

        vocabulary_it->nextEntry();
    }

    delete vocabulary_it;

    CHECK_EQ(PyDict_Size(token2id), self->index_->uniqueTermCount());
    CHECK_EQ(PyDict_Size(id2token), self->index_->uniqueTermCount());
    CHECK_EQ(PyDict_Size(id2df), self->index_->uniqueTermCount());

    return PyTuple_Pack(3, token2id, id2token, id2df);
}

static PyObject* Index_get_term_frequencies(Index* self, PyObject* args) {
    indri::index::VocabularyIterator* const vocabulary_it = self->index_->vocabularyIterator();

    PyObject* const id2tf = PyDict_New();

    vocabulary_it->startIteration();

    while (!vocabulary_it->finished()) {
        indri::index::DiskTermData* const term_data = vocabulary_it->currentEntry();

        const lemur::api::TERMID_T term_id = term_data->termID;

        const uint64_t term_frequency = term_data->termData->corpus.totalCount;
        CHECK_GT(term_frequency, 0);

        PyDict_SetItem(id2tf,
                       PyInt_FromLong(term_id),
                       PyInt_FromLong(term_frequency));

        vocabulary_it->nextEntry();
    }

    delete vocabulary_it;

    CHECK_EQ(PyDict_Size(id2tf), self->index_->uniqueTermCount());

    return id2tf;
}

static PyMethodDef Index_methods[] = {
    {"document", (PyCFunction) Index_document, METH_VARARGS,
     "Return a document (ext_document_id, terms) pair."},
    {"document_base", (PyCFunction) Index_document_base, METH_NOARGS,
     "Returns the lower bound document identifier (inclusive)."},
    {"maximum_document", (PyCFunction) Index_maximum_document, METH_NOARGS,
     "Returns the upper bound document identifier (exclusive)."},
    {"query", (PyCFunction) Index_run_query, METH_VARARGS,
     "Queries an Indri index."},
    {"get_dictionary", (PyCFunction) Index_get_dictionary, METH_NOARGS,
     "Extracts the dictionary from the index."},
    {"get_term_frequencies", (PyCFunction) Index_get_term_frequencies, METH_NOARGS,
     "Extracts the term frequencies from the index."},
    {NULL}  /* Sentinel */
};

static PyTypeObject IndexType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyndri.Index",             /*tp_name*/
    sizeof(Index),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) Index_dealloc, /*tp_dealloc*/
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
    "Index objects",           /* tp_doc */
    0,                   /* tp_traverse */
    0,                   /* tp_clear */
    0,                   /* tp_richcompare */
    0,                   /* tp_weaklistoffset */
    0,                   /* tp_iter */
    0,                   /* tp_iternext */
    Index_methods,             /* tp_methods */
    Index_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) Index_init,      /* tp_init */
    0,                         /* tp_alloc */
    Index_new,                 /* tp_new */
};

// Module methods.

static PyObject* pyndri_stem(PyObject* self, PyObject* args) {
    char* term;

    if (!PyArg_ParseTuple(args, "s", &term)) {
        return NULL;
    }

    static indri::parse::KrovetzStemmer stemmer;

    return PyString_FromString(stemmer.kstem_stemmer(term));
}

static PyMethodDef PyndriMethods[] = {
    {"stem", (PyCFunction) pyndri_stem, METH_VARARGS,
     "Return the Krovetz stemmed version of a term."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyndri(void) {
    if (PyType_Ready(&IndexType) < 0) {
        return;
    }

    PyObject* const module = Py_InitModule("pyndri", PyndriMethods);

    Py_INCREF(&IndexType);
    PyModule_AddObject(module, "Index", (PyObject*) &IndexType);
}
