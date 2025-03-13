#pragma once
#include "VCPU-InstructionSet.hpp"
#include <cstdint>
#include <bit>
#include <vector>
#include <map>
#include <string>
#if _DEBUG
#include <Windows.h>
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef THROW_ERROR
#define THROW_ERROR(x) ::Nisemono::ThrowError(x)
#endif

namespace Nisemono
{
	inline void ThrowError(const char* Message)
	{
#if _DEBUG
		MessageBoxA(0, Message, 0, MB_ICONERROR | MB_TOPMOST);
		DebugBreak();
#endif // _DEBUG
	}

	class Processor;
	struct ProcessorRegister_t
	{
		unsigned char m_Value[8];

		template <typename T, typename _ = typename std::enable_if_t<sizeof(T) <= sizeof(m_Value)>>
		constexpr T& Cast()
		{
			return *std::bit_cast<T*>(&m_Value);
		}

		template <typename T, typename _ = typename std::enable_if_t<sizeof(T) <= sizeof(m_Value)>>
		constexpr const T& Cast() const
		{
			return *std::bit_cast<T*>(&m_Value);
		}
	};

	struct ProcessorState_t
	{
		// Zero-Flag. Set if result of last CMP is 0
		bool ZF;

		// Return value register
		ProcessorRegister_t RV;

		// General-purpose register A
		ProcessorRegister_t RGPA;
		// General-purpose register B
		ProcessorRegister_t RGPB;
		// General-purpose register C
		ProcessorRegister_t RGPC;

		// Instruction pointer register
		ProcessorRegister_t RIP;
		// Stack pointer register
		ProcessorRegister_t RSP;
	};

	namespace Instructions
	{
		void Add(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Add(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Sub(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Sub(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Mul(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Mul(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Div(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Div(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Xor(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Xor(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Halt(
			IN Processor& Context,
			IN ProcessorRegister_t& Target
		);

		void And(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void And(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);

		void Cmp(
			IN ProcessorState_t& State,
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		);

		void Cmp(
			IN ProcessorState_t& State,
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		);
	}

	inline std::vector<InstructionOperand_t*> GetInstructionOperands(
		IN InstructionBase_t* Instruction
	)
	{
		// Operands are right after the InstructionBase_t, and are pushed in the correct order.
		std::vector<InstructionOperand_t*> operands(Instruction->OperandCount);
		for (size_t i = 0; i < Instruction->OperandCount; i++)
			operands[i] = reinterpret_cast<InstructionOperand_t*>(Instruction + 1) + i;

		return operands;
	}

	template <typename T>
	__forceinline T& GetImmediateOperand(
		IN InstructionBase_t* Instruction,
		IN int Index
	)
	{
		auto operand_descriptors = GetInstructionOperands(Instruction);

		size_t offset = 0;
		for (int i = 0; i < Index; i++)
		{
			if (operand_descriptors[i]->Type != OPERAND_REGISTER)
			{
				offset += (operand_descriptors[i]->OperandSize + 1);
			}
		}

		// We first get the last operand descriptor, and go beyond it (+1).
		// Then, cast that result to uint8_t* and step by 'offset' bytes.
		// After that, cast to T* and dereference.
		return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(operand_descriptors.back() + 1) + offset);
	}

	__forceinline EInstructionOperandType GetInstructionOperandType(
		IN InstructionBase_t* Instruction,
		IN int Index
	)
	{
		return GetInstructionOperands(Instruction)[Index]->Type;
	}

	class Processor
	{
	private:
		ProcessorState_t m_State;
		uint8_t* m_InstructionBuffer;
		size_t m_BufferSize;

		constexpr Processor() noexcept(true)
		{
			m_InstructionBuffer = nullptr;
			m_BufferSize = 0;
			m_State = {};
		}
	public:
		template <typename T>
		constexpr void WriteRegister(
			IN EProcessorRegister Register,
			IN T Value
		)
		{
			switch (Register)
			{
			case REGISTER_RV:
				this->m_State.RV.Cast<T>() = Value;
				return;
			case REGISTER_A:
				this->m_State.RGPA.Cast<T>() = Value;
				return;
			case REGISTER_B:
				this->m_State.RGPB.Cast<T>() = Value;
				return;
			case REGISTER_C:
				this->m_State.RGPC.Cast<T>() = Value;
				return;
			case REGISTER_IP:
				this->m_State.RIP.Cast<T>() = Value;
				return;
			case REGISTER_SP:
				this->m_State.RSP.Cast<T>() = Value;
				return;
			}

			THROW_ERROR("Writing invalid register.");
		}

		template <typename T>
		constexpr T ReadRegister(
			IN EProcessorRegister Register
		)
		{
			switch (Register)
			{
			case REGISTER_RV:
				return this->m_State.RV.Cast<T>();
			case REGISTER_A:
				return this->m_State.RGPA.Cast<T>();
			case REGISTER_B:
				return this->m_State.RGPB.Cast<T>();
			case REGISTER_C:
				return this->m_State.RGPC.Cast<T>();
			case REGISTER_IP:
				return this->m_State.RIP.Cast<T>();
			case REGISTER_SP:
				return this->m_State.RSP.Cast<T>();
			}
			THROW_ERROR("Reading invalid register.");
			return T{};
		}

		void CopyRegister(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		)
		{
			memcpy(GetRegister(TargetRegister).m_Value, GetRegister(SourceRegister).m_Value, sizeof(int64_t));
		}

		ProcessorRegister_t& GetRegister(
			IN EProcessorRegister Register
		)
		{
			switch (Register)
			{
			case REGISTER_RV:
				return this->m_State.RV;
			case REGISTER_A:
				return this->m_State.RGPA;
			case REGISTER_B:
				return this->m_State.RGPB;
			case REGISTER_C:
				return this->m_State.RGPC;
			case REGISTER_IP:
				return this->m_State.RIP;
			case REGISTER_SP:
				return this->m_State.RSP;
			}

			THROW_ERROR("Getting invalid register.");
			return this->m_State.RV;
		}

		static constexpr Processor InitializeEmpty() noexcept(true)
		{
			return Processor();
		}

		static constexpr Processor InitializeWithBuffer(
			IN uint8_t* InstructionBuffer,
			IN size_t InstructionBufferSize
		)
		{
			Processor context;
			context.m_InstructionBuffer = InstructionBuffer;
			context.m_BufferSize = InstructionBufferSize;

			return context;
		}

		friend void Processor_Execute(IN Processor& Processor);
	};

	__forceinline void Processor_Execute(IN Processor& Context)
	{
		auto current_ip_offset = Context.ReadRegister<int64_t>(
			REGISTER_IP
		);

		bool should_run = true;
		while (should_run)
		{
			// Grab the current instruction
			InstructionBase_t* instruction = reinterpret_cast<InstructionBase_t*>(Context.m_InstructionBuffer + current_ip_offset);

			// Parse operands
			auto operands = GetInstructionOperands(instruction);

			// Check if this instruction halts the processor
			should_run = instruction->Opcode != INSTRUCTION_HALT;

			switch (instruction->Opcode)
			{
			case INSTRUCTION_ADD:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Add] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Add] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Add(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Add(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_SUB:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Sub] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Sub] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Sub(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Sub(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_MUL:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Mul] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Mul] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Mul(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Mul(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_DIV:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Div] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Div] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Div(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Div(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_XOR:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Xor] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Xor] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Xor(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Xor(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_HALT:
			{
				if (operands.size() < 1)
				{
					THROW_ERROR("[Halt] operands.size() < 1");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Halt] Operands[0]->Type != OPERAND_REGISTER");
				}

				Instructions::Halt(Context, Context.GetRegister(operands[0]->Register));
				break;
			}
			case INSTRUCTION_MOV:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Mov] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Mov] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Context.CopyRegister(operands[0]->Register, operands[1]->Register);
					break;
				case OPERAND_IMMEDIATE:
					Context.WriteRegister(operands[0]->Register, GetImmediateOperand<uint64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_AND:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[And] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[And] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::And(Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::And(Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_CMP:
			{
				if (operands.size() < 2)
				{
					THROW_ERROR("[Cmp] operands.size() < 2");
				}

				if (operands[0]->Type != OPERAND_REGISTER)
				{
					THROW_ERROR("[Cmp] Operands[0]->Type != OPERAND_REGISTER");
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					Instructions::Cmp(Context.m_State, Context.GetRegister(operands[0]->Register), Context.GetRegister(operands[1]->Register));
					break;
				case OPERAND_IMMEDIATE:
					Instructions::Cmp(Context.m_State, Context.GetRegister(operands[0]->Register), GetImmediateOperand<int64_t>(instruction, 1));
					break;
				}

				break;
			}
			case INSTRUCTION_MEMCPY:
			{
				if (operands.size() < 3)
				{
					THROW_ERROR("[MemCpy] operands.size() < 3");
				}

				uintptr_t target_address = 0;
				uintptr_t source_address = 0;
				int64_t size = 0;

				switch (operands[0]->Type)
				{
				case OPERAND_REGISTER:
					target_address = Context.GetRegister(operands[0]->Register).Cast<uintptr_t>();
					break;
				case OPERAND_IMMEDIATE:
					target_address = GetImmediateOperand<uintptr_t>(instruction, 0);
					break;
				}

				switch (operands[1]->Type)
				{
				case OPERAND_REGISTER:
					target_address = Context.GetRegister(operands[1]->Register).Cast<uintptr_t>();
					break;
				case OPERAND_IMMEDIATE:
					target_address = GetImmediateOperand<uintptr_t>(instruction, 1);
					break;
				}

				switch (operands[2]->Type)
				{
				case OPERAND_REGISTER:
					size = Context.GetRegister(operands[2]->Register).Cast<int64_t>();
					break;
				case OPERAND_IMMEDIATE:
					size = GetImmediateOperand<int64_t>(instruction, 2);
					break;
				}

				std::memmove(std::bit_cast<void*>(target_address), std::bit_cast<void*>(source_address), size);

				break;
			}
			case INSTRUCTION_JMP:
			{
				if (operands.size() < 1)
				{
					THROW_ERROR("[Jmp] operands.size() < 1");
				}

				switch (operands[0]->Type)
				{
				case OPERAND_REGISTER:
					current_ip_offset += (Context.GetRegister(operands[0]->Register).Cast<int64_t>() - instruction->InstructionLength);
					break;
				case OPERAND_IMMEDIATE:
					current_ip_offset += (GetImmediateOperand<int64_t>(instruction, 0) - instruction->InstructionLength);
					break;
				}

				break;
			}
			case INSTRUCTION_JZ:
			{
				if (operands.size() < 1)
				{
					THROW_ERROR("[Jz] operands.size() < 1");
				}

				if (Context.m_State.ZF)
				{
					switch (operands[0]->Type)
					{
					case OPERAND_REGISTER:
						current_ip_offset += (Context.GetRegister(operands[0]->Register).Cast<int64_t>() - instruction->InstructionLength);
						break;
					case OPERAND_IMMEDIATE:
						current_ip_offset += (GetImmediateOperand<int64_t>(instruction, 0) - instruction->InstructionLength);
						break;
					}
				}

				break;
			}
			case INSTRUCTION_JNZ:
			{
				if (operands.size() < 1)
				{
					THROW_ERROR("[Jnz] operands.size() < 1");
				}

				if (!Context.m_State.ZF)
				{
					switch (operands[0]->Type)
					{
					case OPERAND_REGISTER:
						current_ip_offset += (Context.GetRegister(operands[0]->Register).Cast<int64_t>() - instruction->InstructionLength);
						break;
					case OPERAND_IMMEDIATE:
						current_ip_offset += (GetImmediateOperand<int64_t>(instruction, 0) - instruction->InstructionLength);
						break;
					}
				}

				break;
			}
			default:
			{
				THROW_ERROR("[ExecutionEngine] Unknown opcode!");
			}
			}

			// Go to the next instruction
			current_ip_offset += instruction->InstructionLength;
		}
	}

	class ProcessorAsm
	{
	private:
		std::vector<uint8_t> m_AssembledBytes;
		std::map<std::string, size_t> m_Labels;

		InstructionBase_t CreateOpcode(
			IN EOpcode Opcode,
			IN uint16_t NumberOfOperands
		);

		InstructionOperand_t CreateOperand(
			IN EProcessorRegister Register
		);

		template <typename T>
		InstructionOperand_t CreateOperand(
			IN const T& Object
		)
		{
			InstructionOperand_t operand{};
			operand.Type = OPERAND_IMMEDIATE;
			operand.OperandSize = sizeof(Object) - 1;

			return operand;
		}

		void FixInstructionSize(
			IN InstructionBase_t* Instruction,
			IN const std::vector<InstructionOperand_t>& Operands
		);

		template <typename T>
		void Serialize(
			const T& Object
		)
		{
			uint8_t* object = std::bit_cast<uint8_t*>(&Object);
			for (size_t i = 0; i < sizeof(Object); i++)
				m_AssembledBytes.push_back(object[i]);
		}

	public:
		void EmitAdd(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitAdd(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitSub(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitSub(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitMul(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitMul(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitDiv(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitDiv(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitXor(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitXor(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitHalt(
			IN EProcessorRegister SourceRegister
		);

		void EmitMov(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitMov(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitAnd(
			IN EProcessorRegister TargetRegister,
			IN EProcessorRegister SourceRegister
		);

		void EmitAnd(
			IN EProcessorRegister TargetRegister,
			IN int64_t SourceValue
		);

		void EmitJmp(
			IN EProcessorRegister TargetRegister
		);

		void EmitJmp(
			IN const char* Label
		);

		void EmitJz(
			IN EProcessorRegister TargetRegister
		);

		void EmitJz(
			IN const char* Label
		);

		void EmitJnz(
			IN EProcessorRegister TargetRegister
		);

		void EmitJnz(
			IN const char* Label
		);

		void CreateLabel(std::string_view Name);
		size_t GetCurrentPosition();
		uint8_t* GetBuffer();
		void Clear();
	};
}

