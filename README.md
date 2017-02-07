pyndri
======

pyndri is a Python interface to the Indri search engine (http://www.lemurproject.org/indri/).

Requirements
------------

During development, we use Python 3.5. Some of the [examples](examples) require numpy.

**NOTE**: Python2 support has ended; if you want to continue using it, please check out the  `python2` tag.

Examples
--------

How to iterate over all documents in a repository:

    import pyndri

    index = pyndri.Index('/path/to/indri/index')

    for document_id in range(index.document_base(), index.maximum_document()):
        print(index.document(document_id))

The above will output pairs of external document identifiers and document terms:

    ('eUK237655', (877, 2171, 191, 2171))
    ('eUK956325', (880, 2171, 345, 2171))
    ('eUK458961', (3566, 1, 2, 3199, 504, 1726, 1, 3595, 1860, 1171, 1527))
    ('eUK390317', (3228, 2397, 2, 945, 1, 3097, 3, 145, 3769, 2102, 1556, 970, 3959))
    ('eUK794201', (770, 247, 1686, 3712, 1, 1085, 3, 830, 1445))

How to launch a Indri query to an index and get the identifiers and scores of retrieved documents:

    import pyndri

    index = pyndri.Index('/path/to/indri/index')

    # Queries the index with 'hello world' and returns the first 1000 results.
    results = index.query('hello world', results_requested=1000)

    for int_document_id, score in results:
        ext_document_id, _ = index.document(int_document_id)
        print(ext_document_id, score)

The above will print document identifiers and language modeling scores:

    eUK306804 -8.77414652243
    eUK700967 -8.8712247934
    eUK437700 -8.88184436222
    eUK107263 -8.89119022464
    ...

The token to term identifier mapping can be extracted as follows:

    import pyndri

    index = pyndri.Index('/path/to/indri/index')
    token2id, id2token, id2df = index.get_dictionary()

    id2tf = index.get_term_frequencies()

Citation
--------

If you use pyndri to produce results for your scientific publication, please refer to our [ECIR 2017](https://arxiv.org/abs/1701.00749) paper.

	@inproceedings{VanGysel2017pyndri,
	  title={Pyndri: a Python Interface to the Indri Search Engine},
	  author={Van Gysel, Christophe and Kanoulas, Evangelos and de Rijke, Maarten},
	  booktitle={ECIR},
	  volume={2017},
	  year={2017},
	  organization={Springer}
	}

Installation
------------

Install indri 5.10

python setup.py install

If indri is installed locally, you can (1) update setup.py or (2) update the enviroment variable: LD_LIBRARY_PATH and
export INDRI_INCLUDE_PATH which is set to the location of header files in the indri installation directory.


Frequently Asked Questions
--------------------------

### Importing `pyndri` in Python causes the error `Undefined symbol std::__cxx11::basic_string ...`

You are using GCC 5 (or above) and this version of the compiler includes new implementations of common types (`std::string`, etc.). You have to recompile Indri first by setting the `_GLIBCXX_USE_CXX11_ABI` macro to 0.

	make clean
	./configure CXX="g++ -D_GLIBCXX_USE_CXX11_ABI=0"
	make
	sudo make install

Afterwards, recompile pyndri from a clean install.

### Importing `pyndri` in Python causes the error `undefined symbol: std::throw_out_of_range_fmt ...`

Your Python version was compiled with a different standard library than you used to compile Indri and pyndri with. For example, the Anaconda distribution comes with pre-compiled binaries and its own standard library.

We do not provide support for this, as this is a problem with your Python installation and goes beyond the scope of this project. However, we've identified three possible paths:

   * Re-compile Python yourself from source.
   * Compile Indri and pyndri with the standard library of your Python distribution. This might be difficult, as the headers are often not included in the distribution.
   * Use the Python executables part of your Linux distribution. Be sure to install the development headers (e.g., `python3.5-dev` using `apt-get` on Ubuntu).

License
-------

Pyndri is licensed under the [MIT license](LICENSE). Please note that [Indri](http://www.lemurproject.org/indri.php) is licensed separately. If you modify Pyndri in any way, please link back to this repository.
