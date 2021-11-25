# Plyr: nested `map`

No more will you need to manually pack/unpack nested structures (dicts, lists, tuples etc.) in order to call the same function on its underlying objects. In order to support such basic arbitrarily nested objects, I wrote a c-api implementation of a `map`-like functionality for nested containers with arbitrary inner data. See the dosctring of `plyr.apply` for more details.

```python
from plyr import apply

# sum the leaf data in two nested objects
apply(
    lambda u, v: u + v,
    [{'a': [1, 2, 3]}, {'b': 1, 'z': 'abc'}, ],
    [{'a': [4, 6, 8]}, {'b': 4, 'z': 'xyz'}, ],
)
# outputs [{'a': [5, 8, 11]}, {'b': 5, 'z': 'abcxyz'}]
```

`plyr` \[/plaɪ'ə/\] is derived from `applier`, but also happens to coincide with a similarly named library for [`R` statistical computations language](https://www.r-project.org/), which streamlines dataframe and vector/matrix transformations.
