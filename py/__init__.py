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
