disable_opt = 0
gcc_exe = 'gcc'
gpp_exe = 'g++'
from distutils import sysconfig
save_init_posix = sysconfig._init_posix
def my_init_posix():
    save_init_posix()
    g = sysconfig._config_vars
    for n,r in [('LDSHARED',gpp_exe),('CC',gpp_exe)]:
        if g[n][:3]=='gcc':
            print 'my_init_posix: changing %s = %r'%(n,g[n]),
            g[n] = r+g[n][3:]
            print 'to',`g[n]`
    if disable_opt and g['OPT'][:15]=='-DNDEBUG -g -O3':
        print 'my_init_posix: changing OPT =',`g['OPT']`,
        g['OPT'] = ' -DNDEBUG -g '+g['OPT'][15:]
        print 'to',`g['OPT']`
sysconfig._init_posix = my_init_posix

from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

linkdefs = [
  'm',
  'pthread',
  'X11',           
  'stdc++',
  'pHash',
]

ext_modules = [Extension("pyphash", ["pyphash.pyx"],
                         extra_compile_args=["-g"],
                         libraries=linkdefs,
                         include_dirs=["../pHash/src"])]

setup(
  name = 'PyPHash',
  cmdclass = {'build_ext': build_ext},
  ext_modules = ext_modules
)