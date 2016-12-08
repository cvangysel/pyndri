from pyndri.dictionary import Dictionary, extract_dictionary

from pyndri_ext import Index as __IndexBase
from pyndri_ext import QueryEnvironment, stem, tokenize

__all__ = [
    'Index',
    'Dictionary',
    'QueryEnvironment',
    'extract_dictionary',
    'stem',
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
    })


class Index(__IndexBase):

    def tokenize(self, string):
        return [
            self.process_term(token)
            for token in
            tokenize(escape(string))]


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
