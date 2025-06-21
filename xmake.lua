add_rules("mode.debug", "mode.release")

add_repositories("groupmountain-repo https://github.com/GroupMountain/xmake-repo.git")

add_requires(
    "endstone 0.9.1",
    "openssl",
    "asio 1.32.0",
    "websocketpp 0.8.2",
    "nlohmann_json 3.12.0", {configs = {cmake = false}}
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
        
        -- 获取当前构建模式
        local mode = get_config("mode")
        
        -- 处理Linux平台文件名
        if os.host() == "linux" then
            filename = filename:sub(4)
        end
        
        -- 添加构建模式后缀
        local ext = ""
        local name = filename
        local dotpos = filename:find("%.")
        if dotpos then
            name = filename:sub(1, dotpos-1)
            ext = filename:sub(dotpos)
        end
        
        -- 添加构建模式后缀
        if mode == "release" then
            filename = name .. "_release" .. ext
        else
            filename = name .. "_debug" .. ext
        end
        
        os.cp(file, path.join(output_dir, filename))
        cprint("${bright green}[Plugin]: ${reset}plugin already generated to " .. output_dir)
        
        -- 只在Debug模式下复制PDB文件
        if mode == "debug" and is_plat("windows") then
            local pdb_file = path.join(path.directory(file), path.basename(file) .. ".pdb")
            if os.isfile(pdb_file) then
                local pdb_filename = name .. "_debug.pdb"
                os.cp(pdb_file, path.join(output_dir, pdb_filename))
                cprint("${bright green}[PDB]: ${reset}debug symbols copied to " .. output_dir)
            end
        end
    end)