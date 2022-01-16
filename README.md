# Plyr: computing on nested containers 

`plyr` \[/plaɪ'ə/\], derived from `applier`, is a python C-extension, that implements
a `map`-like logic, which computes a specified function on the lower-most non-container
data of arbitrarily nested *built-in* python containers, i.e. dicts, lists, tuples.
It automatically unpacks nested containers in order to call the same function on their
underlying non-container objects and then reassembles the structures. See the docstring
of `plyr.apply` for details.

`plyr` happens to coincide with a similarly named library for [`R` statistical computations
language](https://www.r-project.org/), which streamlines dataframe and vector/matrix
transformations.


## the Essential Example

Below we provide an example, which, we hope, illustrates the cases `plyr` might be useful in.

```python
import plyr
from collections import namedtuple

nt = namedtuple('nt', 'u,v')

# add the leaf data in a pair of nested objects
plyr.apply(
    lambda u, v: u + v,
    [{'a': (1, 2, 3), 'z': 3.1415}, nt([1, 'u'], 'abc')],
    [{'a': (4, 6, 8), 'z': 2.7128}, nt([4, 'v'], 'xyz')],
    # _star=True,  # (default) call fn(d1, d2, **kwargs)
)
# output: [{'a': (5, 8, 11), 'z': 5.8543}, nt(u=[5, 'uv'], v='abcxyz')]

# join strings in a pair of tuples
plyr.apply(
    ' -->> '.join,
    ('abc', 'uvw', 'xyz',),
    ('123', '456', '789',),
    _star=False,  # call fn((d1, d2,), **kwargs)
)
# output: ('abc -->> 123', 'uvw -->> 456', 'xyz -->> 789')
```

By default `.apply` performs safety checks to ensure identical structure if multiple
nested objects are given. If the arguments have identical structure by design, then
these integrity checks may be turned off by specifying `_safe=False`. Please refer
to the docs of `plyr.apply`.

`plyr.ragged` is a special version of `.apply` which implements leaf broadcasting
semantics. When processing multiple nested objects it allows one structure to subsume
the other structures: any intermediate leaf data **is broadcasted deeper into the hierarchy**
of the other nested structures. Please refer to `./doc/mapping_structures.ipynb` for
details.


## Serializing and deserializing

Serialization and deserialization of nested objects can be done by these procedures:

```python
import plyr


def flatten(struct, *, flat=None):
    """Get a flat depth-first representation of the nested object.

    Parameters
    ----------
    struct : nested object
        The nested object to serialize.

    flat : list, or None
        A flat iterable container to serially append the leaf data to.

    Returns
    -------
    flat : list
        The container populated with the leaves in depth-first structure order.

    struct : nested object
        The skeletal structure of the nested object with arbitrary leaf data.
    """
    if not isinstance(flat, list):
        flat = []

    return flat, plyr.apply(flat.append, struct)


def unflatten(flat, struct, *, raises=True):
    """Place the data from iterable into the specified nested structure.

    Parameters
    ----------
    flat : iterable
        The iterable that supplies the data for the nested object in fifo
        depth-first order.

    struct : nested object
        The desired structure of the nested object (leaf data is intact).

    Returns
    -------
    result : nested object
        The nested object with the specified hierarchy populated by the data
        from the iterable.

    Details
    -------
    Raises `StopItertaion` if the iterable is exhausted before the structure is
    completely rebuilt. Data consumed by an unsuccessful `unflatten` is LOST.
    """
    # use `next(it, None)` to fill missing leaves with `None`-s.
    if not raises:
        return plyr.apply(lambda *a, it=iter(flat): next(it, None), struct)

    return plyr.apply(lambda *a, it=iter(flat): next(it), struct)
    # return plyr.ragged(lambda it, *a: next(it), iter(flat), struct)
```

The following snippet shows how to represent a given nested object as a flat list
and then undo the process.

```python
o = [1, (2, 3), {'a': (4, 5), 'z': {'a': 6}}, 7]

flat, skel = flatten(o)
assert o == unflatten(flat, skel)

flat
# output: ([1, 2, 3, 4, 5, 6, 7]
```

This example demonstrates how to unpack a stream of data into nested objects.

```python
stream, _ = iter(range(13)), None

struct = ({'foo': _, 'bar': [_, _]}, _)

objects = [unflatten(stream, struct) for _ in range(3)]
```


## Other Examples

Below we perform something fancy with `numpy`. Specifically, we stack outputs from some
experiments (dicts of arrays), to get the standard deviation between the results.

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

You may notice that `.apply` is very _unsophisticated_: it applies the specified function
to the leaf data regardless of its type, and every dict, list, or tuple is _always_ treated
as a nested container.
