import pyndri
import sys

import numpy as np
import scipy.stats.mstats

if len(sys.argv) <= 1:
    print('Usage: python {0} <path-to-indri-index> [<index-name>]'.format(
        sys.argv[0]))

    sys.exit(0)

with pyndri.open(sys.argv[1]) as index:
    num_documents = 0
    mean = 0.0
    M2 = 0.0

    min_ = np.inf
    max_ = -np.inf

    lengths = []

    for document_id in range(index.document_base(), index.maximum_document()):
        x = float(index.document_length(document_id))
        lengths.append(x)

        num_documents += 1
        delta = x - mean
        mean += delta / num_documents
        M2 += delta * (x - mean)

        min_ = min(min_, x)
        max_ = max(max_, x)

    std = np.sqrt(M2 / (num_documents - 1))

    mode = scipy.stats.mstats.mode(lengths)
    median = np.median(lengths)

    prefix = '' if len(sys.argv) == 2 else '{}_'.format(sys.argv[2].upper())

    print('export {}NUM={}'.format(prefix, num_documents))
    print('export {}LENGTH_MEDIAN={}'.format(prefix, median))
    print('export {}LENGTH_MODE={}'.format(prefix, mode.mode[0]))
    print('export {}LENGTH_MEAN={}'.format(prefix, mean))
    print('export {}LENGTH_MIN={}'.format(prefix, min_))
    print('export {}LENGTH_MAX={}'.format(prefix, max_))
    print('export {}LENGTH_STD={}'.format(prefix, std))
    print('export {}TOTAL_TERMS={}'.format(prefix, index.total_terms()))
    print('export {}UNIQUE_TERMS={}'.format(prefix, index.unique_terms()))
