#include <Python.h>
#include "structmember.h"

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>

#include <antlr/NoViableAltException.hpp>
#include <antlr/MismatchedTokenException.hpp>
#include <antlr/TokenStreamRecognitionException.hpp>

#define private public
#include <indri/LocalQueryServer.hpp>
#undef private

#include <indri/CompressedCollection.hpp>
#include <indri/DiskIndex.hpp>
#include <indri/KrovetzStemmer.hpp>
#include <indri/QueryEnvironment.hpp>
#include <indri/QueryParserFactory.hpp>
#include <indri/QuerySpec.hpp>
#include <indri/Path.hpp>
#include <indri/Porter_Stemmer.hpp>
#include <indri/SnippetBuilder.hpp>

using std::string;

#define ENCODING "latin1"

#define CHECK(condition) assert(condition)
#define CHECK_EQ(first, second) assert(first == second)
#define CHECK_GT(first, second) assert(first > second)
#define CHECK_GE(first, second) assert(first >= second)
#define CHECK_NOTNULL(condition) assert(condition != 0)

// Helpers.
int PyDict_SetItemAndSteal(PyObject* p, PyObject* key, PyObject* val) {
    CHECK(key != Py_None);
    CHECK(val != Py_None);

    int ret = PyDict_SetItem(p, key, val);

    Py_XDECREF(key);
    Py_XDECREF(val);

    return ret;
}

static PyTypeObject IndexType;
static PyTypeObject QueryEnvironmentType;

// Index

typedef struct {
    PyObject_HEAD

    char* repository_path_;

    indri::api::Parameters* parameters_;

    indri::collection::CompressedCollection* collection_;
    indri::index::DiskIndex* index_;

    indri::api::QueryEnvironment* query_env_;
} Index;

static void Index_dealloc(Index* self) {
    // self->collection_->close();
    self->index_->close();
    // self->query_env_->close();

    delete self->parameters_;

    // delete self->collection_;
    delete self->index_;
    delete self->query_env_;

    delete [] self->repository_path_;
}

static PyObject* Index_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    Index* self;

    self = (Index*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->repository_path_ = NULL;

        self->parameters_ = new indri::api::Parameters;

        self->collection_ = new indri::collection::CompressedCollection;
        self->index_ = new indri::index::DiskIndex;

        self->query_env_ = new indri::api::QueryEnvironment;
    }

    return (PyObject*) self;
}

static int Index_init(Index* self, PyObject* args, PyObject* kwds) {
    const char* repository_path = NULL;

    static char* kwlist[] = {"repository_path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist,
                                     &repository_path)) {
        return -1;
    }

    const size_t repository_path_length = strlen(repository_path);

    self->repository_path_ = new char[repository_path_length + 1];
    memcpy(self->repository_path_, repository_path, repository_path_length);
    self->repository_path_[repository_path_length] = 0;

    // Load parameters.
    try {
        self->parameters_->loadFile(
            indri::file::Path::combine(repository_path, "manifest"));
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

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

    // TODO(cvangysel): possibly remove query_env_ in the future.
    try {
        self->query_env_->addIndex(repository_path);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

    return 0;
}

static PyMemberDef Index_members[] = {
    {"path", T_STRING, offsetof(Index, repository_path_), READONLY, "path to repository"},
    {NULL}  /* Sentinel */
};

static PyObject* Index_get_document_ids(Index* self, PyObject* args) {
    PyObject* external_doc_ids = NULL;

    if (!PyArg_ParseTuple(args, "O", &external_doc_ids)) {
        return NULL;
    }

    if (external_doc_ids == NULL) {
        return NULL;
    }

    PyObject* const iterator = PyObject_GetIter(external_doc_ids);
    PyObject *item;

    if (iterator == NULL) {
        PyErr_SetString(
            PyExc_TypeError,
            "Passed object is not iterable.");

        Py_DECREF(iterator);

        return NULL;
    }

    std::vector<std::string> ext_document_ids;

    while (item = PyIter_Next(iterator)) {
        CHECK(PyUnicode_CheckExact(item));

        PyObject* item_bytes = PyUnicode_AsEncodedString(item, ENCODING, "strict");
        char* const ext_document_id = PyBytes_AsString(item_bytes);

        // Create a copy of the string.
        ext_document_ids.push_back(ext_document_id);

        Py_DECREF(item_bytes);
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    std::vector<lemur::api::DOCID_T> int_doc_ids;

    try {
        int_doc_ids = self->query_env_->documentIDsFromMetadata("docno", ext_document_ids);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return NULL;
    }

    PyObject* const doc_ids_tuple = PyTuple_New(int_doc_ids.size());

    Py_ssize_t pos = 0;
    for (std::vector<lemur::api::DOCID_T>::iterator int_doc_ids_it = int_doc_ids.begin();
         int_doc_ids_it != int_doc_ids.end();
         ++int_doc_ids_it, ++pos) {
        const lemur::api::DOCID_T int_document_id = *int_doc_ids_it;

        std::string ext_document_id;

        try {
            ext_document_id = self->collection_->retrieveMetadatum(int_document_id, "docno");
        } catch (const lemur::api::Exception& e) {
            PyErr_SetString(PyExc_IOError, e.what().c_str());

            Py_DECREF(doc_ids_tuple);

            return NULL;
        }

        PyObject* py_ext_document_id = PyUnicode_Decode(
            ext_document_id.c_str(),
            ext_document_id.size(),
            ENCODING,
            "strict");

        PyObject* py_int_document_id = PyLong_FromLong(int_document_id);

        PyTuple_SetItem(doc_ids_tuple,
                        pos,
                        PyTuple_Pack(
                            2,
                            py_ext_document_id,
                            py_int_document_id));

        Py_DECREF(py_ext_document_id);
        Py_DECREF(py_int_document_id);
    }

    return doc_ids_tuple;
}

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

    string ext_document_id;
    const indri::index::TermList* term_list = 0;

    try {
        ext_document_id = self->collection_->retrieveMetadatum(
            int_document_id, "docno");
        term_list = self->index_->termList(int_document_id);
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        if (term_list != 0) {
            delete term_list;
        }

        return NULL;
    }

    PyObject* terms = PyTuple_New(term_list->terms().size());

    indri::utility::greedy_vector<lemur::api::TERMID_T>::const_iterator term_it =
        term_list->terms().begin();

    Py_ssize_t pos = 0;
    for (; term_it != term_list->terms().end(); ++term_it, ++pos) {
        PyTuple_SetItem(terms, pos, PyLong_FromLong(*term_it));
    }

    delete term_list;

    PyObject* name = PyUnicode_Decode(
        ext_document_id.c_str(),
        ext_document_id.size(),
        ENCODING,
        "strict");

    PyObject* ret = PyTuple_Pack(2, name, terms);

    Py_DECREF(name);
    Py_DECREF(terms);

    return ret;
}

static PyObject* Index_ext_document_id(Index* self, PyObject* args) {
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

    string ext_document_id;

    try {
        ext_document_id = self->collection_->retrieveMetadatum(
            int_document_id, "docno");
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return NULL;
    }

    return PyUnicode_Decode(ext_document_id.c_str(),
                            ext_document_id.size(),
                            ENCODING,
                            "strict");
}

static PyObject* Index_document_base(Index* self) {
    return PyLong_FromLong(self->index_->documentBase());
}

static PyObject* Index_maximum_document(Index* self) {
    return PyLong_FromLong(self->index_->documentMaximum());
}

static PyObject* Index_document_count(Index* self) {
    return PyLong_FromLong(self->index_->documentCount());
}

static PyObject* Index_total_terms(Index* self) {
    return PyLong_FromLong(self->index_->termCount());
}

static PyObject* Index_unique_terms(Index* self) {
    return PyLong_FromLong(self->index_->uniqueTermCount());
}

static PyObject* Index_term_count(Index* self, PyObject* args) {
    char* term_object;

    if (!PyArg_ParseTuple(args, "s", &term_object)) {
        return NULL;
    }

    return PyLong_FromLong(self->index_->termCount(term_object));
}

static PyObject* Index_process_term(Index* self, PyObject* args) {
    char* term_object;

    if (!PyArg_ParseTuple(args, "s", &term_object)) {
        return NULL;
    }

    indri::collection::Repository* const repository =
        &dynamic_cast<indri::server::LocalQueryServer*>(
            self->query_env_->getServers()[0])->_repository;

    const std::string processed_term =
        repository->processTerm(term_object);

    return PyUnicode_Decode(processed_term.c_str(),
                            processed_term.size(),
                            ENCODING,
                            "strict");
}

static PyObject* Index_document_length(Index* self, PyObject* args) {
    int int_document_id;

    if (!PyArg_ParseTuple(args, "i", &int_document_id)) {
        return NULL;
    }

    return PyLong_FromLong(self->index_->documentLength(int_document_id));
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

        PyDict_SetItemAndSteal(
            token2id,
            PyUnicode_Decode(term.c_str(),
                             term.size(),
                             ENCODING,
                             "strict"),
            PyLong_FromLong(term_id));

        PyDict_SetItemAndSteal(
            id2token,
            PyLong_FromLong(term_id),
            PyUnicode_Decode(term.c_str(),
                             term.size(),
                             ENCODING,
                             "strict"));

        PyDict_SetItemAndSteal(
            id2df,
            PyLong_FromLong(term_id),
            PyLong_FromLong(document_frequency));

        vocabulary_it->nextEntry();
    }

    delete vocabulary_it;

    CHECK_EQ(PyDict_Size(token2id), self->index_->uniqueTermCount());
    CHECK_EQ(PyDict_Size(id2token), self->index_->uniqueTermCount());
    CHECK_EQ(PyDict_Size(id2df), self->index_->uniqueTermCount());

    PyObject* ret = PyTuple_Pack(3, token2id, id2token, id2df);

    Py_DECREF(token2id);
    Py_DECREF(id2token);
    Py_DECREF(id2df);

    return ret;
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

        PyDict_SetItemAndSteal(id2tf,
                               PyLong_FromLong(term_id),
                               PyLong_FromLong(term_frequency));

        vocabulary_it->nextEntry();
    }

    delete vocabulary_it;

    CHECK_EQ(PyDict_Size(id2tf), self->index_->uniqueTermCount());

    return id2tf;
}

static PyMethodDef Index_methods[] = {
    {"document_ids", (PyCFunction) Index_get_document_ids, METH_VARARGS,
     "Returns the internal DOC_IDs given the external identifiers."},
    {"document", (PyCFunction) Index_document, METH_VARARGS,
     "Return a document (ext_document_id, terms) pair."},
    {"ext_document_id", (PyCFunction) Index_ext_document_id, METH_VARARGS,
     "Return a document external identifier pair."},
    {"document_base", (PyCFunction) Index_document_base, METH_NOARGS,
     "Returns the lower bound document identifier (inclusive)."},
    {"maximum_document", (PyCFunction) Index_maximum_document, METH_NOARGS,
     "Returns the upper bound document identifier (exclusive)."},

    {"document_count", (PyCFunction) Index_document_count, METH_NOARGS,
     "Returns the number of documents in the index."},
    {"document_length", (PyCFunction) Index_document_length, METH_VARARGS,
     "Returns the length of a document."},

    {"total_terms", (PyCFunction) Index_total_terms, METH_NOARGS,
     "Returns the number of total terms in the index."},
    {"unique_terms", (PyCFunction) Index_unique_terms, METH_NOARGS,
     "Returns the number of unique terms in the index."},

    {"term_count", (PyCFunction) Index_term_count, METH_VARARGS,
     "Return the term frequency for a term."},

    {"process_term", (PyCFunction) Index_process_term, METH_VARARGS,
     "Pre-processes an index term."},

    {"get_dictionary", (PyCFunction) Index_get_dictionary, METH_NOARGS,
     "Extracts the dictionary from the index."},
    {"get_term_frequencies", (PyCFunction) Index_get_term_frequencies, METH_NOARGS,
     "Extracts the term frequencies from the index."},
    {NULL}  /* Sentinel */
};

// QueryEnvironment

typedef struct {
    PyObject_HEAD

    PyObject* index_;
    indri::api::QueryEnvironment* query_env_;
} QueryEnvironment;

static void QueryEnvironment_dealloc(QueryEnvironment* self) {
    Py_DECREF(self->index_);
    self->index_ = NULL;

    self->query_env_->close();

    // self->query_env_->close();
    delete self->query_env_;
}

static PyObject* QueryEnvironment_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    QueryEnvironment* self;

    self = (QueryEnvironment*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->index_ = NULL;
        self->query_env_ = new indri::api::QueryEnvironment;
    }

    return (PyObject*) self;
}

static int QueryEnvironment_init(QueryEnvironment* self, PyObject* args, PyObject* kwds) {
    PyObject* index_obj = NULL;
    PyObject* rules_obj = NULL;
    PyObject* baseline_obj = NULL;

    static char* kwlist[] = {"index", "rules", "baseline",
                             NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!O!", kwlist,
                                     &IndexType, &index_obj,
                                     &PyTuple_Type, &rules_obj,
                                     &PyUnicode_Type, &baseline_obj)) {
        return -1;
    }

    self->index_ = index_obj;
    Py_INCREF(self->index_);

    PyObject* repostitory_path_obj = PyObject_GetAttrString(
        self->index_, "path");
    CHECK_NOTNULL(repostitory_path_obj);

    try {
        self->query_env_->addIndex(PyUnicode_AsUTF8(repostitory_path_obj));
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        return -1;
    }

    Py_DECREF(repostitory_path_obj);

    if (rules_obj != NULL && baseline_obj != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to specify smoothing rules for baseline.");

        return -1;
    } else if (rules_obj != NULL) {
        std::vector<std::string> rules;

        for (Py_ssize_t i = 0; i < PyTuple_Size(rules_obj); ++i) {
            rules.push_back(PyUnicode_AsUTF8(PyTuple_GetItem(rules_obj, i)));
        }

        self->query_env_->setScoringRules(rules);
    } else if (baseline_obj != NULL) {
        const std::string baseline = PyUnicode_AsUTF8(baseline_obj);
        self->query_env_->setBaseline(baseline);
    }

    return 0;
}

static PyObject* QueryEnvironment_run_query(QueryEnvironment* self, PyObject* args, PyObject* kwds) {
    PyObject* query = NULL;
    PyObject* document_set = NULL;
    long results_requested = 0;
    bool include_snippets = false;

    static char* kwlist[] = {"query_str",
                             "document_set",
                             "results_requested",
                             "include_snippets",
                             NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|Olb", kwlist,
                                     &query,
                                     &document_set,
                                     &results_requested,
                                     &include_snippets)) {
        return NULL;
    }

    CHECK(PyUnicode_Check(query));

    PyObject* query_bytes = PyUnicode_AsEncodedString(query, ENCODING, "strict");
    char* query_str = PyBytes_AsString(query_bytes);

    std::vector<lemur::api::DOCID_T> document_ids;

    if (document_set != NULL) {
        CHECK(PyIter_Check(document_set));

        PyObject* const iterator = PyObject_GetIter(document_set);
        PyObject *item;

        if (iterator == NULL) {
            PyErr_SetString(
                PyExc_TypeError,
                "Passed object for document_set not iterable.");

            Py_DECREF(iterator);
            Py_DECREF(query_bytes);

            return NULL;
        }

        while (item = PyIter_Next(iterator)) {
            CHECK(PyLong_CheckExact(item));

            const lemur::api::DOCID_T int_doc_id = PyLong_AsLong(item);
            if (int_doc_id < 0) {
                continue;
            }

            document_ids.push_back(int_doc_id);

            Py_DECREF(item);
        }

        Py_DECREF(iterator);
    }

    if (results_requested <= 0) {
        if (document_set != NULL) {
            results_requested = document_ids.size();
        } else {
            results_requested = 100;
        }
    }

    CHECK_GE(results_requested, 0);

    indri::api::QueryAnnotation* query_annotation;

    try {
        if (document_ids.empty()) {
            query_annotation = self->query_env_->runAnnotatedQuery(
                query_str, results_requested);
        } else{
            query_annotation = self->query_env_->runAnnotatedQuery(
                query_str, document_ids, results_requested);
        }
    } catch (const lemur::api::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what().c_str());

        Py_DECREF(query_bytes);

        return NULL;
    }

    Py_DECREF(query_bytes);

    std::vector<indri::api::ScoredExtentResult> query_results =
        query_annotation->getResults();

    std::vector<indri::api::ScoredExtentResult>::const_iterator it = query_results.begin();

    std::vector<string> snippets;

    if (include_snippets) {
        indri::api::SnippetBuilder builder(false /* html */);

        std::vector<lemur::api::DOCID_T> documentIDs(query_results.size(), 0);

        for (size_t i = 0; i < query_results.size(); ++i) {
            documentIDs[i] = query_results[i].document;
        }

        try {
            const std::vector<indri::api::ParsedDocument*> documents =
                self->query_env_->documents(documentIDs);

            for (size_t i = 0; i < query_results.size(); ++i) {
                snippets.push_back(
                    builder.build(documentIDs[i], documents[i], query_annotation));

                delete documents[i];
            }
        } catch (const lemur::api::Exception& e) {}
    }

    delete query_annotation;

    if (include_snippets && snippets.empty()) {
        PyErr_SetString(PyExc_IOError,
                        "Unable to retrieve snippets. "
                        "Make sure storeDocs is enabled "
                        "in your Indri configuration.");

        return NULL;
    }

    PyObject* results = PyTuple_New(query_results.size());

    Py_ssize_t pos = 0;
    for (; it != query_results.end(); ++it, ++pos) {
        PyObject* const result = PyTuple_New(include_snippets ? 3 : 2);

        PyTuple_SetItem(result, 0, PyLong_FromLong(it->document));
        PyTuple_SetItem(result, 1, PyFloat_FromDouble(it->score));

        if (include_snippets) {
            PyTuple_SetItem(result, 2, PyUnicode_Decode(snippets[pos].c_str(),
                                                        snippets[pos].size(),
                                                        ENCODING,
                                                        "strict"));
        }

       PyTuple_SetItem(results, pos, result);
    }

    query_results.clear();

    return results;
}

static PyMemberDef QueryEnvironment_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef QueryEnvironment_methods[] = {
    {"query", (PyCFunction) QueryEnvironment_run_query, METH_VARARGS | METH_KEYWORDS,
     "Queries an Indri index."},

    {NULL}  /* Sentinel */
};

// Module methods.

static PyObject* pyndri_krovetz_stem(PyObject* self, PyObject* args) {
    PyObject* term;

    if (!PyArg_ParseTuple(args, "U", &term)) {
        return NULL;
    }

    CHECK(PyUnicode_Check(term));

    PyObject* term_bytes = PyUnicode_AsEncodedString(term, ENCODING, "strict");

    if (term_bytes == NULL) {
        return NULL;
    }

    char* term_str = PyBytes_AsString(term_bytes);

    static indri::parse::KrovetzStemmer stemmer;
    std::string stemmed_term(stemmer.kstem_stemmer(term_str));

    Py_DECREF(term_bytes);

    PyObject* result = PyUnicode_Decode(stemmed_term.c_str(),
                                        stemmed_term.size(),
                                        ENCODING,
                                        "strict");

    // Canonical; to indicate that result can be NULL.
    if (result == NULL) {
        return NULL;
    }

    return result;
}

static PyObject* pyndri_porter_stem(PyObject* self, PyObject* args) {
    PyObject* term;

    if (!PyArg_ParseTuple(args, "U", &term)) {
        return NULL;
    }

    CHECK(PyUnicode_Check(term));

    PyObject* term_bytes = PyUnicode_AsEncodedString(term, ENCODING, "strict");

    if (term_bytes == NULL) {
        return NULL;
    }

    std::string term_str(PyBytes_AsString(term_bytes)); // Takes a copy.
    Py_DECREF(term_bytes);

    char* const stemmed_term = new char[term_str.length() + 1];
    term_str.copy(stemmed_term, term_str.length());
    stemmed_term[term_str.length()] = '\0';

    static indri::parse::Porter_Stemmer stemmer;
    const int new_end = stemmer.porter_stem(stemmed_term, 0, term_str.length() - 1);

    PyObject* result = PyUnicode_Decode(stemmed_term,
                                        new_end + 1,
                                        ENCODING,
                                        "strict");

    delete [] stemmed_term;

    // Canonical; to indicate that result can be NULL.
    if (result == NULL) {
        return NULL;
    }

    return result;
}

class TokenExtractor : public indri::lang::Walker {
 public:
    explicit TokenExtractor(std::vector<std::string>* const tokens) : tokens_(tokens) {
        tokens_->clear();
    }

    virtual void defaultBefore(indri::lang::Node* node) {}

    virtual void after(indri::lang::IndexTerm* node) {
        tokens_->push_back(node->getText());
    }

 private:
    std::vector<std::string>* const tokens_;
};

static PyObject* pyndri_tokenize(PyObject* self, PyObject* args) {
    PyObject* input;

    if (!PyArg_ParseTuple(args, "U", &input)) {
        return NULL;
    }

    PyObject* input_bytes = PyUnicode_AsEncodedString(input, ENCODING, "strict");
    char* input_str = PyBytes_AsString(input_bytes);

    indri::api::QueryParserWrapper* const parser = indri::api::QueryParserFactory::get(input_str, "indri");

    indri::lang::ScoredExtentNode* root_node = NULL;

    try {
        root_node = parser->query();
    } catch (const antlr::NoViableAltException& e) {
        PyErr_SetString(PyExc_IOError, e.getMessage().c_str());
    } catch (const antlr::MismatchedTokenException& e) {
        PyErr_SetString(PyExc_IOError, e.getMessage().c_str());
    } catch (const antlr::TokenStreamRecognitionException& e) {
        PyErr_SetString(PyExc_IOError, e.getMessage().c_str());
    }

    Py_DECREF(input_bytes);

    if (root_node == NULL) {
        return NULL;
    }

    std::vector<std::string> tokens;

    TokenExtractor extractor(&tokens);
    root_node->walk(extractor);

    PyObject* const tokens_tuple = PyTuple_New(tokens.size());

    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        PyTuple_SetItem(tokens_tuple, idx, PyUnicode_Decode(tokens[idx].c_str(),
                                                            tokens[idx].size(),
                                                            ENCODING,
                                                            "strict"));
    }

    return tokens_tuple;
}

static PyMethodDef PyndriMethods[] = {
    {"krovetz_stem", (PyCFunction) pyndri_krovetz_stem, METH_VARARGS,
     "Return the Krovetz stemmed version of a term."},
    {"porter_stem", (PyCFunction) pyndri_porter_stem, METH_VARARGS,
     "Return the Porter stemmed version of a term."},
    {"tokenize", (PyCFunction) pyndri_tokenize, METH_VARARGS,
     "Tokenize an input string."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef PyndriModule = {
    PyModuleDef_HEAD_INIT,
    "pyndri_ext",
    "Python interface to the Indri search engine.",
    -1,
    PyndriMethods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_pyndri_ext(void) {
    IndexType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "pyndri.Index",             /* tp_name */
        sizeof(Index),             /* tp_basicsize */
        0,                         /* tp_itemsize */
        (destructor) Index_dealloc, /* tp_dealloc */
        0,                         /* tp_print */
        0,                         /* tp_getattr */
        0,                         /* tp_setattr */
        0,                         /* tp_reserved */
        0,                         /* tp_repr */
        0,                         /* tp_as_number */
        0,                         /* tp_as_sequence */
        0,                         /* tp_as_mapping */
        0,                         /* tp_hash */
        0,                         /* tp_call */
        0,                         /* tp_str */
        0,                         /* tp_getattro */
        0,                         /* tp_setattro */
        0,                         /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
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

    if (PyType_Ready(&IndexType) < 0) {
        return NULL;
    }

    QueryEnvironmentType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "pyndri.QueryEnvironment",             /* tp_name */
        sizeof(QueryEnvironment),             /* tp_basicsize */
        0,                         /* tp_itemsize */
        (destructor) QueryEnvironment_dealloc, /* tp_dealloc */
        0,                         /* tp_print */
        0,                         /* tp_getattr */
        0,                         /* tp_setattr */
        0,                         /* tp_reserved */
        0,                         /* tp_repr */
        0,                         /* tp_as_number */
        0,                         /* tp_as_sequence */
        0,                         /* tp_as_mapping */
        0,                         /* tp_hash */
        0,                         /* tp_call */
        0,                         /* tp_str */
        0,                         /* tp_getattro */
        0,                         /* tp_setattro */
        0,                         /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
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

    if (PyType_Ready(&QueryEnvironmentType) < 0) {
        return NULL;
    }

    PyObject* const module = PyModule_Create(&PyndriModule);

    if (module == NULL) {
        return NULL;
    }

    Py_INCREF(&IndexType);
    PyModule_AddObject(module, "Index", (PyObject*) &IndexType);

    Py_INCREF(&QueryEnvironmentType);
    PyModule_AddObject(module, "QueryEnvironment", (PyObject*) &QueryEnvironmentType);

    return module;
}
