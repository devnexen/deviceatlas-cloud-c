from distutils.core import setup, Extension
import os
import sys

if sys.version_info[0] < 3:
    raise "Python 3 only is supported"

cflags = '.'
ldflags = ''

if 'CFLAGS' in os.environ:
    cflags = os.environ['CFLAGS']

if 'LDFLAGS' in os.environ:
    ldflags = os.environ['LDFLAGS']
dacloud = Extension('dacloud', sources = [ 'dacloud.c'], libraries = [ 'dacloud' ],
            include_dirs = [ cflags ], extra_link_args = [ ldflags ])

setup ( name = 'dacloudmod', version = '0.1.0', description = 'Python 3 wrapper',
            ext_modules = [ dacloud ], author = 'David Carlier', author_email = 'devnexen@gmail.com',
            long_description = 'DeviceAtlas C Cloud API Python 3 wrapper' )
