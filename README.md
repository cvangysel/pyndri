pyndri
======

pyndri is a Python interface to the Indri search engine (http://www.lemurproject.org/indri/).

Here's an example:

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