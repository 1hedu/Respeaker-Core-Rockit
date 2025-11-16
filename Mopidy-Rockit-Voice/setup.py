from __future__ import unicode_literals

from setuptools import find_packages, setup


def get_version(filename):
    import re
    content = open(filename).read()
    metadata = dict(re.findall("__([a-z]+)__ = '([^']+)'", content))
    return metadata['version']


setup(
    name='Mopidy-Rockit-Voice',
    version=get_version('mopidy_rockit_voice/__init__.py'),
    url='https://github.com/1hedu/Respeaker-Core-Rockit',
    license='Apache License, Version 2.0',
    author='Based on mopidy-hallo by ReSpeaker',
    description='Voice control for Rockit synth via MIDI',
    long_description=open('README.md').read(),
    packages=find_packages(exclude=['tests', 'tests.*']),
    zip_safe=False,
    include_package_data=True,
    install_requires=[
        'setuptools',
        'Mopidy >= 2.0',
        'Pykka >= 1.1',
    ],
    entry_points={
        'mopidy.ext': [
            'rockit_voice = mopidy_rockit_voice:Extension',
        ],
    },
    classifiers=[
        'Environment :: No Input/Output (Daemon)',
        'Intended Audience :: End Users/Desktop',
        'License :: OSI Approved :: Apache Software License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        'Topic :: Multimedia :: Sound/Audio :: Players',
    ],
)
