set_project("GoldSrc Renderer")

-- third party packages
local thirdPartyPkgs = {
    "vulkansdk",
    "glm",
    "glfw",
    "vcpkg::imgui[core,vulkan-binding,glfw-binding]", -- xmake seems not have glfw/vulkan binding for now, so use vcpkg source
    "stb",
    "tinyexr",
    "nativefiledialog",
}
for i, pkgName in pairs(thirdPartyPkgs) do
    add_requires(pkgName)
    add_packages(pkgName) -- add for all
end

set_languages("cxx17")

-- projects
local projects = {
    "Common",
    "Gui",
    "IO",
    "RenderPass",
    "Scene",
    "Visualize",
    "Vulkan",
    "Engine",
}

for i, projectName in pairs(projects) do
    add_includedirs(string.format("Src/%s", projectName)) -- add include dir for all
end

function addProject(kind, name, localDeps)
    target(name) 
        set_kind(kind)
        add_files(string.format("Src/%s/*.cpp", name))
        if kind == "binary" then 
            for i, projectName in pairs(projects) do
                add_deps(projectName)
            end
        end
    target_end()
end

addProject("static", "Common")
addProject("static", "IO")
addProject("static", "Vulkan")
addProject("static", "Gui")
addProject("static", "Engine")
addProject("static", "Visualize")
addProject("static", "Scene")
addProject("static", "RenderPass")
addProject("binary", "GoldSrc-Renderer")

--TODO: build tests and experiments
