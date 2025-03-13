#include "VCPU.hpp"

namespace Nisemono
{
	InstructionBase_t ProcessorAsm::CreateOpcode(
		IN EOpcode Opcode,
		IN uint16_t NumberOfOperands
	)
	{
		InstructionBase_t instruction_base{};
		instruction_base.Opcode = Opcode;
		instruction_base.OperandCount = NumberOfOperands;
		instruction_base.InstructionLength = sizeof(instruction_base) + (sizeof(InstructionOperand_t) * NumberOfOperands);

		return instruction_base;
	}

	InstructionOperand_t ProcessorAsm::CreateOperand(
		IN EProcessorRegister Register
	)
	{
		InstructionOperand_t operand{};
		operand.Type = OPERAND_REGISTER;
		operand.Register = Register;

		return operand;
	}

	void ProcessorAsm::FixInstructionSize(
		IN InstructionBase_t* Instruction, 
		IN const std::vector<InstructionOperand_t>& Operands
	)
	{
		for (auto operand : Operands)
		{
			if (operand.Type == OPERAND_REGISTER)
				continue;

			Instruction->InstructionLength += (operand.OperandSize + 1);
		}
	}

	void ProcessorAsm::EmitAdd(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_ADD, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitAdd(
		IN EProcessorRegister TargetRegister, 
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_ADD, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitSub(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_SUB, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitSub(
		IN EProcessorRegister TargetRegister,
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_SUB, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitMul(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_MUL, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitMul(
		IN EProcessorRegister TargetRegister, 
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_MUL, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitDiv(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_DIV, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitDiv(
		IN EProcessorRegister TargetRegister, 
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_DIV, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitXor(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_XOR, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitXor(
		IN EProcessorRegister TargetRegister, 
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_XOR, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitHalt(
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_HALT, 1);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
	}

	void ProcessorAsm::EmitMov(
		IN EProcessorRegister TargetRegister, 
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_MOV, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitMov(
		IN EProcessorRegister TargetRegister,
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_MOV, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitAnd(
		IN EProcessorRegister TargetRegister,
		IN EProcessorRegister SourceRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_AND, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
	}

	void ProcessorAsm::EmitAnd(
		IN EProcessorRegister TargetRegister, 
		IN int64_t SourceValue
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_AND, 2);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));
		operands.push_back(CreateOperand(SourceValue));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(operands[1]);
		Serialize(SourceValue);
	}

	void ProcessorAsm::EmitJmp(
		IN EProcessorRegister TargetRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JMP, 1);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
	}

	void ProcessorAsm::EmitJmp(
		IN const char* Label
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JMP, 1);

		int64_t label_offset = m_Labels[Label];
		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(label_offset));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(label_offset);
	}

	void ProcessorAsm::EmitJz(
		IN EProcessorRegister TargetRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JZ, 1);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
	}

	void ProcessorAsm::EmitJz(
		IN const char* Label
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JZ, 1);

		int64_t label_offset = m_Labels[Label];
		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(label_offset));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(label_offset);
	}

	void ProcessorAsm::EmitJnz(
		IN EProcessorRegister TargetRegister
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JNZ, 1);

		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(TargetRegister));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
	}

	void ProcessorAsm::EmitJnz(
		IN const char* Label
	)
	{
		InstructionBase_t instruction = CreateOpcode(INSTRUCTION_JNZ, 1);

		int64_t label_offset = m_Labels[Label];
		std::vector<InstructionOperand_t> operands;
		operands.push_back(CreateOperand(label_offset));

		FixInstructionSize(&instruction, operands);

		Serialize(instruction);
		Serialize(operands[0]);
		Serialize(label_offset);
	}

	void ProcessorAsm::CreateLabel(std::string_view Name)
	{
		m_Labels[Name.data()] = GetCurrentPosition();
	}

	size_t ProcessorAsm::GetCurrentPosition()
	{
		return m_AssembledBytes.size();
	}

	uint8_t* ProcessorAsm::GetBuffer()
	{
		return m_AssembledBytes.data();
	}

	void ProcessorAsm::Clear()
	{
		m_AssembledBytes.clear();
	}
}

