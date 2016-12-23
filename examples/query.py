import operator
import pyndri
import sys

if len(sys.argv) <= 1:
    print('Usage: python {0} <path-to-indri-index>'.format(sys.argv[0]))

    sys.exit(0)

with pyndri.open(sys.argv[1]) as index:
    results = index.query('hello world', results_requested=10)

    for int_document_id, score in results:
        ext_document_id, _ = index.document(int_document_id)

        print('Document {ext_document_id} retrieved with score {score}.'.format(
            ext_document_id=ext_document_id, score=score))

    print()

    results = index.query(
        'hello world',
        document_set=map(
            operator.itemgetter(1),
            index.document_ids(['eUK306804', 'eUK700967'])),
        results_requested=-5,
        include_snippets=True)

    for int_document_id, score, snippet in results:
        ext_document_id, _ = index.document(int_document_id)

        print('Document {ext_document_id} ("{snippet}") '
              'retrieved with score {score}.'.format(
                  ext_document_id=ext_document_id,
                  snippet=snippet.replace('\n', ' '),
                  score=score))
