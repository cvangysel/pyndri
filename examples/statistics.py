import pyndri
import sys

import numpy as np

if len(sys.argv) <= 1:
    print('Usage: python {0} <path-to-indri-index> [<index-name>]'.format(
        sys.argv[0]))

    sys.exit(0)

index = pyndri.Index(sys.argv[1])

num_documents = 0
mean = 0.0
M2 = 0.0

for document_id in range(index.document_base(), index.maximum_document()):
    x = float(index.document_length(document_id))

    num_documents += 1
    delta = x - mean
    mean += delta / num_documents
    M2 += delta * (x - mean)

std = np.sqrt(M2 / (num_documents - 1))

prefix = '' if len(sys.argv) == 2 else '{}_'.format(sys.argv[2].upper())

print('export {}NUM={}'.format(prefix, num_documents))
print('export {}LENGTH_MEAN={}'.format(prefix, mean))
print('export {}LENGTH_STD={}'.format(prefix, std))
print('export {}TOTAL_TERMS={}'.format(prefix, index.total_terms()))
print('export {}UNIQUE_TERMS={}'.format(prefix, index.unique_terms()))
