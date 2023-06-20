// Disassembler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <map>
#include <string>


// virtual CPU registers
#define SREG_SP           1     // stack pointer
#define SREG_MAR          2     // memory address register
#define SREG_AX           3     // general purpose
#define SREG_BX           4
#define SREG_CX           5
#define SREG_OP           6    // object pointer for member func calls
#define SREG_DX           7
#define CC_NUM_REGISTERS  8

// virtual CPU commands
#define SCMD_ADD          1     // reg1 += arg2
#define SCMD_SUB          2     // reg1 -= arg2
#define SCMD_REGTOREG     3     // reg2 = reg1
#define SCMD_WRITELIT     4     // m[MAR] = arg2 (copy arg1 bytes)
#define SCMD_RET          5     // return from subroutine
#define SCMD_LITTOREG     6     // set reg1 to literal value arg2
#define SCMD_MEMREAD      7     // reg1 = m[MAR]
#define SCMD_MEMWRITE     8     // m[MAR] = reg1
#define SCMD_MULREG       9     // reg1 *= reg2
#define SCMD_DIVREG       10    // reg1 /= reg2
#define SCMD_ADDREG       11    // reg1 += reg2
#define SCMD_SUBREG       12    // reg1 -= reg2
#define SCMD_BITAND       13    // bitwise  reg1 & reg2
#define SCMD_BITOR        14    // bitwise  reg1 | reg2
#define SCMD_ISEQUAL      15    // reg1 == reg2   reg1=1 if true, =0 if not
#define SCMD_NOTEQUAL     16    // reg1 != reg2
#define SCMD_GREATER      17    // reg1 > reg2
#define SCMD_LESSTHAN     18    // reg1 < reg2
#define SCMD_GTE          19    // reg1 >= reg2
#define SCMD_LTE          20    // reg1 <= reg2
#define SCMD_AND          21    // (reg1!=0) && (reg2!=0) -> reg1
#define SCMD_OR           22    // (reg1!=0) || (reg2!=0) -> reg1
#define SCMD_CALL         23    // jump to subroutine at reg1
#define SCMD_MEMREADB     24    // reg1 = m[MAR] (1 byte)
#define SCMD_MEMREADW     25    // reg1 = m[MAR] (2 bytes)
#define SCMD_MEMWRITEB    26    // m[MAR] = reg1 (1 byte)
#define SCMD_MEMWRITEW    27    // m[MAR] = reg1 (2 bytes)
#define SCMD_JZ           28    // jump if ax==0 to arg1
#define SCMD_PUSHREG      29    // m[sp]=reg1; sp++
#define SCMD_POPREG       30    // sp--; reg1=m[sp]
#define SCMD_JMP          31    // jump to arg1
#define SCMD_MUL          32    // reg1 *= arg2
#define SCMD_CALLEXT      33    // call external (imported) function reg1
#define SCMD_PUSHREAL     34    // push reg1 onto real stack
#define SCMD_SUBREALSTACK 35    // decrement stack ptr by literal
#define SCMD_LINENUM      36    // debug info - source code line number
#define SCMD_CALLAS       37    // call external script function
#define SCMD_THISBASE     38    // current relative address
#define SCMD_NUMFUNCARGS  39    // number of arguments for ext func call
#define SCMD_MODREG       40    // reg1 %= reg2
#define SCMD_XORREG       41    // reg1 ^= reg2
#define SCMD_NOTREG       42    // reg1 = !reg1
#define SCMD_SHIFTLEFT    43    // reg1 = reg1 << reg2
#define SCMD_SHIFTRIGHT   44    // reg1 = reg1 >> reg2
#define SCMD_CALLOBJ      45    // next call is member function of reg1
#define SCMD_CHECKBOUNDS  46    // check reg1 is between 0 and arg2
#define SCMD_MEMWRITEPTR  47    // m[MAR] = reg1 (adjust ptr addr)
#define SCMD_MEMREADPTR   48    // reg1 = m[MAR] (adjust ptr addr)
#define SCMD_MEMZEROPTR   49    // m[MAR] = 0    (blank ptr)
#define SCMD_MEMINITPTR   50    // m[MAR] = reg1 (but don't free old one)
#define SCMD_LOADSPOFFS   51    // MAR = SP - arg1 (optimization for local var access)
#define SCMD_CHECKNULL    52    // error if MAR==0
#define SCMD_FADD         53    // reg1 += arg2 (float,int)
#define SCMD_FSUB         54    // reg1 -= arg2 (float,int)
#define SCMD_FMULREG      55    // reg1 *= reg2 (float)
#define SCMD_FDIVREG      56    // reg1 /= reg2 (float)
#define SCMD_FADDREG      57    // reg1 += reg2 (float)
#define SCMD_FSUBREG      58    // reg1 -= reg2 (float)
#define SCMD_FGREATER     59    // reg1 > reg2 (float)
#define SCMD_FLESSTHAN    60    // reg1 < reg2 (float)
#define SCMD_FGTE         61    // reg1 >= reg2 (float)
#define SCMD_FLTE         62    // reg1 <= reg2 (float)
#define SCMD_ZEROMEMORY   63    // m[MAR]..m[MAR+(arg1-1)] = 0
#define SCMD_CREATESTRING 64    // reg1 = new String(reg1)
#define SCMD_STRINGSEQUAL 65    // (char*)reg1 == (char*)reg2   reg1=1 if true, =0 if not
#define SCMD_STRINGSNOTEQ 66    // (char*)reg1 != (char*)reg2
#define SCMD_CHECKNULLREG 67    // error if reg1 == NULL
#define SCMD_LOOPCHECKOFF 68    // no loop checking for this function
#define SCMD_MEMZEROPTRND 69    // m[MAR] = 0    (blank ptr, no dispose if = ax)
#define SCMD_JNZ          70    // jump to arg1 if ax!=0
#define SCMD_DYNAMICBOUNDS 71   // check reg1 is between 0 and m[MAR-4]
#define SCMD_NEWARRAY     72    // reg1 = new array of reg1 elements, each of size arg2 (arg3=managed type?)
#define SCMD_NEWUSEROBJECT 73   // reg1 = new user object of arg1 size

#define CC_NUM_SCCMDS     74

struct ScriptCommandInfo
{
    int opcode;
    std::string mnemonic;
    size_t num_param;
    bool param_is_reg[2];
};

 ScriptCommandInfo const sccmd_info[CC_NUM_SCCMDS] =
{
    { 0                    , "NULL"              , 0, { false, false, } },
    { SCMD_ADD             , "addi"              , 2, { true, false, } },
    { SCMD_SUB             , "subi"              , 2, { true, false, } },
    { SCMD_REGTOREG        , "mov"               , 2, { true, true, } },
    { SCMD_WRITELIT        , "memwritelit"       , 2, { false, false, } },
    { SCMD_RET             , "ret"               , 0, { false, false, } },
    { SCMD_LITTOREG        , "movl"              , 2, { true, false, } },
    { SCMD_MEMREAD         , "memread4"          , 1, { true, false, } },
    { SCMD_MEMWRITE        , "memwrite4"         , 1, { true, false, } },
    { SCMD_MULREG          , "mul"               , 2, { true, true, } },
    { SCMD_DIVREG          , "div"               , 2, { true, true, } },
    { SCMD_ADDREG          , "add"               , 2, { true, true, } },
    { SCMD_SUBREG          , "sub"               , 2, { true, true, } },
    { SCMD_BITAND          , "and"               , 2, { true, true, } },
    { SCMD_BITOR           , "or"                , 2, { true, true, } },
    { SCMD_ISEQUAL         , "cmpeq"             , 2, { true, true, } },
    { SCMD_NOTEQUAL        , "cmpne"             , 2, { true, true, } },
    { SCMD_GREATER         , "gt"                , 2, { true, true, } },
    { SCMD_LESSTHAN        , "lt"                , 2, { true, true, } },
    { SCMD_GTE             , "gte"               , 2, { true, true, } },
    { SCMD_LTE             , "lte"               , 2, { true, true, } },
    { SCMD_AND             , "land"              , 2, { true, true, } },
    { SCMD_OR              , "lor"               , 2, { true, true, } },
    { SCMD_CALL            , "call"              , 1, { true, false, } },
    { SCMD_MEMREADB        , "memread1"          , 1, { true, false, } },
    { SCMD_MEMREADW        , "memread2"          , 1, { true, false, } },
    { SCMD_MEMWRITEB       , "memwrite1"         , 1, { true, false, } },
    { SCMD_MEMWRITEW       , "memwrite2"         , 1, { true, false, } },
    { SCMD_JZ              , "jzi"               , 1, { false, false, } },
    { SCMD_PUSHREG         , "push"              , 1, { true, false, } },
    { SCMD_POPREG          , "pop"               , 1, { true, false, } },
    { SCMD_JMP             , "jmpi"              , 1, { false, false, } },
    { SCMD_MUL             , "muli"              , 2, { true, false, } },
    { SCMD_CALLEXT         , "farcall"           , 1, { true, false, } },
    { SCMD_PUSHREAL        , "farpush"           , 1, { true, false, } },
    { SCMD_SUBREALSTACK    , "farsubsp"          , 1, { false, false, } },
    { SCMD_LINENUM         , "sourceline"        , 1, { false, false, } },
    { SCMD_CALLAS          , "callscr"           , 1, { true, false, } },
    { SCMD_THISBASE        , "thisaddr"          , 1, { false, false, } },
    { SCMD_NUMFUNCARGS     , "setfuncargs"       , 1, { false, false, } },
    { SCMD_MODREG          , "mod"               , 2, { true, true, } },
    { SCMD_XORREG          , "xor"               , 2, { true, true, } },
    { SCMD_NOTREG          , "not"               , 1, { true, false, } },
    { SCMD_SHIFTLEFT       , "shl"               , 2, { true, true, } },
    { SCMD_SHIFTRIGHT      , "shr"               , 2, { true, true, } },
    { SCMD_CALLOBJ         , "callobj"           , 1, { true, false, } },
    { SCMD_CHECKBOUNDS     , "checkbounds"       , 2, { true, false, } },
    { SCMD_MEMWRITEPTR     , "memwrite.ptr"      , 1, { true, false, } },
    { SCMD_MEMREADPTR      , "memread.ptr"       , 1, { true, false, } },
    { SCMD_MEMZEROPTR      , "memwrite.ptr.0"    , 0, { false, false, } },
    { SCMD_MEMINITPTR      , "meminit.ptr"       , 1, { true, false, } },
    { SCMD_LOADSPOFFS      , "load.sp.offs"      , 1, { false, false, } },
    { SCMD_CHECKNULL       , "checknull.ptr"     , 0, { false, false, } },
    { SCMD_FADD            , "faddi"             , 2, { true, false, } },
    { SCMD_FSUB            , "fsubi"             , 2, { true, false, } },
    { SCMD_FMULREG         , "fmul"              , 2, { true, true, } },
    { SCMD_FDIVREG         , "fdiv"              , 2, { true, true, } },
    { SCMD_FADDREG         , "fadd"              , 2, { true, true, } },
    { SCMD_FSUBREG         , "fsub"              , 2, { true, true, } },
    { SCMD_FGREATER        , "fgt"               , 2, { true, true, } },
    { SCMD_FLESSTHAN       , "flt"               , 2, { true, true, } },
    { SCMD_FGTE            , "fgte"              , 2, { true, true, } },
    { SCMD_FLTE            , "flte"              , 2, { true, true, } },
    { SCMD_ZEROMEMORY      , "zeromem"           , 1, { false, false, } },
    { SCMD_CREATESTRING    , "newstring"         , 1, { true, false, } },
    { SCMD_STRINGSEQUAL    , "streq"             , 2, { true, true, } },
    { SCMD_STRINGSNOTEQ    , "strne"             , 2, { true, true, } },
    { SCMD_CHECKNULLREG    , "checknull"         , 1, { true, false, } },
    { SCMD_LOOPCHECKOFF    , "loopcheckoff"      , 0, { false, false, } },
    { SCMD_MEMZEROPTRND    , "memwrite.ptr.0.nd" , 0, { false, false, } },
    { SCMD_JNZ             , "jnzi"              , 1, { false, false, } },
    { SCMD_DYNAMICBOUNDS   , "dynamicbounds"     , 1, { true, false, } },
    { SCMD_NEWARRAY        , "newarray"          , 3, { true, false, } },
    { SCMD_NEWUSEROBJECT   , "newuserobject"     , 2, { true, false, } },
 };

const std::string regname[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };

int index = -1;

void write_reg(int param)
{
    std::string reg;
    if (param >= 0 && param < 8)
    {
        std::cout << regname[param];
        return;
    }
    std::cout << "[reg " << param << "]";
}

void write_jump(int param)
{
    std::cout << param << " [to " << (index + 1 + param) << "]";
}

void write_lit(int param, bool convert_to_float)
{
    double d = *(float *) &param; // interpret as a float
    std::cout.width(0);
    std::cout << param;
    if (convert_to_float)
        std::cout << " [float " << d << ']';
}


inline bool input_ends() { return (std::cin.eof() || std::cin.fail()); }

int get()
{
    ++index;
    while (!input_ends())
    {
        int ch = std::cin.peek();
        if ((ch >= '0' && ch <= '9') || (ch == '-'))
            break;
        if (ch == '/')
        {
            std::string throwaway;
            std::getline(std::cin, throwaway);
        }
        else
            std::cin.get();
    }
    int ret;
    std::cin >> ret;
    return ret;
}

int main()
{
    if ('=' == std::cin.peek())
    {
        index = get();
        index--;
    }
    while(!input_ends())
    {
        
        int opcode = get();
        std::cout.width(4);
        std::cout << index << ":   ";
        
        if (input_ends()) break;

        if (opcode < 0 || opcode >= CC_NUM_SCCMDS)
        {
            std::cout << "Illegal opcode " << opcode << " EXITING";
            break;
        }
        ScriptCommandInfo sci = sccmd_info[opcode];

        std::cout.width(5);
        std::cout.setf(std::ios::adjustfield, std::ios::left);
        std::cout << sci.mnemonic << "   ";
        std::cout.unsetf(std::ios::adjustfield);

        for (size_t idx = 0; idx < sci.num_param; ++idx)
        {
            int param = get();
            if (idx < 2 && sci.param_is_reg[idx])
                write_reg(param);
            else if (opcode == SCMD_JNZ || opcode == SCMD_JMP || opcode == SCMD_JZ)
                write_jump(param);
            else
                write_lit(param, 
                    opcode != SCMD_CHECKBOUNDS &&
                    opcode != SCMD_LINENUM &&
                    opcode != SCMD_LOADSPOFFS &&
                    opcode != SCMD_NEWARRAY &&
                    opcode != SCMD_NEWUSEROBJECT &&
                    opcode != SCMD_ZEROMEMORY);
            if (idx != sci.num_param - 1)
                std::cout << ",   ";
        }
        std::cout << std::endl;
        std::cout.flush();
    }
}
