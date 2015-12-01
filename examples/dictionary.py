import pyndri
import sys

if len(sys.argv) <= 1:
    print 'Usage: python {0} <path-to-indri-index>'.format(sys.argv[0])

    sys.exit(0)

index = pyndri.Index(sys.argv[1])

token2id, id2token, id2df, id2tf = index.get_dictionary()
print token2id
