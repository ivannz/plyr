from setuptools import setup, Extension

setup(
    name='plyr',
    description="""Mapping tools for nested containers.""",
    version='0.5',
    author='Ivan Nazarov',
    license='MIT',
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
                'include'
            ], extra_compile_args=[
                '-O3', '-Ofast'
            ], language='c++',
        ),
    ],
)
