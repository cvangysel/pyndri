import pyndri
import sys

if len(sys.argv) <= 1:
    print 'Usage: python {0} <path-to-indri-index>'.format(sys.argv[0])

    sys.exit(0)

index = pyndri.Index(sys.argv[1])

results = index.query('hello world', results_requested=10)

for int_document_id, score in results:
    ext_document_id, _ = index.document(int_document_id)

    print 'Document {ext_document_id} retrieved with score {score}.'.format(
        ext_document_id=ext_document_id, score=score)

results = index.query(
    'hello world',
    document_set=index.document_ids(['eUK306804', 'eUK700967']),
    results_requested=-5)

for int_document_id, score in results:
    print int_document_id, score
