souce /venv/bin/activate


# https://github.com/zeromq/pyzmq/blob/f30a5cb00cfd67ab65b474c86852a31e77c349af/.github/workflows/wheels.yml
CIBW_REPAIR_WHEEL_COMMAND='' cibuildwheel --platform=linux

#Error: Command ['sh', '-c', 'auditwheel repair -w /tmp/cibuildwheel/repaired_wheel /tmp/cibuildwheel/built_wheel/cassarrow-0.0.1rc0-cp39-cp39-linux_x86_64.whl'] failed with code 1.
# -> Use CIBW_REPAIR_WHEEL_COMMAND=''

# On cp39-manylinux_i68
# Error: Command ['python', '-m', 'pip', 'wheel', PurePosixPath('/project'), '--wheel-dir=/tmp/cibuildwheel/built_wheel', '--no-deps'] failed with code 1.
# ERROR: Could not build wheels for pyarrow, which is required to install pyproject.toml-based projects
# Could NOT find Arrow (missing: ARROW_INCLUDE_DIR ARROW_LIB_DIR