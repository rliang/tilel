import os
import ycm_core

flags = [
'-x', 'c',
'-std=c11',
'-I', '/usr/include/xcb/',
'-Wall',
'-Wextra',
'-pedantic',
'-DUSE_CLANG_COMPLETER'
]

def DirectoryOfThisScript():
  return os.path.dirname(os.path.abspath(__file__))

def FlagsForFile(filename):
  return {
    'flags': flags,
    'do_cache': True
  }
