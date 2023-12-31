#pragma once
#include <stdlib.h>

namespace MyNetwork
{
	struct RPCBuffer
	{
		explicit RPCBuffer(unsigned long long size)
		{ 
			data = (char*)malloc(size);
		}
		~RPCBuffer()
		{
			free(data);
		}

		void* Data()
		{
			return data;
		}

		char* data = nullptr;
	};
}