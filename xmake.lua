add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_repositories("groupmountain-repo https://github.com/GroupMountain/xmake-repo.git")

add_requires(
    "endstone 0.9.1",
    "openssl",
    "asio 1.32.0",
    "websocketpp 0.8.2",
    "nlohmann_json 3.12.0"
)

if is_plat("windows") and not has_config("vs_runtime") then
    set_runtimes("MD")
end

if is_plat("linux") then
    set_toolchains("clang")
end

target("huhobot")
    set_kind("shared")
    set_languages("c++23")
    add_packages(
        "endstone",
        "asio",
        "websocketpp",
        "nlohmann_json",
        "openssl"
    )
    add_includedirs("src")
    add_files("src/**.cpp")

    if is_plat("windows") then
        add_defines("NOMINMAX")
        add_cxflags(
            "/EHsc", 
            "/utf-8", 
            "/W4"
        )
        add_syslinks("crypt32", "ws2_32")
    else
        add_cxxflags("-Wno-gnu-line-marker")
        add_cxflags(
            "-Wall",
            "-pedantic",
            "-fexceptions",
            "-stdlib=libc++"
        )
        add_ldflags(
            "-stdlib=libc++"
        )
    end

    after_build(function(target)
        local file = target:targetfile()
        local output_dir = path.join(os.projectdir(), "bin")
        os.mkdir(output_dir)
        local filename = path.filename(file)
        if os.host() == "linux" then
            filename = filename:sub(4)
        end
        os.cp(file, path.join(output_dir, filename))
        cprint("${bright green}[Plugin]: ${reset}plugin already generated to " .. output_dir)
    end)