if _OPTIONS["with-examples"]
or _OPTIONS["with-combined-examples"] then
	group "examples"

    exampleProject(_OPTIONS["with-combined-examples"], "01-cubes-brtshaderc")

    includedirs {
		path.join(BGFX_DIR,   "tools/brtshaderc")
    }

    links {
		"brtshaderc"
    }
    
	files {
		path.join(BGFX_DIR, "examples/01-cubes-brtshaderc/cubes.cpp")
	}
end

