from pyndri.dictionary import *
from pyndri_ext import *

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
        ord('\''): None,  # '"',
        ord('.'): ' ',
        ord(':'): ' ',
        ord('\t'): ' ',
        ord('/'): ' ',
        ord('&'): ' ',
        ord(','): ' ',
        ord('-'): ' ',
    })
