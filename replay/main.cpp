#include "pin.H"
#include "portability.H"

#include <string>

#include "Config.h"
#include "REProgramInfo.h"

KNOB<std::string> output_folder(
	KNOB_MODE_WRITEONCE,
	"pintool",
	"o",
	"",
	"Output Folder");


KNOB<std::string> result_folder(
	KNOB_MODE_WRITEONCE,
	"pintool",
	"r",
	"result",
	"Sub-folder of report folder");

static boost::uint32_t Usage()
{
    //printf("pin.exe -t FPGate.dll -- application.exe [arg1...]\n");
	printf("%s\n", KNOB_BASE::StringKnobSummary());
    return -1;
}

VOID Encounter_Ins(INS ins, void *v)
{	
	((REProgramInfo*)v)->Process_Encounter_Insn(ins);
}

VOID Encounter_Img(IMG img, void* v)
{
	((REProgramInfo*)v)->Process_Encounter_Image(img);
}

VOID Encounter_Rtn(RTN rtn, void* v)
{
	((REProgramInfo*)v)->Process_Encounter_Rtn(rtn);
}

VOID Encounter_Trace(TRACE trace, void* v)
{
	((REProgramInfo*)v)->Process_Encounter_Trace(trace);
}

VOID Encounter_Finalize(int n, void *v)
{    
	((REProgramInfo*)v)->Process_Encounter_Finalize();
}

EXCEPT_HANDLING_RESULT Encounter_Exception(THREADID tid, 
										EXCEPTION_INFO * pExceptInfo, 
										PHYSICAL_CONTEXT * pPhysCtxt, 
										VOID * arg)
{
	//printf("Exception caught!\n");
	LOQ(CONFIG_EXCETION,
		"Uncaught Exception: %s\n", 
		PIN_ExceptionToString(pExceptInfo).c_str());

	printf("Uncaught Exception: %s\n", PIN_ExceptionToString(pExceptInfo).c_str());
	((REProgramInfo*)arg)->ExitProcess();
	return EHR_HANDLED;
}

int main(int argc, char *argv[])
{
	//boost::filesystem::create_directories("H:\\Coverit\\trunk\\report\\");
	//return 0;

	if( PIN_Init(argc,argv) )
	{
		return Usage();
	}    

	if (argc <= 6)
		return -1;

	std::string filename = result_folder.Value();

	Config::Initialize(output_folder.Value().c_str(), filename.c_str());

	REProgramInfo* program = new REProgramInfo(filename.c_str());

	PIN_InitSymbols();
	//INS_AddInstrumentFunction(Encounter_Ins, program);
	IMG_AddInstrumentFunction(Encounter_Img, program);
	//RTN_AddInstrumentFunction(Encounter_Rtn, program);
	TRACE_AddInstrumentFunction(Encounter_Trace, program);

	PIN_AddInternalExceptionHandler(Encounter_Exception, program);
	PIN_AddFiniFunction(Encounter_Finalize, program);

	PIN_StartProgram();

	return 0;
}
