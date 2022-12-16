import sys

if sys.platform == "win32":
    # See https://stackoverflow.com/a/62723124/109525
    import os

    import pyarrow as pa

    for directory in pa.get_library_dirs():
        os.add_dll_directory(directory)


from cassarrow.impl import install_cassarrow, metadata_to_schema, result_set_to_table

__version__ = "0.2.0"

__all__ = ["install_cassarrow", "result_set_to_table", "metadata_to_schema"]
