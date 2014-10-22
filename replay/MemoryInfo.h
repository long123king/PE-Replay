#pragma once

enum MemoryRegionType
{
	MemoryRegionType_None = 0,
	MemoryRegionType_Mapped_Image,
	MemoryRegionType_Mapped_Empty,
	MemoryRegionType_Stack,
	MemoryRegionType_Heap,
	MemoryRegionType_Mapped_Data
};

class MemoryRegion
{

};

class MemoryInfo
{
public:
	MemoryInfo(void);
	~MemoryInfo(void);
};
