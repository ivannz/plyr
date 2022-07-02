import os
from setuptools import setup, Extension

# update the version number
version = open("VERSION", "r").read().strip()

# graft a dunder-version file into the root of the package
# XXX dunder-init imports dunder-version
cwd = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(cwd, "plyr", "__version__.py"), "w") as f:
    f.write(f"__version__ = '{version}'\n")

setup(
    name="python-plyr",
    version=version,
    description="""Mapping tools for nested containers.""",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/ivannz/plyr",
    license="MIT License",
    author="Ivan Nazarov",
    author_email="ivannnnz@gmail.com",
    packages=[
        "plyr",
    ],
    ext_modules=[
        Extension(
            "plyr.base",
            [
                "src/plyr.cpp",
                "src/validate.cpp",
                "src/operations.cpp",
                "src/apply.cpp",
                "src/tools.cpp",
                "src/ragged.cpp",
                "src/populate.cpp",
            ],
            include_dirs=["src/include"],
            extra_compile_args=["-O3", "-Ofast", "--std=c++11"],
            language="c++",
        ),
    ],
    python_requires=">=3.7",
    install_requires=[],
)
