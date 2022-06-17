"""Streamlined operations on built-in nested containers."""
from .__version__ import __version__

from .base import (
    apply,
    flatapply,
    validate,
    ragged,
    suply,
    tuply,
    s_ply,
    t_ply,
    getitem,
    setitem,
    xgetitem,
    xsetitem,
    identity,
    populate,
    AtomicTuple,
    AtomicList,
    AtomicDict,
)


def flatten(struct):
    """Get a flat depth-first representation of the nested object.

    Parameters
    ----------
    struct : nested object
        The nested object to serialize.

    Returns
    -------
    flat : list
        The container populated with the leaves in depth-first structure order.

    struct : nested object
        The skeletal structure of the nested object with arbitrary leaf data.
    """
    return flatapply(identity, struct)


def unflatten(flat, struct, *, raises=True):
    """Place the data from iterable into the specified nested structure.

    Parameters
    ----------
    flat : iterable
        The iterable that supplies the data for the nested object in fifo
        depth-first order.

    struct : nested object
        The desired structure of the nested object (leaf data is ignored).

    Returns
    -------
    result : nested object
        The nested object with the specified structure populated by the data
        from the iterable.

    Details
    -------
    Raises `StopItertaion` if the iterable is exhausted before the structure is
    completely rebuilt. Data consumed by an unsuccessful `unflatten` is LOST.
    """
    # use `next(it, None)` to fill missing leaves with `None`-s.
    if not raises:
        return populate(struct, iter(flat), default=None)

    return populate(struct, iter(flat))


def iapply(f, *objects, _star=True, **kwargs):
    """Compute the function on the nested objects' leaves and return
    the results in `inverted` structure: positional's structure is nested
    at the bottom of the structure returned by the function.

    Parameters
    ----------
    callable : callable
        A callable to be applied to the leaf data.

    *objects : nested objects
        All remaining positional arguments are assumed to be nested objects,
        leaves of which supply arguments for the callable.

    _star : bool, default=True
        Determines whether to pass the leaf data to the callable as
        positionals or as a tuple. See `.apply`.

    **kwargs : variable keyword arguments
       Optional keyword arguments passed AS IS to the `callable`.

    Returns
    -------
    result : a new nested object
        The nested object that contains the values returned by `callable`.
        Guaranteed to have IDENTICAL structure as the first nested object
        in objects.
    """

    flat, struct = flatapply(f, *objects, _star=_star, **kwargs)
    if not flat:
        return None

    return apply(unflatten, *flat, _star=False, struct=struct)
