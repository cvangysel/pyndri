"""
Query environments example.

This example shows how to use query environments with different
retrieval models. The query() method supports the same arguments
as Index.query(); see query.py.
"""

import pyndri
import sys

if len(sys.argv) <= 1:
    print('Usage: python {0} <path-to-indri-index>'.format(sys.argv[0]))

    sys.exit(0)

with pyndri.open(sys.argv[1]) as index:
    # Constructs a QueryEnvironment that uses a
    # language model with Dirichlet smoothing.
    lm_query_env = pyndri.QueryEnvironment(
        index, rules=('method:dirichlet,mu:5000',))
    print(lm_query_env.query('hello world'))

    # Constructs a QueryEnvironment that uses the TF-IDF retrieval model.
    #
    # See "Baseline (non-LM) retrieval"
    # (https://lemurproject.org/doxygen/lemur/html/IndriRunQuery.html)
    tfidf_query_env = pyndri.TFIDFQueryEnvironment(index)
    print(tfidf_query_env.query('hello world'))

    # Constructs a QueryEnvironment that uses the Okapi BM25 retrieval model.
    #
    # See "Baseline (non-LM) retrieval"
    # (https://lemurproject.org/doxygen/lemur/html/IndriRunQuery.html)
    bm25_query_env = pyndri.OkapiQueryEnvironment(index)
    print(bm25_query_env.query('hello world'))
