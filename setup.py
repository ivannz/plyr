from setuptools import setup, Extension

setup(
    name="python-plyr",
    version="0.8",
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
            ],
            include_dirs=["src/include"],
            extra_compile_args=["-O3", "-Ofast", "--std=c++11"],
            language="c++",
        ),
    ],
    python_requires=">=3.7",
    install_requires=[],
)
