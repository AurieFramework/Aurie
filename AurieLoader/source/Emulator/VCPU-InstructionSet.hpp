#pragma once
#include <cstdint>

namespace Nisemono
{
	enum EProcessorRegister : uint8_t
	{
		REGISTER_RV = 0,
		REGISTER_A,
		REGISTER_B,
		REGISTER_C,
		REGISTER_IP,
		REGISTER_SP
	};

	enum EInstructionOperandType : uint8_t
	{
		OPERAND_REGISTER = 0,
		OPERAND_IMMEDIATE = 1
	};

	union InstructionOperand_t
	{
		struct
		{
			EInstructionOperandType Type : 2;
			// Unused if Type == OPERAND_IMMEDIATE, it comes after operands in that case.
			EProcessorRegister Register : 3;
			// 0-7, actual size is this +1.
			uint8_t OperandSize : 3;
		};
		uint8_t Bitfield;
	};
	static_assert(sizeof(InstructionOperand_t) == sizeof(uint8_t));

	enum EOpcode : uint8_t
	{
		// ADD <target_reg, source_reg> => int64_t
		// ADD <target_reg, int64_t> => int64_t
		INSTRUCTION_ADD,
		// SUB <target_reg, source_reg> => int64_t
		// SUB <target_reg, int64_t> => int64_t
		INSTRUCTION_SUB,
		// MUL <target_reg, source_reg> => int64_t
		// MUL <target_reg, int64_t> => int64_t
		INSTRUCTION_MUL,
		// DIV <target_reg, source_reg> => int64_t
		// DIV <target_reg, int64_t> => int64_t
		INSTRUCTION_DIV,
		// XOR <target_reg, source_reg> => int64_t
		// XOR <target_reg, int64_t> => int64_t
		INSTRUCTION_XOR,
		// HALT <source_reg>
		INSTRUCTION_HALT,
		// MOV <target_reg, source_reg>
		// MOV <target_reg, imm64>
		INSTRUCTION_MOV,
		// AND <target_reg, source_reg>
		// AND <target_reg, imm64>
		INSTRUCTION_AND,
		// CMP <target_reg, target_reg>
		// CMP <target_reg, imm64>
		INSTRUCTION_CMP,
		// MEMCPY <imm64/reg, imm64/reg, imm64/reg>
		INSTRUCTION_MEMCPY,
		// JMP <imm64/reg>
		INSTRUCTION_JMP,
		// JZ <imm64/reg>
		INSTRUCTION_JZ,
		// JNZ <imm64/reg>
		INSTRUCTION_JNZ
	};

	union InstructionBase_t
	{
		struct
		{
			// The length of the current instruction, in bytes.
			// We use 4 bits for instructions with lengths 0-31.
			uint16_t InstructionLength : 5;
			// The opcode to use.
			uint16_t Opcode : 8;
			// The number of operands.
			// After the BaseInformation block, there are OperandCount operands (each being 1 byte) that reflect each operand.
			uint16_t OperandCount : 3;
		};
		uint16_t Bitfield;
	};
	static_assert(sizeof(InstructionBase_t) == sizeof(uint16_t));
}
