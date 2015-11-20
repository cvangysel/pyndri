import pyndri
import sys

if len(sys.argv) <= 1:
    print 'Usage: python {0} <path-to-indri-index>'.format(sys.argv[0])

    sys.exit(0)

query_environment = pyndri.QueryEnvironment(sys.argv[1])
repository = pyndri.IndriRepository(sys.argv[1])

results = query_environment.run_query('hello world', 1000)

for int_document_id, score in results:
    ext_document_id, _ = repository.document(int_document_id)

    print 'Document {ext_document_id} retrieved with score {score}.'.format(
        ext_document_id=ext_document_id, score=score)
