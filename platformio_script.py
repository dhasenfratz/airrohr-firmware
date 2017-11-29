
Import("env")
import os
import shutil
import subprocess

def after_build(source, target, env):
    version = subprocess.check_output(["git", "describe"]).strip()
    shutil.copy(target[0].path, "./builds/airrohr-fw-" + version + ".bin")


env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
