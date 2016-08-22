import collections
import pyndri
import logging

__all__ = [
    'Dictionary',
    'extract_dictionary',
]


class Dictionary(object):
    """
    Interface to Indri dictionary.

    This class is compatible with gensim.
    """

    def __init__(self, token2id, id2token, id2df, krovetz_stemming=True):
        assert len(token2id) == len(id2token)

        self.token2id = token2id
        self.id2token = id2token

        self.dfs = id2df

        self.krovetz_stemming = krovetz_stemming

    def __getitem__(self, token_id):
        return self.id2token[token_id]

    def __contains__(self, token_id):
        return token_id in self.id2token

    def __iter__(self):
        return self.iterkeys()

    def keys(self):
        return self.iterkeys()

    def iterkeys(self):
        return self.id2token.iterkeys()

    def __len__(self):
        return len(self.id2token)

    def __str__(self):
        return 'IndriDictionary({0} unique tokens)'.format(
            len(self.id2token))

    def _process_token(self, token):
        if self.krovetz_stemming:
            return pyndri.stem(token)
        else:
            return token

    def translate_token(self, token):
        return self.token2id.get(self._process_token(token), None)

    def has_token(self, token):
        return self._process_token(token) in self.token2id

    def doc2bow(self, document):
        if isinstance(document, basestring):
            raise TypeError(
                'doc2bow expects an array of unicode tokens on input, '
                'not a single string')

        counter = collections.defaultdict(int)

        for token in document:
            if isinstance(token, int):
                token_id = token
            else:
                token_id = self.translate_token(token)

            if token_id is None:
                continue

            counter[token_id] += 1

        return sorted(counter.iteritems())


def extract_dictionary(index):
    assert isinstance(index, pyndri.Index)

    logging.debug('Extracting dictionary from index %s.', index)
    token2id, id2token, id2df = index.get_dictionary()

    return Dictionary(token2id, id2token, id2df)
