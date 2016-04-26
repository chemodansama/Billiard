#include "StdAfx.h"
#include "utils.h"

#include <fstream>
#include <cassert>
#include <Windows.h>
#include <WinBase.h>
#include <Dbghelp.h>

#include "glog\logging.h"
#include <FreeImage.h>

namespace utils {
    glm::mat4 biasMatrix (
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    );

	std::string getExePath() {
        TCHAR szPath[MAX_PATH];

        if (auto size = GetModuleFileName(0, szPath, MAX_PATH)) {
            std::string filename(szPath);
            return filename.substr(0, filename.find_last_of("\\/") + 1);
        } else {
            LOG(ERROR) << "Cannot get module file name: " << GetLastError();
            return "";
        }
    }

	std::vector<char> loadAsset(const std::string &filename) {
		std::ifstream t(filename);
		std::vector<char> data;

		t.seekg(0, std::ios::end);   
		data.reserve(static_cast<size_t>(t.tellg()));
		t.seekg(0, std::ios::beg);

		data.assign((std::istreambuf_iterator<char>(t)),
					std::istreambuf_iterator<char>());

		return data;
	}

	std::vector<unsigned char> loadPng(const char *filename, 
			unsigned int *width, unsigned int *height, unsigned int *bpp) {
		std::vector<unsigned char> result;
		if (auto bitmap = FreeImage_Load(FIF_PNG, filename)) {
			*width = FreeImage_GetWidth(bitmap);
			*height = FreeImage_GetHeight(bitmap);
			*bpp = FreeImage_GetBPP(bitmap);

            if (*bpp != 24) {
                auto oldBitmap = bitmap;
                bitmap = FreeImage_ConvertTo32Bits(oldBitmap);
                FreeImage_Unload(oldBitmap);
                *bpp = FreeImage_GetBPP(bitmap);
            }
            assert(*bpp == 24 || *bpp == 32);

			auto type = FreeImage_GetImageType(bitmap);
			auto bits = FreeImage_GetBits(bitmap);
            
			auto pitch = FreeImage_GetPitch(bitmap);
			for (unsigned int y = 0; y < *height; y++) {
				auto pixel = (BYTE*)bits;
				for (unsigned int x = 0; x < *width; x++) {
					result.push_back(pixel[FI_RGBA_RED]);
					result.push_back(pixel[FI_RGBA_GREEN]);
					result.push_back(pixel[FI_RGBA_BLUE]);
					pixel += *bpp / 8;
				}
				// next line
				bits += pitch;
			}
			FreeImage_Unload(bitmap);
		}
		return result;
	}

    void printStack( void )
    {
         unsigned int   i;
         void         * stack[ 100 ];
         unsigned short frames;
         SYMBOL_INFO  * symbol;
         HANDLE         process;

         process = GetCurrentProcess();

         SymInitialize( process, NULL, TRUE );

         frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
         symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
         symbol->MaxNameLen   = 255;
         symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

         for( i = 0; i < frames; i++ ) {
             SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );
             printf( "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, (unsigned int)symbol->Address );
         }

         free( symbol );
    }
}