#include <cstdio>
#include <bx/file.h>
#include <vector>

// hack to fix the multiple definition link errors
#define getUniformTypeName getUniformTypeName_shaderc
#define nameToUniformTypeEnum nameToUniformTypeEnum_shaderc
#define s_uniformTypeName s_uniformTypeName_shaderc

namespace bgfx
{
    typedef void(*UserErrorFn)(void*, const char*, va_list);
    static UserErrorFn s_user_error_fn = nullptr;
    static void* s_user_error_ptr = nullptr;
    void setShaderCErrorFunction(UserErrorFn fn, void* user_ptr)
    {
        s_user_error_fn = fn;
        s_user_error_ptr = user_ptr;
    }
}

void printError(FILE* file, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (bgfx::s_user_error_fn)
    {
        bgfx::s_user_error_fn(bgfx::s_user_error_ptr, format, args);
    }
    else
    {
        vfprintf(file, format, args);
    }
    va_end(args);
}

// hack to defined stuff
#define fprintf printError
#define main fakeMain
#define g_allocator g_shaderc_allocator

// fix warnings
#undef BX_TRACE
#undef BX_WARN
#undef BX_CHECK

// include original shaderc code files
#include "../../bgfx/tools/shaderc/shaderc.cpp"
#include "../../bgfx/tools/shaderc/shaderc_hlsl.cpp"
#include "../../bgfx/tools/shaderc/shaderc_glsl.cpp"
//#define static_allocate static_allocate_shaderc
//#define static_deallocate static_deallocate_shaderc
//#include "../../bgfx/tools/shaderc/shaderc_spirv.cpp"
//#include "../../bgfx/tools/shaderc/shaderc_pssl.cpp"

namespace bgfx 
{
    bool compilePSSLShader(const Options&, uint32_t, const std::string&, bx::WriterI*)
    {
        return false;
    }
    bool compileSPIRVShader(const Options&, uint32_t, const std::string&, bx::WriterI*)
    {
        return false;
    }
}



#include "brtshaderc.h"

namespace shaderc
{
    /// not a real FileWriter, but a hack to redirect write() to a memory block.
    class BufferWriter : public bx::FileWriter
    {
    public:

        BufferWriter()
        {
        }

        ~BufferWriter()
        {
        }

        const bgfx::Memory* finalize()
        {
            if(_buffer.size() > 0)
            {
                _buffer.push_back('\0');

                const bgfx::Memory* mem = bgfx::alloc(_buffer.size());
                bx::memCopy(mem->data, _buffer.data(), _buffer.size());
                return mem;
            }

            return nullptr;
        }

        int32_t write(const void* _data, int32_t _size, bx::Error* _err)
        {
            const char* data = (const char*)_data;
            _buffer.insert(_buffer.end(), data, data+_size);
            return _size;
        }

    private:
        BX_ALIGN_DECL(16, uint8_t) m_internal[64];
        typedef std::vector<uint8_t> Buffer;
        Buffer _buffer;
    };



    const bgfx::Memory* compileShader(ShaderType type, const char* filePath, const char* defines, const char* varyingPath, const char* profile)
    {
        bgfx::Options options;
        bx::memSet(&options, 0, sizeof(bgfx::Options));

        options.inputFilePath = filePath;
        options.shaderType = type;

        // set platform
#if BX_PLATFORM_LINUX
        options.platform = "linux";
#elif BX_PLATFORM_WINDOWS
        options.platform = "windows";
#elif BX_PLATFORM_ANDROID
        options.platform = "android";
#elif BX_PLATFORM_EMSCRIPTEN
        options.platform = "asm.js";
#elif BX_PLATFORM_IOS
        options.platform = "ios";
#elif BX_PLATFORM_OSX
        options.platform = "osx";
#endif

        // set profile
        if(profile)
        {
            // user profile
            options.profile = profile;
        }
        else
        {
            // set default profile for current running renderer.

            bgfx::RendererType::Enum renderType = bgfx::getRendererType();

            switch(renderType)
            {
            case bgfx::RendererType::Noop:         //!< No rendering.
                break;
            case bgfx::RendererType::Direct3D9:    //!< Direct3D 9.0
            {
                if(type == 'v')
                    options.profile = "vs_3_0";
                else if(type == 'f')
                    options.profile = "ps_3_0";
                else if(type == 'c')
                    options.profile = "ps_3_0";
            }
            break;
            case bgfx::RendererType::Direct3D11:   //!< Direct3D 11.0
            {
                if(type == 'v')
                    options.profile = "vs_4_0";
                else if(type == 'f')
                    options.profile = "ps_4_0";
                else if(type == 'c')
                    options.profile = "cs_5_0";
            }
            break;
            case bgfx::RendererType::Direct3D12:   //!< Direct3D 12.0
            {
                if(type == 'v')
                    options.profile = "vs_5_0";
                else if(type == 'f')
                    options.profile = "ps_5_0";
                else if(type == 'c')
                    options.profile = "cs_5_0";
            }
            case bgfx::RendererType::Gnm:          //!< GNM
                break;
            case bgfx::RendererType::Metal:        //!< Metal
                break;
            case bgfx::RendererType::OpenGLES:     //!< OpenGL ES 2.0+
                break;
            case bgfx::RendererType::OpenGL:       //!< OpenGL 2.1+
            {
                if(type == 'v' || type == 'f')
                    options.profile = "120";
                else if(type == 'c')
                    options.profile = "430";
            }
            break;
            case bgfx::RendererType::Vulkan:       //!< Vulkan
                break;
            };
        }


        // include current dir
        std::string dir;
        {
            const char* base = bgfx::baseName(filePath);

            if (base != filePath)
            {
                dir.assign(filePath, base-filePath);
                options.includeDirs.push_back(dir.c_str());
            }
        }

        // set defines
        while (NULL != defines
        &&    '\0'  != *defines)
        {
            defines = bx::strws(defines);
            const char* eol = bx::strFind(defines, ';');
            if (NULL == eol)
            {
                eol = defines + bx::strLen(defines);
            }
            std::string define(defines, eol);
            options.defines.push_back(define.c_str() );
            defines = ';' == *eol ? eol+1 : eol;
        }


        // set varyingdef
        std::string defaultVarying = dir + "varying.def.sc";
        const char* varyingdef = varyingPath ? varyingPath : defaultVarying.c_str();
        bgfx::File attribdef(varyingdef);
        const char* parse = attribdef.getData();
        if (NULL != parse
        &&  *parse != '\0')
        {
            options.dependencies.push_back(varyingdef);
        }
        else
        {
            fprintf(stderr, "ERROR: Failed to parse varying def file: \"%s\" No input/output semantics will be generated in the code!\n", varyingdef);
            return nullptr;
        }



        // read shader source file
        bx::FileReader reader;
        if (!bx::open(&reader, filePath) )
        {
            fprintf(stderr, "Unable to open file '%s'.\n", filePath);
            return nullptr;
        }

        // add padding
        const size_t padding = 4096;
        uint32_t size = (uint32_t)bx::getSize(&reader);
        char* data = new char[size+padding+1];
        size = (uint32_t)bx::read(&reader, data, size);

        if (data[0] == '\xef'
                &&  data[1] == '\xbb'
                &&  data[2] == '\xbf')
        {
            bx::memMove(data, &data[3], size-3);
            size -= 3;
        }

        // Compiler generates "error X3000: syntax error: unexpected end of file"
        // if input doesn't have empty line at EOF.
        data[size] = '\n';
        bx::memSet(&data[size+1], 0, padding);
        bx::close(&reader);


        // compile shader.

        BufferWriter writer;
        if ( bgfx::compileShader(attribdef.getData(), data, size, options, &writer) )
        {
            // this will copy the compiled shader data to a memory block and return mem ptr
            return writer.finalize();
        }

        return nullptr;
    }
}
