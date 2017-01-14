import os
#from distutils.core import setup, Extension
from setuptools import setup, Extension
import pkg_resources

platform = pkg_resources.get_platform()

if 'macosx' in platform:
    pyndri_ext = Extension(
        'pyndri_ext',
        sources=['src/pyndri.cpp'],
        extra_compile_args=['-std=c++11'],
        libraries=['indri', 'z', 'pthread', 'm'],
        library_dirs=list(
            filter(len, os.environ.get('LD_LIBRARY_PATH', '').split(':'))),
        undef_macros=['NDEBUG'])

else:
     pyndri_ext = Extension(
        'pyndri_ext',
        sources=['src/pyndri.cpp'],
        libraries=['indri', 'z', 'pthread', 'm'],
        library_dirs=list(
            filter(len, os.environ.get('LD_LIBRARY_PATH', '').split(':'))),
        define_macros=[('_GLIBCXX_USE_CXX11_ABI', '0'),
                    ('P_NEEDS_GNU_CXX_NAMESPACE', '1')],
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
