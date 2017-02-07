import os
from distutils.core import setup, Extension

pyndri_ext = Extension(
    'pyndri_ext',
    sources=['src/pyndri.cpp'],
    libraries=['indri', 'z', 'pthread', 'm'],
    library_dirs=list(
        filter(len, os.environ.get('LD_LIBRARY_PATH', '').split(':'))),
    include_dirs=[os.environ.get('INDRI_INCLUDE_PATH', '')],
    define_macros=[('_GLIBCXX_USE_CXX11_ABI', '0'),
                   ('P_NEEDS_GNU_CXX_NAMESPACE', '1')],
    undef_macros=['NDEBUG'])

setup(name='pyndri',
      version='0.1',
      description='pyndri is a Python interface to the Indri search engine',
      author='Christophe Van Gysel',
      author_email='cvangysel@uva.nl',
      ext_modules=[pyndri_ext],
      packages=['pyndri'],
      package_dir={'pyndri': 'py'},
      url='https://github.com/cvangysel/pyndri',
      download_url='https://github.com/cvangysel/pyndri/tarball/0.1',
      keywords=['indri', 'language models', 'retrieval', 'indexing'],
      classifiers=[
          'Development Status :: 3 - Alpha',
          'License :: OSI Approved :: MIT License',
          'Programming Language :: Python',
          'Programming Language :: C++',
          'Intended Audience :: Science/Research',
          'Operating System :: POSIX :: Linux',
          'Topic :: Text Processing :: Indexing',
      ])
