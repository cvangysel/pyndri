from pyndri.dictionary import Dictionary, extract_dictionary

from pyndri_ext import Index as __IndexBase
from pyndri_ext import QueryEnvironment, krovetz_stem, porter_stem, tokenize

import os

__all__ = [
    'Index',
    'Dictionary',
    'QueryEnvironment',
    'extract_dictionary',
    'krovetz_stem',
    'porter_stem',
    'tokenize',
    'escape',
]


def escape(input):
    return input.translate({
        ord('('): None,
        ord(')'): None,
        ord('\''): None,
        ord('\"'): None,
        ord('.'): ' ',
        ord(':'): ' ',
        ord('\t'): ' ',
        ord('/'): ' ',
        ord('&'): ' ',
        ord(','): ' ',
        ord('-'): ' ',
        ord('?'): ' ',
        ord('+'): ' ',
        ord(';'): ' ',
        ord('`'): None,
        ord('$'): None,
        ord('<'): ' ',
        ord('>'): ' ',
        ord('%'): ' ',
        ord('@'): ' ',
        ord('\\'): ' ',
        ord('*'): ' ',
    })


class Index(__IndexBase):

    def __init__(self, *args, **kwargs):
        super(Index, self).__init__(*args, **kwargs)

        self.__default_query_env = QueryEnvironment(self)

    def query(self, *args, **kwargs):
        assert self.__default_query_env is not None, \
            'Index has been closed.'

        return self.__default_query_env.query(*args, **kwargs)

    def tokenize(self, string):
        return [
            self.process_term(token)
            for token in
            tokenize(escape(string))]

    def __len__(self):
        return self.maximum_document() - self.document_base()

    def __repr__(self):
        return '<pyndri.Index of {} documents>'.format(len(self))

    def close(self):
        assert self.__default_query_env is not None, \
            'Index has been closed.'

        del self.__default_query_env
        self.__default_query_env = None


class __IndexOpener(object):

    def __init__(self, path):
        if not os.path.isdir(path):
            raise IOError('Index path is not a directory.')
        elif not os.path.exists(os.path.join(path, 'manifest')):
            raise IOError('Index manifest not found.')

        self.path = path

    def __enter__(self):
        self.index = Index(self.path)
        return self.index

    def __exit__(self, type, value, traceback):
        self.index.close()

        del self.index
        self.index = None


open = __IndexOpener


class TFIDFQueryEnvironment(QueryEnvironment):

    def __init__(self, index, k1=1.2, b=0.75):
        super(TFIDFQueryEnvironment, self).__init__(
            index, baseline='tfidf,k1:{k1:.5f},b:{b:.5f}'.format(
                k1=k1, b=b))


class OkapiQueryEnvironment(QueryEnvironment):

    def __init__(self, index, k1=1.2, b=0.75, k3=7.0):
        super(OkapiQueryEnvironment, self).__init__(
            index, baseline='okapi,k1:{k1:.5f},b:{b:.5f},k3:{k3:.5f}'.format(
                k1=k1, b=b, k3=k3))
