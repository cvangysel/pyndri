import pyndri
import sys

if len(sys.argv) <= 1:
    print 'Usage: python {0} <path-to-indri-index>'.format(sys.argv[0])

    sys.exit(0)

index = pyndri.Index(sys.argv[1])

for document_id in xrange(index.document_base(), index.maximum_document()):
    # Prints pairs of form (external_document_id, terms).
    #
    # Example:
    #   ('eUK950521', (877, 2171, 797, 877, 2171, 2771, 1768, 1262, 2171))
    #   ('eUK436208', (381, 3346))
    print index.document(document_id)

# The following line will raise an exception, as there is no document
# with internal identifier 0.
print index.document(0)
