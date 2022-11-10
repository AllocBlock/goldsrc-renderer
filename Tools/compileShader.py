
# 2022-11-10
# This script is DEPRECATED as dynamic shader compile is implemented
# But you can still use it if needed
# -----------------------

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
import json

gVulkanDir = os.getenv('VULKAN_SDK')
gSourceShaderExtensions = ['.vert', '.frag']
gCacheFilename = "shaderCompileCache.json"

def getChangeTime(file):
    return time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(os.path.getmtime(file)))

class CompileCache():
    def __init__(self):
        self.compileInfos = dict()

    def load(self, file):
        self.compileInfos = dict()
        if (os.path.exists(file)):
            with open(file, "r") as f:
                infos = json.loads(f.read())
                for entry in infos:
                    self.compileInfos[entry['file']] = entry['changeTime']

    def doesNeedRecompile(self, file):
        file = os.path.abspath(file)
        changeTime = getChangeTime(file)
        if file in self.compileInfos:
            return self.compileInfos[file] != changeTime
        else:
            return True
    
    def add(self, file):
        file = os.path.abspath(file)
        self.compileInfos[file] = getChangeTime(file)

    def save(self, file):
        with open(file, "w") as f:
            data = []
            for file in self.compileInfos:
                data.append({"file": file, "changeTime": self.compileInfos[file]})
            f.write(json.dumps(data, indent=4))


if not gVulkanDir:
    raise 'no vulkan found'

gCompilerExe = gVulkanDir + "/Bin/glslangValidator.exe"

def abort(err = None):
    if err:
        print("错误：" + err)
    time.sleep(5)

# x:/project/shaders/test.vert -> x:/project/shaders/testVert.spv
def get_compiled_shader_path(sourceShaderPath):
    filePathP1 = os.path.splitext(sourceShaderPath)[0] # dir + name prefix
    ext = os.path.splitext(sourceShaderPath)[1]
    filePathP2 = ext[1].upper() + ext[2:] + ".spv"
    return filePathP1 + filePathP2

# def delete_compiled_shaders(dir):
#     fileList = os.listdir(dir)
#     for fileName in fileList:
#         path = dir + "/" + fileName
#         if os.path.isdir(path):
#             continue
#         if os.path.splitext(path)[1] == '.spv':
#             os.remove(path)

def delete_compiled_shader_of_source(sourceShaderFile):
    path = get_compiled_shader_path(sourceShaderFile)
    if os.path.exists(path):
        os.remove(path)

def find_shader_source_in_dir(dir, cache):
    needCompileFiles = []
    notneedCompileFiles = []
    files = os.listdir(shaderDir)
    for file in files:
        path = shaderDir + "/" + file
        if os.path.isdir(path):
            continue
        elif os.path.splitext(path)[1] not in gSourceShaderExtensions:
            continue
        elif cache.doesNeedRecompile(path):
            needCompileFiles.append(path)
        else:
            notneedCompileFiles.append(path)
    return [needCompileFiles, notneedCompileFiles]

def compile_shader(sourceShaderFile):
    if not os.path.exists(sourceShaderFile):
        abort("文件不存在：" + sourceShaderFile)

    compiledFile = get_compiled_shader_path(sourceShaderFile)

    print("编译 [" + os.path.split(sourceShaderFile)[-1] + " -> " + os.path.split(compiledFile)[-1] + "]")
    cmd = "start /wait /b %s -V %s -o %s" % (gCompilerExe, sourceShaderFile, compiledFile)
    os.system(cmd)

param = sys.argv[1] if len(sys.argv) >= 2 else None
if not param:
    param = input("指定文件/文件夹：")
if not os.path.exists(param):
    abort("文件/文件夹不存在" + param)

cache = None
toProcessList = []
alreadyCompiledNum = 0

if os.path.isdir(param):
    shaderDir = param

    cache = CompileCache()
    cacheFilePath = os.path.join(shaderDir, gCacheFilename)
    cache.load(cacheFilePath)

    [need, notNeed] = find_shader_source_in_dir(shaderDir, cache)
    toProcessList.extend(need)
    alreadyCompiledNum += len(notNeed)
else:
    toProcessList.append(param)

for sourceShaderPath in toProcessList:
    compile_shader(sourceShaderPath)
    cache.add(sourceShaderPath)

if cache:
    cache.save(cacheFilePath)

print(f"已编译数量：{len(toProcessList)}\n无需编译数量：{alreadyCompiledNum}")
abort()