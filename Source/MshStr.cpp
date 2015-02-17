//   MshStr.cpp
//   Implementation of the 'MarshalString' class, which is used to implement conversion between
//   System::String and char *.

#include "stdafx.h"
#include "mshstr.h"

namespace kernel
{
        //   Constructor.
        MarshalString::MarshalString()
        {
                FreePtrs = gcnew array<IntPtr^>(16);
        }

        //   Destructor.
        MarshalString::~MarshalString()
        {
                FreeTemp();
                delete FreePtrs;
        }

        //   Converts a string type 'System :: String' to another type 'char *', but with proper C ++
        char *MarshalString::GetString(String ^src)
        {
                IntPtr p_src = Marshal::StringToHGlobalAnsi(src);
                FreePtrs[FreePtrsCount++] = p_src;
                return static_cast<char *> (p_src.ToPointer());
        }

        //   Removes all temporary strings that may have been generated with the above function.
        void MarshalString::FreeTemp()
        {
                while(FreePtrsCount > 0)
                {
                        FreePtrsCount--;
                        Marshal::FreeHGlobal((IntPtr) FreePtrs[FreePtrsCount]);
                }
        }

        //   Converts a type 'char *' string into another type 'System :: String', more appropriate for managed languages.
        String ^MarshalString::SetString(char *src)
        {
                return Marshal::PtrToStringAnsi((IntPtr) (char *) src);
        }

        //   Converts a type 'char *' buffer, of a given length, to a 'System :: String'.
        String ^MarshalString::SetString(char *src, int len)
        {
                return Marshal::PtrToStringAnsi((IntPtr)(char *) src, len);
        }
}
