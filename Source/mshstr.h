//   MshStr.h
//   Class for handling temporary strings that are generated when moving from managed Strings
//   to char * and vice versa.

#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

namespace kernel
{
        ref class MarshalString
        {
        private:
                //IntPtr FreePtrs[];
                array<IntPtr^>^ FreePtrs;                       // Strings que debe eliminar.
                int FreePtrsCount;                              // Strings a liberar.

        public:
                MarshalString();                                // Constructor.
                ~MarshalString();                               // Destructor.

                char *GetString(String ^src);                   // Convierte 'String' en 'char *'
                String ^SetString(char *src);                   // Convierte 'char *' en 'String'
                String ^SetString(char *src, int len);          // Convierte 'void *' en 'String'
                void FreeTemp();                                // Libera las instancias utilizadas.
        };
}
