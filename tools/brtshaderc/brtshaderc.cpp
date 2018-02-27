#include <cstdio>
#include <bx/file.h>
#include <vector>

// hack to fix the multiple definition link errors
#define getUniformTypeName getUniformTypeName_shaderc
#define nameToUniformTypeEnum nameToUniformTypeEnum_shaderc
#define s_uniformTypeName s_uniformTypeName_shaderc

namespace shaderc
{
    /// basic struct to hold memory block infos
    struct MemoryBuffer
    {
        uint8_t* data;
        uint32_t size;
    };

    /// not a real FileWriter, but a hack to redirect write() to a memory block.
    class BufferWriter : public bx::FileWriter
    {
    public:

        BufferWriter(MemoryBuffer* memBuffer) :
            _memBuffer(memBuffer)
        {
        }

        ~BufferWriter()
        {
        }

        void finalize()
        {
            if(_memBuffer)
            {
                bx::DefaultAllocator crtAllocator;
                size_t size = _buffer.size() + 1;
                _memBuffer->data = (uint8_t*)bx::alloc(&crtAllocator, size);
                _memBuffer->size = size;

                bx::memCopy(_memBuffer->data, _buffer.data(), _buffer.size());
                _memBuffer->data[_memBuffer->size - 1] = '\0';
            }
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
        MemoryBuffer* _memBuffer;
    };
}


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
    const bgfx::Memory* compileShader(ShaderType type, const char* filePath, const char* defines, const char* varyingPath)
    {
        bgfx::Options options;
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

        MemoryBuffer mb;
        BufferWriter writer(&mb);
        if ( bgfx::compileShader(attribdef.getData(), data, size, options, &writer) )
        {
            // this will copy the compiled shader data to the MemoryBuffer
            writer.finalize();

            // make memory ref on MemoryBuffer
            const bgfx::Memory* mem = bgfx::makeRef(mb.data, mb.size);
            return mem;
        }

        return nullptr;
    }
}

#undef fprintf
#include "bx/allocator.h"
