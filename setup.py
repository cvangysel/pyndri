from distutils.core import setup, Extension

pyndri = Extension(
    'pyndri',
    sources=['src/pyndri.cpp'],
    libraries=['indri', 'z', 'pthread', 'm'],
    define_macros=[('P_NEEDS_GNU_CXX_NAMESPACE', '1')])

setup(name='Distutils',
      version='1.0',
      description='pyndri is a Python interface to the Indri search engine',
      author='Christophe Van Gysel',
      author_email='cvangysel@uva.nl',
      url='http://ilps.science.uva.nl',
      ext_modules=[pyndri])
