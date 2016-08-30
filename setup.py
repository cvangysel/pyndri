import os
from distutils.core import setup, Extension

pyndri_ext = Extension(
    'pyndri_ext',
    sources=['src/pyndri.cpp'],
    libraries=['indri', 'z', 'pthread', 'm'],
    library_dirs=os.environ.get('LD_LIBRARY_PATH', '').split(':'),
    define_macros=[('P_NEEDS_GNU_CXX_NAMESPACE', '1')],
    undef_macros=['NDEBUG'])

setup(name='pyndri',
      version='1.0',
      description='pyndri is a Python interface to the Indri search engine',
      author='Christophe Van Gysel',
      author_email='cvangysel@uva.nl',
      url='http://ilps.science.uva.nl',
      ext_modules=[pyndri_ext],
      packages=['pyndri'],
      package_dir={'pyndri': 'py'})
