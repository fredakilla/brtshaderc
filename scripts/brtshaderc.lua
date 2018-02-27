--
-- Copyright 2010-2018 Branimir Karadzic. All rights reserved.
-- License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
--

--group "tools/brtshaderc"

local GLSL_OPTIMIZER = path.join(BGFX_DIR, "3rdparty/glsl-optimizer")
local FCPP_DIR = path.join(BGFX_DIR, "3rdparty/fcpp")

project "brtshaderc"
	kind "StaticLib"

	includedirs {
		path.join(BX_DIR,   "include"),
		path.join(BIMG_DIR, "include"),
		path.join(BGFX_DIR, "include"),

		path.join(BGFX_DIR, "3rdparty/dxsdk/include"),
		FCPP_DIR,

		path.join(BGFX_DIR, "3rdparty/glslang/glslang/Public"),
		path.join(BGFX_DIR, "3rdparty/glslang/glslang/Include"),
		path.join(BGFX_DIR, "3rdparty/glslang"),

		path.join(GLSL_OPTIMIZER, "include"),
		path.join(GLSL_OPTIMIZER, "src/glsl"),
	}

	configuration { "not vs*" }
		buildoptions {
--			"-Wno-deprecated-register",
			"-Wno-ignored-qualifiers",
--			"-Wno-inconsistent-missing-override",
			"-Wno-missing-field-initializers",
			--"-Wno-reorder",
			"-Wno-return-type",
			"-Wno-shadow",
			"-Wno-sign-compare",
			"-Wno-undef",
			"-Wno-unknown-pragmas",
			"-Wno-unused-function",
			"-Wno-unused-parameter",
			"-Wno-unused-variable",
		}


	--links {
	--	"bx",
	--	"fcpp",
	--	"glslang",
	--	"glsl-optimizer",
	--}

	files {

		path.join(BGFX_DIR, "tools/brtshaderc/brtshaderc.cpp"),
		--path.join(BGFX_DIR, "tools/shaderc/shaderclib.cpp"),
		--"brtshaderc.cpp",
		--path.join(BGFX_DIR, "tools/shaderc/**.cpp"),
		--path.join(BGFX_DIR, "tools/shaderc/**.h"),
		path.join(BGFX_DIR, "src/vertexdecl.**"),
		--path.join(BGFX_DIR, "src/shader_spirv.**"),
	}


	--------------------------------------------------------------
	-- bx dependencies
	--------------------------------------------------------------
	files {
		--path.join(BX_DIR, "src/commandline.cpp"),
	}	

	--------------------------------------------------------------
    -- fcpp dependencies
    --------------------------------------------------------------
	defines { -- fcpp
		"NINCLUDE=64",
		"NWORK=65536",
		"NBUFF=65536",
		"OLD_PREPROCESSOR=0",
	}

	files {
		path.join(FCPP_DIR, "**.h"),
		path.join(FCPP_DIR, "cpp1.c"),
		path.join(FCPP_DIR, "cpp2.c"),
		path.join(FCPP_DIR, "cpp3.c"),
		path.join(FCPP_DIR, "cpp4.c"),
		path.join(FCPP_DIR, "cpp5.c"),
		path.join(FCPP_DIR, "cpp6.c"),
		path.join(FCPP_DIR, "cpp6.c"),
	}

	--------------------------------------------------------------
	-- glslang dependencies
	--------------------------------------------------------------
	files {
		path.join(GLSLANG, "glslang/**.cpp"),
		path.join(GLSLANG, "glslang/**.h"),

		path.join(GLSLANG, "hlsl/**.cpp"),
		path.join(GLSLANG, "hlsl/**.h"),

		path.join(GLSLANG, "SPIRV/**.cpp"),
		path.join(GLSLANG, "SPIRV/**.h"),

		path.join(GLSLANG, "OGLCompilersDLL/**.cpp"),
		path.join(GLSLANG, "OGLCompilersDLL/**.h"),
	}

	removefiles {
		path.join(GLSLANG, "glslang/OSDependent/Unix/main.cpp"),
		path.join(GLSLANG, "glslang/OSDependent/Windows/main.cpp"),
	}

	configuration { "windows" }
		removefiles {
			path.join(GLSLANG, "glslang/OSDependent/Unix/**.cpp"),
			path.join(GLSLANG, "glslang/OSDependent/Unix/**.h"),
		}

	configuration { "not windows" }
		removefiles {
			path.join(GLSLANG, "glslang/OSDependent/Windows/**.cpp"),
			path.join(GLSLANG, "glslang/OSDependent/Windows/**.h"),
		}

	--------------------------------------------------------------
	-- glsl-optimizer dependencies
	--------------------------------------------------------------
	includedirs {
		path.join(GLSL_OPTIMIZER, "src"),
		path.join(GLSL_OPTIMIZER, "include"),
		path.join(GLSL_OPTIMIZER, "src/mesa"),
		path.join(GLSL_OPTIMIZER, "src/mapi"),
		path.join(GLSL_OPTIMIZER, "src/glsl"),
	}	

	files {
		path.join(GLSL_OPTIMIZER, "src/mesa/**.c"),
		path.join(GLSL_OPTIMIZER, "src/glsl/**.cpp"),
		path.join(GLSL_OPTIMIZER, "src/mesa/**.h"),
		path.join(GLSL_OPTIMIZER, "src/glsl/**.c"),
		path.join(GLSL_OPTIMIZER, "src/glsl/**.cpp"),
		path.join(GLSL_OPTIMIZER, "src/glsl/**.h"),
		path.join(GLSL_OPTIMIZER, "src/util/**.c"),
		path.join(GLSL_OPTIMIZER, "src/util/**.h"),
	}

	removefiles {
		path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp.c"),
		path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/tests/**"),
		path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.l"),
		path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.y"),
		path.join(GLSL_OPTIMIZER, "src/glsl/ir_set_program_inouts.cpp"),
		path.join(GLSL_OPTIMIZER, "src/glsl/main.cpp"),
		path.join(GLSL_OPTIMIZER, "src/glsl/builtin_stubs.cpp"),
	}



	configuration { "mingw-*" }
		targetextension ".exe"

	configuration { "osx" }
		links {
			"Cocoa.framework",
		}

	configuration { "vs*" }
		includedirs {
			path.join(GLSL_OPTIMIZER, "include/c99"),
		}

	configuration { "vs20* or mingw*" }
		links {
			"psapi",
		}

	configuration {}

	if filesexist(BGFX_DIR, path.join(BGFX_DIR, "../bgfx-ext"), {
		path.join(BGFX_DIR, "scripts/shaderc.lua"), }) then

		if filesexist(BGFX_DIR, path.join(BGFX_DIR, "../bgfx-ext"), {
			path.join(BGFX_DIR, "tools/shaderc/shaderc_pssl.cpp"), }) then

			removefiles {
				path.join(BGFX_DIR, "tools/shaderc/shaderc_pssl.cpp"),
			}
		end

		dofile(path.join(BGFX_DIR, "../bgfx-ext/scripts/shaderc.lua") )
	end

	configuration { "osx or linux*" }
		links {
			"pthread",
		}

	configuration {}

	strip()

group "tools"
