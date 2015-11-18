pyndri
======

pyndri is a Python interface to the Indri search engine (http://www.lemurproject.org/indri/).

Here's an example:

    import pyndri

    repository = pyndri.IndriRepository('/path/to/indri/index')

    for document_id in xrange(repository.document_base(),
                              repository.maximum_document()):
        print repository.document(document_id)