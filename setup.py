from setuptools import setup, Extension

setup(
    name='plyr',
    version='0.5',
    author='Ivan Nazarov',
    license='MIT',
    description="""Mapping tools for nested containers.""",
    long_description=open('README.md').read(),
    long_description_content_type="text/markdown",
    packages=[],
    install_requires=[],
    ext_modules=[
        Extension(
            'plyr', [
                'src/plyr.cpp',
                'src/validate.cpp',
                'src/operations.cpp',
                'src/apply.cpp',
            ], include_dirs=[
                'src/include'
            ], extra_compile_args=[
                '-O3', '-Ofast'
            ], language='c++',
        ),
    ],
)
