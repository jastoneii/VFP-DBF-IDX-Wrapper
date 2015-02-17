# VFP-DBF-IDX-Wrapper
C++ Code to Read/Write/Create VFP DBF and IDX Files.  

The attached files were originally posted to C# Corner by Ricardo Federico  on Feb 25, 2002. The purpose of the code was to create a C# or VB.net wrapper that allowed access to DBF and IDX files without a VFP driver.  Given VFP is no longer supported by MS and future versions of Windows may make existing VFP drivers/DLLs obsolete, I decided to update Ricardo's code to work with VS Express and give it more complete functionality. Ricardo's originally code comments were in Spanish.  I have translated them to English or added my own comments.  If you prefer Spanish, Ricardo's code is still available on C# Corner.

As I am relatively new to VS Express C++, I'm sure the code could be cleaned up more to make it more standard; but, it does work.  Index files can be created based on a single field or multiple fields so long as the result is a concatenated string. An index may convert a number to a string, convert a string/character field to upper-case and convert a date field to a string. 

The ConsoleApplication.cpp file shows various examples on how to create .idx files and most of the existing available functions.

AppendFrom could/should be faster than it is.  I tried to make it so that if you are appending from a file that has corresponding field names but those fields are of different data types, AppendFrom will attempt to do the appropriate conversion when possible.

.DBC functionality would be a good thing to add as well as Memo field capability.  Tested datatypes are: char, double, int,  number, date and logical.

