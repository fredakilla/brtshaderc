namespace shaderc
{
    enum ShaderType
    {
        ST_VERTEX      = 'v',   /// vertex
        ST_FRAGMENT    = 'f',   /// fragment
        ST_COMPUTE     = 'c',   /// compute
    };

    /**
     * Compile a shader from source file and return memory pointer that contains the compiled shader.
     *
     * @param type : Shader type to comile (vertex, fragment or compute)
     * @param filePath : Shader source file path.
     * @param defines : List of defines semicolon separated ex: "foo=1;bar;baz=1".
     * @param varyingPath : File path for varying.def.sc, or assume default name is "varying.def.sc" in current dir.
     * @return
     */
    const bgfx::Memory* compileShader(
            ShaderType type
          , const char* filePath
          , const char* defines = nullptr
          , const char* varyingPath = nullptr
          );
}
