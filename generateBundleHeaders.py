import argparse
import os.path

def dump_header(contents : bytes, variable_name, file_name : str, directory : str):
    with open(directory + file_name, "w") as f:
        f.write("unsigned char " + variable_name + "[] = {\n")
        fullLines : int = len(contents) // 8
        for line in range(fullLines):
            f.write("\t")
            for i in range(8):
                f.write(f"0x{contents[line * 8 + i]:02x}, ")
            f.write("\n");
        f.write("\t")
        for i in range(len(contents) % 8):
            f.write(f"0x{contents[fullLines * 8 + i]:02x}, ")
        f.write("\n")
        f.write("};\n")
        f.write("unsigned int " + variable_name + "_len" + f" = {len(contents)};\n")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('variable_name', type=str)
    parser.add_argument('file', type=str)

    args = parser.parse_args()

    # output
    directory : str = os.path.dirname(os.path.normpath(__file__)) + "/src/bundle/"
    var_name : str = args.variable_name

    data : bytes
    with open(args.file, "rb") as f:
        data = f.read()

    dump_header(data, var_name, var_name + ".h", directory)

if __name__ == "__main__":
    main()
