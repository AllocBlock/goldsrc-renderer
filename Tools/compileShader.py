
# this script help compile shader without configing all shader individually in VS
# pass in dir

# if no python env, you can use following setup in VS:
# 1. put shader in to project, right click on it and choose property
# 2. change to custom build
# 3. navigate to "custom build" -> "general", add command below
# 4. Command Line: $(VULKAN_SDK)\Bin\glslangValidator.exe -V %(Identity) -o %(RelativeDir)%(Filename)Frag.spv (replace Frag with Vert, Compute, etc...)
# 5. Output: %(RelativeDir)%(Filename)Frag.spv (replace Frag with Vert, Compute, etc...)

import os
import sys
import time

gVulkanDir = os.getenv('VULKAN_SDK')

if not gVulkanDir:
    raise 'no vulkan found'

gCompilerExe = gVulkanDir + "/Bin/glslangValidator.exe"

def abort(err = None):
    if err:
        print("错误：" + err)
    time.sleep(5)

# x:/project/shaders/test.vert -> x:/project/shaders/testVert.spv
def getOutputFilePath(InputFilePath):
    FilePathP1 = os.path.splitext(InputFilePath)[0] # dir + name prefix
    Ext = os.path.splitext(InputFilePath)[1]
    FilePathP2 = Ext[1].upper() + Ext[2:] + ".spv"
    return FilePathP1 + FilePathP2

def compileShader(InputFilePath):
    if not os.path.exists(InputFilePath):
        abort("文件不存在：" + InputFilePath)

    ShaderOutputFile = getOutputFilePath(InputFilePath)

    print("编译 [" + os.path.split(InputFilePath)[-1] + " -> " + os.path.split(ShaderOutputFile)[-1] + "]")
    cmd = "start /wait /b %s -V %s -o %s" % (gCompilerExe, InputFilePath, ShaderOutputFile)
    os.system(cmd)

ShaderDir = sys.argv[1] if len(sys.argv) >= 2 else None
if not ShaderDir:
    ShaderDir = input("指定文件/文件夹：")
if not os.path.exists(ShaderDir):
    abort("文件/文件夹不存在" + ShaderDir)
if not os.path.isdir(ShaderDir):
    ShaderDir = [ShaderDir]

FileList = os.listdir(ShaderDir)
for FileName in FileList:
    Path = ShaderDir + "/" + FileName
    if os.path.isdir(Path):
        continue
    elif os.path.splitext(Path)[1] == '.spv':
        continue
    compileShader(os.path.normpath(Path))
abort()