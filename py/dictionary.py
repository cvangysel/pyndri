import collections
import heapq
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

    def __init__(self, token2id, id2token, id2df, **kwargs):
        assert len(token2id) == len(id2token)

        self.token2id = token2id
        self.id2token = id2token

        self.dfs = id2df

        if 'krovetz_stemming' in kwargs:
            raise NotImplementedError('krovetz_stemming was deprecated '
                                      'in favour of Index.process_term.')

    def __getitem__(self, token_id):
        return self.id2token[token_id]

    def __contains__(self, token_id):
        return token_id in self.id2token

    def __iter__(self):
        return self.iterkeys()

    def keys(self):
        return self.iterkeys()

    def iterkeys(self):
        return self.id2token.keys()

    def __len__(self):
        return len(self.id2token)

    def __str__(self):
        return 'IndriDictionary({0} unique tokens)'.format(
            len(self.id2token))

    def translate_token(self, token):
        return self.token2id.get(token, None)

    def has_token(self, token):
        return token in self.token2id

    def doc2bow(self, document):
        if isinstance(document, str) or isinstance(document, bytes):
            raise TypeError(
                'doc2bow expects an iterable of unicode tokens on input, '
                'not a single string')

        counter = collections.defaultdict(int)

        for token in document:
            if isinstance(token, int):
                token_id = token
            else:
                assert isinstance(token, str)

                token_id = self.translate_token(token)

            if token_id is None:
                continue

            counter[token_id] += 1

        return sorted(counter.items())


def extract_dictionary(index,
                       max_terms=None, make_contiguous=False,
                       **kwargs):
    assert isinstance(index, pyndri.Index)

    logging.debug('Extracting dictionary from index %s.', index)
    token2id, id2token, id2df = index.get_dictionary()

    if max_terms is not None and max_terms:
        id2tf = index.get_term_frequencies()

        top_terms_ids = set(heapq.nlargest(
            max_terms,
            iterable=id2tf, key=lambda id: id2tf[id]))

        token2id = {key: value for key, value in token2id.items()
                    if value in top_terms_ids}
        id2token = {key: value for key, value in id2token.items()
                    if key in top_terms_ids}
        id2df = {key: value for key, value in id2df.items()
                 if key in top_terms_ids}

    if make_contiguous:
        # Dictionary does not have contiguous identifiers; wrap it.
        indri_id2cid = {id: cid for cid, id in enumerate(id2token)}
        id2token = {
            cid: id2token[id] for id, cid in indri_id2cid.items()}
        token2id = {
            token: cid for cid, token in id2token.items()}
        id2df = {
            cid: id2df[indri_id] for indri_id, cid in indri_id2cid.items()}
    else:
        indri_id2cid = None

    dictionary = Dictionary(token2id, id2token, id2df, **kwargs)

    if make_contiguous:
        return dictionary, indri_id2cid
    else:
        return dictionary
