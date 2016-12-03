// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuX86.cpp
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *  (c) 2016 Luke Usher <luke.usher@outlook.com>
// *  All rights reserved
// *
// ******************************************************************
#define _CXBXKRNL_INTERNAL
#define _XBOXKRNL_DEFEXTRN_

#include <Zydis.hpp>

#include "CxbxKrnl.h"
#include "Emu.h"
#include "EmuX86.h"
#include "EmuNV2A.h"

#include "../../import/asmjit-next/src/asmjit/asmjit.h"
asmjit::VMemMgr vm;

bool EmuX86_AsmJitGp(asmjit::X86Gp* out, Zydis::Register reg)
{
	using namespace asmjit::x86;

	switch (reg) {
		case Zydis::Register::AH: *out = ah; break;
		case Zydis::Register::AL: *out = al; break;
		case Zydis::Register::AX: *out = ax; break;
		case Zydis::Register::BH: *out = bh; break;
		case Zydis::Register::BL: *out = bl; break;
		case Zydis::Register::BP: *out = bp; break;
		case Zydis::Register::BPL: *out = bpl; break;
		case Zydis::Register::BX: *out = bx; break;
		case Zydis::Register::CH: *out = ch; break;
		case Zydis::Register::CL: *out = cl; break;
		case Zydis::Register::EAX: *out = eax; break;
		case Zydis::Register::EBX: *out = ebx; break;
		case Zydis::Register::ECX: *out = ecx; break;
		case Zydis::Register::EDI: *out = edi; break;
		case Zydis::Register::EDX: *out = edx; break;
		case Zydis::Register::ESI: *out = esi; break;
		default: return false;
	}

	return true;
}

bool EmuX86_CompileMOV(Zydis::InstructionInfo& info, asmjit::X86Assembler& a);

bool EmuX86_CompileBlock(uint32_t addr)
{
	Zydis::MemoryInput input((uint8_t*)addr, XBOX_MEMORY_SIZE - addr);
	Zydis::InstructionInfo info;
	Zydis::InstructionDecoder decoder;
	Zydis::IntelInstructionFormatter formatter;

	decoder.setDisassemblerMode(Zydis::DisassemblerMode::M32BIT);
	decoder.setDataSource(&input);
	decoder.setInstructionPointer(addr);

	// Setup ASMJit
	using namespace asmjit;
	using namespace asmjit::x86;

	CodeHolder code;         
	code.init(CodeInfo(ArchInfo::kTypeX86));
	X86Assembler a(&code);

	bool completed = false;
	while (!completed) {
		// Decode a single instruction
		decoder.decodeInstruction(info);

		if (info.flags & Zydis::IF_ERROR_MASK)
		{
			EmuWarning("EmuX86: Error decoding opcode at 0x%08X\n", addr);
		}
		else
		{
			DbgPrintf("EmuX86: 0x%08X: %s\n", (uint32_t)info.instrAddress, formatter.formatInstruction(info));

			bool result = false;
			switch (info.mnemonic) {
			case Zydis::InstructionMnemonic::MOV:
				result = EmuX86_CompileMOV(info, a);
				break;
			// If we hit any instruction that alters the program counter, insert a jmp to that instruction and mark completed
			case Zydis::InstructionMnemonic::CALL: case Zydis::InstructionMnemonic::JB:  case Zydis::InstructionMnemonic::JBE:
			case Zydis::InstructionMnemonic::JCXZ: case Zydis::InstructionMnemonic::JL:  case Zydis::InstructionMnemonic::JLE:
			case Zydis::InstructionMnemonic::JMP:  case Zydis::InstructionMnemonic::JNB: case Zydis::InstructionMnemonic::JNO:
			case Zydis::InstructionMnemonic::JNP:  case Zydis::InstructionMnemonic::JNS: case Zydis::InstructionMnemonic::JO:
			case Zydis::InstructionMnemonic::JP:   case Zydis::InstructionMnemonic::JS:  case Zydis::InstructionMnemonic::LOOP:
			case Zydis::InstructionMnemonic::RETF: case Zydis::InstructionMnemonic::RSM: case Zydis::InstructionMnemonic::SYSENTER:
			case Zydis::InstructionMnemonic::SYSEXIT:
				a.jmp(info.instrAddress);
				completed = true;
				result = true;
				break;
			}

			if (!result) {
				CxbxKrnlCleanup("EmuX86: 0x%08X: %s Not Implemented\n", (uint32_t)info.instrAddress, formatter.formatInstruction(info));
			}
		}
	}

	// The code has been completed, we need to relocate the code into a buffer
	void* buffer = vm.alloc(code.getCodeSize());
	code.relocate(buffer, (uint32_t)buffer);

	// Patch the original code to jmp to the new buffer
	*(uint08*)addr = 0xE9;
	*(uint32*)(addr + 1) = (uint32)buffer - addr - 5;
	return true;
}

bool EmuX86_CompileMOV(Zydis::InstructionInfo& info, asmjit::X86Assembler& a)
{
	Zydis::Register src_reg = Zydis::Register::NONE;
	Zydis::Register dst_reg = Zydis::Register::NONE;

	if (info.operand[0].type == Zydis::OperandType::REGISTER) {
		dst_reg = info.operand[0].base;
	}

	if (info.operand[1].type == Zydis::OperandType::REGISTER) {
		src_reg = info.operand[1].base;
	}

	if (info.operand[0].type == Zydis::OperandType::REGISTER && info.operand[1].type == Zydis::OperandType::REGISTER) {
		asmjit::X86Gp src, dst;
		
		if (!EmuX86_AsmJitGp(&src, src_reg) || !EmuX86_AsmJitGp(&dst, dst_reg)) {
			return false;
		}

		a.mov(src, dst);
		return true;
	}

	// Memory Write
	if (info.operand[0].type == Zydis::OperandType::MEMORY) {
		if (src_reg != Zydis::Register::NONE) {
			a.pusha();

			asmjit::X86Gp src;
			asmjit::X86Gp base;
			asmjit::X86Gp index;

			// Setup registers
			EmuX86_AsmJitGp(&src, src_reg);
			EmuX86_AsmJitGp(&base, info.operand[0].base);
			EmuX86_AsmJitGp(&index, info.operand[0].index);

			// Make Base register = full memory address
			a.mul(index, info.operand[0].scale);
			a.add(base, index);

			a.push(src);
			a.push(base);

			switch (info.operand[1].size) {
				case 8:
					a.call((uint32_t)EmuX86_Write8);
					break;
				case 16:
					a.call((uint32_t)EmuX86_Write16);
					break;
				case 32:
					a.call((uint32_t)EmuX86_Write32);
					break;
			}

			a.popa();
			return true;
		}
	}

	// Memory Read
	
	return false;
}

bool EmuX86_DecodeException(LPEXCEPTION_POINTERS e)
{
	if (e->ContextRecord->Eip > XBOX_MEMORY_SIZE || e->ContextRecord->Eip < 0x10000) {
		return false;
	}

	return EmuX86_CompileBlock(e->ContextRecord->Eip);
}

uint8_t __stdcall EmuX86_Read8(uint32_t addr)
{
	EmuWarning("EmuX86_Read8: Unknown Read Address %02X", addr);
	return 0;
}

uint16_t __stdcall EmuX86_Read16(uint32_t addr)
{
	EmuWarning("EmuX86_Read16: Unknown Read Address %04X", addr);
	return 0;
}

uint32_t __stdcall EmuX86_Read32(uint32_t addr)
{
	if (addr >= 0xFD000000 && addr <= 0xFE000000) {
		return EmuNV2A_Read32(addr & 0x00FFFFFF);
	}

	EmuWarning("EmuX86_Read32: Unknown Read Address %08X", addr);
	return 0;
}

void __stdcall EmuX86_Write8(uint32_t addr, uint8_t value)
{
	EmuWarning("EmuX86_Write8: Unknown Write Address %08X (value %02X)", addr, value);
}

void __stdcall EmuX86_Write16(uint32_t addr, uint16_t value)
{
	EmuWarning("EmuX86_Write16: Unknown Write Address %08X (value %04X)", addr, value);
}

void __stdcall EmuX86_Write32(uint32_t addr, uint32_t value)
{
	if (addr >= 0xFD000000 && addr <= 0xFE000000) {
		EmuNV2A_Write32(addr & 0x00FFFFFF, value);
		return;
	}

	EmuWarning("EmuX86_Write32: Unknown Write Address %08X (value %08X)", addr, value);
}
