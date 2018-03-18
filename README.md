# SIS

This is SIS 1.4, an unofficial release of SIS, the logic synthesis system
from UC Berkeley.  SIS is no longer supported, but since there are some people still using it, I'm making this release.

The primary features added on top of SIS 1.3 are:
- General code refactoring, like porting it to C11
- Exploring and deleting unused/unuseful parts (like `xsis`)
- Updating obsolete parts that are "uncompilable" under modern Linux systems
- Migrate to `doxygen` for code documentation
- Migrate from `autotools` to `cmake`
- Rewriting the `sis` shell using `GNU readline`
- Updating general documentation