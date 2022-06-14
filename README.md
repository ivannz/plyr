# Plyr: computing on nested containers 

`plyr` \[/plaɪ'ə/\], derived from `applier`, is a python C-extension, that implements a `map`-like logic, which computes a specified function on the lower-most non-container data of arbitrarily nested *built-in* python containers, i.e. dicts, lists, tuples. It automatically unpacks nested containers in order to call the same function on their underlying non-container objects and then reassembles the structures. See the docstring of `plyr.apply` for details.

`plyr` happens to coincide with a similarly named library for [`R` statistical computations language](https://www.r-project.org/), which streamlines dataframe and vector/matrix transformations.

## Setup

The package can be installed from `pip`:

```bash
pip install python-plyr
```

The development environment could be initialized with the following commands:

```bash
# create a dedicated env
conda create -n plyr "python>=3.7" pip pytest twine \
  && conda activate plyr \
  && pip install build

# build and install
pip install -e . -vv
```

## the Essential Example

Below we provide an example, which, we hope, illustrates the cases `plyr` might be useful in.

```python
import plyr
from collections import namedtuple

nt = namedtuple("nt", "u,v")

# add the leaf data in a pair of nested objects
plyr.apply(
    lambda u, v: u + v,
    [{"a": (1, 2, 3), "z": 3.1415}, nt([1, "u"], "abc")],
    [{"a": (4, 6, 8), "z": 2.7128}, nt([4, "v"], "xyz")],
    # _star=True,  # (default) call fn(d1, d2, **kwargs)
)
# output: [{'a': (5, 8, 11), 'z': 5.8543}, nt(u=[5, 'uv'], v='abcxyz')]

# join strings in a pair of tuples
plyr.apply(
    " -->> ".join,
    ("abc", "uvw", "xyz"),
    ("123", "456", "789"),
    _star=False,  # call fn((d1, d2,), **kwargs)
)
# output: ('abc -->> 123', 'uvw -->> 456', 'xyz -->> 789')
```

By default `.apply` performs safety checks to ensure identical structure if multiple nested objects are given. If the arguments have identical structure by design, then these integrity checks may be turned off by specifying `_safe=False`. Please refer to the docs of `plyr.apply`.

`plyr.ragged` is a special version of `.apply` which implements leaf broadcasting semantics. When processing multiple nested objects it allows one structure to subsume the other structures: any intermediate leaf data **is broadcasted deeper into the hierarchy** of the other nested structures. Please refer to `./doc/mapping_structures.ipynb` for details.


## Serializing and deserializing

Serialization and deserialization of nested objects can be done by `.flatten` and `.unflatten`. The following snippet shows how to represent a given nested object as a flat list and then undo the process.

```python
import plyr

o = [1, (2, 3), {"a": (4, 5), "z": {"a": 6}}, 7]

flat, skel = plyr.flatten(o)
assert o == plyr.unflatten(flat, skel)

flat
# output: ([1, 2, 3, 4, 5, 6, 7]
```

This next example demonstrates how to unpack a stream of data into nested objects.

```python
import plyr

stream, _ = iter(range(13)), None

struct = ({"foo": _, "bar": [_, _]}, _)

objects = [plyr.unflatten(stream, struct) for _ in range(3)]
```

The following example illustrates how to compute the a functon over a nested object and *invert* the structure (see `.iapply`):

```python
import plyr


def func(x, y):
    return {"z": x * y}, -y


# `inverted` shall nests the original structure of `o` _under_ func's
#  return-value structure
flat, struct = plyr.flatapply(func, (2, {"a": 1},), (4, {"a": 3},))
inverted = plyr.apply(plyr.unflatten, *flat, _star=False, struct=struct)

inverted
# output: ({'z': (8, {'a': 3})}, (-4, {'a': -3}))
```

Here's how one might broadcast a value across the structure of a specified nested object.

```python
import plyr

struct = {"foo": {"bar": 1}, "baz": (2, 3)}

const = "abc"
broadcast = plyr.apply(lambda x: const, struct)

broadcast
# output: {'foo': {'bar': 'abc'}, 'baz': ('abc', 'abc')}
```

## Other Examples

Below we perform something fancy with `numpy`. Specifically, we stack outputs from some experiments (dicts of arrays), to get the standard deviation between the results.

```python
import plyr
import numpy as np


# some computations
def experiment(j):
    return dict(
        a=float(np.random.normal()),
        u=np.random.normal(size=(5, 2)) * 0.1,
        z=np.random.normal(size=(2, 5)) * 10,
    )


# run 10 replications of an experiment
results = [experiment(j) for j in range(10)]

# stack and analyze the results (np.stack needs an iterable argument)
res = plyr.apply(np.stack, *results, axis=0, _star=False)

# get the shapes
shapes = plyr.apply(lambda x: x.shape, res)

# compute the std along the replication axis
plyr.apply(np.std, res, axis=0)
```

You may notice that `.apply` is very _unsophisticated_: it applies the specified function to the leaf data regardless of its type, and every dict, list, or tuple is _always_ treated as a nested container.
