import argparse
import subprocess
import os.path

from typing import List
from pathlib import Path

def compile_shaders(shaders : List[str], shader_stage : str, directory : str):
    stage_argument = "" 
    if shader_stage == "vertex":
        stage_argument = "vs"
    elif shader_stage == "fragment":
        stage_argument = "ps"
    else:
        print(f"Invalid shader stage argument {stage_argument}")
        return # invalid

    shader_model = "_6_0"
    target = stage_argument + shader_model

    for s in shaders:
        path = Path(s).stem

        subprocess.run(["dxc", "-T", target, "-E", "main", s, "-Fo", directory + "binary/" + path + ".dxil"], check=True)
        subprocess.run(["dxc", "-spirv", "-T", target, "-E", "main", s, "-Fo", directory + "binary/" + path + ".spv"], check=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-vertex', action="append")
    parser.add_argument('-fragment', action="append")

    args = parser.parse_args()

    directory : str = os.path.dirname(os.path.normpath(__file__)) + "/"

    if args.vertex:
        compile_shaders(args.vertex, "vertex", directory)
    if args.fragment:
        compile_shaders(args.fragment, "fragment", directory)

    if not (args.vertex or args.fragment):
        print("Please provide shaders to compile")

if __name__ == "__main__":
    main()
