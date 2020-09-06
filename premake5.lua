-- Generation script

workspace "RoadefGoogleChallenge"
    architecture "x64"
    
    configurations
    {
        "Debug",
        "DebugOptOn",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.architecture}-%{cfg.system}"

project "RoadefGoogleChallenge"
    location ""
    kind "ConsoleApp"
    staticruntime "On"

    language "C++"
    cppdialect "C++17"

    floatingpoint "Fast"

    warnings "Extra"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    debugdir ("%{cfg.targetdir}")

    files
    {
        "Source/**.hpp",
        "Source/**.cpp"
    }
    
    includedirs
    {
        "External",
        "Source"
    }

    postbuildcommands
    {
		"{copy} Assets %{cfg.targetdir}/Assets"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "MODE_DEBUG" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:DebugOptOn"
        defines { "MODE_DEBUGOPTON" }
        runtime "Debug"
        symbols "On"
        optimize "On"

    filter "configurations:Release"
        defines { "MODE_RELEASE" }
        runtime "Release"
        symbols "Off"
        optimize "On"