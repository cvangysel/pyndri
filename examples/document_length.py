import pyndri
import sys

if len(sys.argv) <= 2:
    print('Usage: python {0} <path-to-indri-index> '
          '[<index-name>] [<max-document-length>]'.format(sys.argv[0]))

    sys.exit(0)

with pyndri.open(sys.argv[1]) as index:
    for document_id in range(index.document_base(), index.maximum_document()):
        if index.document_length(document_id) < int(sys.argv[2]):
            print(index.document(document_id)[0])
