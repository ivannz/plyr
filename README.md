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


## Example

Below we provide an example, which, we hope, illustrates the cases `plyr` might be useful in.

```python
import plyr
from collections import namedtuple

nt = namedtuple('nt', 'u,v')

# add the leaf data in a pair of nested objects
plyr.apply(
    lambda u, v: u + v,
    [{'a': (1, 2, 3), 'z': 3.1415}, nt(1, 'abc')],
    [{'a': (4, 6, 8), 'z': 2.7128}, nt(4, 'xyz')],
    # _star=True,  # (default) call fn(d1, d2, **kwargs)
)
# output: [{'a': [5, 8, 11]}, {'b': 5, 'z': 'abcxyz'}]

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


## Other examples

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

You may notice that `.apply` is very _straightforward_: it applies the specified function
regardless of the leaf data type.
