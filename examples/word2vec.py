import copy
import gensim
import logging
import pyndri
import pyndri.compat
import sys

logging.basicConfig(level=logging.INFO)

if len(sys.argv) <= 1:
    logging.error('Usage: python %s <path-to-indri-index>'.format(sys.argv[0]))

    sys.exit(0)

logging.info('Initializing word2vec.')

word2vec_init = gensim.models.Word2Vec(
    size=300,  # Embedding size
    window=5,  # One-sided window size
    sg=True,  # Skip-gram.
    min_count=5,  # Minimum word frequency.
    sample=1e-3,  # Sub-sample threshold.
    hs=False,  # Hierarchical softmax.
    negative=10,  # Number of negative examples.
    iter=1,  # Number of iterations.
    workers=8,  # Number of workers.
)

with pyndri.open(sys.argv[1]) as index:
    logging.info('Loading vocabulary.')
    dictionary = pyndri.extract_dictionary(index)
    sentences = pyndri.compat.IndriSentences(index, dictionary)

    logging.info('Constructing word2vec vocabulary.')

    # Build vocab.
    word2vec_init.build_vocab(sentences, trim_rule=None)

    models = [word2vec_init]

    for epoch in range(1, 5 + 1):
        logging.info('Epoch %d', epoch)

        model = copy.deepcopy(models[-1])
        model.train(sentences)

        models.append(model)

    logging.info('Trained models: %s', models)
