# Build wheels for currently supported python versions
# XXX see https://endoflife.date/python
declare -a envnfo=( \
    "plyr-cp37" "python=3.7" \
    "plyr-cp38" "python=3.8" \
    "plyr-cp39" "python=3.9" \
    "plyr-cp310" "python=3.10" \
)

# https://github.com/conda/conda/issues/7980
eval "$(conda shell.bash hook)"
for ((i=0; i<${#envnfo[@]}; i+=2)); do
    # stup env with the correct python version
    conda create -n ${envnfo[i+0]} -y "${envnfo[i+1]}" pip twine
    conda activate ${envnfo[i+0]}

    # build the package
    pip install build
    python -m build

    # cleanup
    conda deactivate
    conda env remove -n ${envnfo[i+0]}
done
