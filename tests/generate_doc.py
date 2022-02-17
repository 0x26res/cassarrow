import pyarrow.csv as pa

if __name__ == "__main__":
    print(
        pa.read_csv("types.csv")
        .to_pandas()
        .assign(pyarrow=lambda x: ("`" + x["pyarrow"] + "`").where(x["pyarrow"] != "", ""))
        .to_markdown(index=False)
    )

    print(
        pa.read_csv("collections.csv")
        .to_pandas()
        .assign(pyarrow=lambda x: ("`" + x["pyarrow"] + "`").where(x["pyarrow"] != "", ""))
        .to_markdown(index=False)
    )
