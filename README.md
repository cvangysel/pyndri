pyndri
======

pyndri is a Python interface to the [Indri search engine](http://www.lemurproject.org/indri/). It is API-compatible with the latest Indri version (5.9, released in June 2015) and supersedes the outdated [pymur](http://findingscience.com/pymur/).

How to iterate over all documents in a repository:

    import pyndri

    repository = pyndri.IndriRepository('/path/to/indri/index')

    for document_id in xrange(repository.document_base(),
                              repository.maximum_document()):
        print repository.document(document_id)

The above will output pairs of external document identifiers and document terms:

    ('eUK237655', (877, 2171, 191, 2171))
    ('eUK956325', (880, 2171, 345, 2171))
    ('eUK458961', (3566, 1, 2, 3199, 504, 1726, 1, 3595, 1860, 1171, 1527))
    ('eUK390317', (3228, 2397, 2, 945, 1, 3097, 3, 145, 3769, 2102, 1556, 970, 3959))
    ('eUK794201', (770, 247, 1686, 3712, 1, 1085, 3, 830, 1445))

How to launch a Indri query to an index and get the identifiers and scores of retrieved documents:

    import pyndri

    query_environment = pyndri.QueryEnvironment('/path/to/indri/index')
    repository = pyndri.IndriRepository('/path/to/indri/index')

    # Queries the index with 'hello world' and returns the first 1000 results.
    results = query_environment.run_query('hello world', 1000)

    for int_document_id, score in results:
        ext_document_id, _ = repository.document(int_document_id)
        print ext_document_id, score

The above will print document identifiers and language modeling scores:

    eUK306804, -8.77414652243
    eUK700967, -8.8712247934
    eUK437700, -8.88184436222
    eUK107263, -8.89119022464
    ...
