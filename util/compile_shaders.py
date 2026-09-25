import argparse
import subprocess
import os.path
import shutil

from typing import List
from pathlib import Path

def to_spirv_cross_stage_name(stage_name : str) -> str:
    if stage_name == "vertex":
        return "vert"
    elif stage_name == "fragment":
        return "frag"
    elif stage_name == "compute":
        return "comp"

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

    dxc_available : bool = shutil.which("dxc") is not None
    spirv_cross_available : bool = shutil.which("spirv-cross") is not None
    shader_cross_available : bool = shutil.which("shadercross") is not None

    for shader in shaders:
        path = Path(shader).stem

        spv_out : str = directory + "/" + path + ".spv"
        dxil_out : str = directory + "/" + path + ".dxil"
        msl_out : str = directory + "/" + path + ".msl"

        command_spv : str
        command_dxil : str
        command_msl : str

        if shader_cross_available:
            command_spv = ["shadercross", shader, "-s", "HLSL", "-d", "SPIRV", "-e", "main", "-t", shader_stage, "-o", spv_out]
            command_dxil = ["shadercross", shader, "-s", "HLSL", "-d", "DXIL", "-e", "main", "-t", shader_stage, "-o", dxil_out]
            command_msl = ["shadercross", shader, "-s", "HLSL", "-d", "MSL", "-e", "main", "-t", shader_stage, "-o", msl_out]
        elif dxc_available:
            command_spv = ["dxc", "-spirv", "-T", target, "-E", "main", shader, "-Fo", spv_out]
            command_dxil = ["dxc", "-T", target, "-E", "main", shader, "-Fo", dxil_out]
            if spirv_cross_available:
                command_msl = ["spirv-cross", "--msl", spv_out, "--stage", to_spirv_cross_stage_name(shader_stage), "--output", msl_out]

        print(command_spv)
        print(command_dxil)
        print(command_msl)

        subprocess.run(command_dxil, check=True)
        subprocess.run(command_spv, check=True)

        if shader_cross_available or (dxc_available and spirv_cross_available):
            subprocess.run(command_msl, check=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory')
    parser.add_argument('-vertex', action="append")
    parser.add_argument('-fragment', action="append")

    args = parser.parse_args()

    if not args.directory:
        print("Please provide the output directory for shaders")
        return

    directory : str = args.directory

    if args.vertex:
        compile_shaders(args.vertex, "vertex", directory)
    if args.fragment:
        compile_shaders(args.fragment, "fragment", directory)

    if not (args.vertex or args.fragment):
        print("Please provide shaders to compile")

if __name__ == "__main__":
    main()
