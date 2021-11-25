# Plyr: nested `map`

This python c-extension implementats a `map`-like functionality for arbitrarily nested *built-in* python containers, i.e. dicts, lists, tuples etc. No need to manually pack/repack nested structures in order to call the same function on its underlying non-container objects. See the dosctring of `plyr.apply` for more details.

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
RE