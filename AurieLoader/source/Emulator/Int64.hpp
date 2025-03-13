#include "VCPU.hpp"
#include <iostream>

// Example implementation of an obfuscated Int64 class.
class Int64
{
private:
	int64_t m_Value;
public:
	Int64() : m_Value(0) {}
	__forceinline Int64(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, Value);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);


		m_Value = processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	operator int64_t()
	{
		return m_Value;
	}

	__forceinline Int64 operator+(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, this->m_Value);
		assembler.EmitMov(REGISTER_B, Value);
		assembler.EmitAdd(REGISTER_A, REGISTER_B);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);


		return processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	__forceinline Int64 operator+(int32_t Value)
	{
		return operator+(static_cast<int64_t>(Value));
	}

	__forceinline Int64 operator-(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, this->m_Value);
		assembler.EmitMov(REGISTER_B, Value);
		assembler.EmitSub(REGISTER_A, REGISTER_B);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);


		return processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	__forceinline Int64 operator-(int32_t Value)
	{
		return operator-(static_cast<int64_t>(Value));
	}

	__forceinline Int64 operator*(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, this->m_Value);
		assembler.EmitMov(REGISTER_B, Value);
		assembler.EmitMul(REGISTER_A, REGISTER_B);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);

		return processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	__forceinline Int64 operator*(int32_t Value)
	{
		return operator*(static_cast<int64_t>(Value));
	}

	__forceinline Int64 operator/(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, this->m_Value);
		assembler.EmitMov(REGISTER_B, Value);
		assembler.EmitDiv(REGISTER_A, REGISTER_B);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);

		return processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	__forceinline Int64 operator/(int32_t Value)
	{
		return operator/(static_cast<int64_t>(Value));
	}

	Int64 operator&(int64_t Value)
	{
		using namespace Nisemono;

		ProcessorAsm assembler;
		assembler.EmitMov(REGISTER_A, this->m_Value);
		assembler.EmitMov(REGISTER_B, Value);
		assembler.EmitAnd(REGISTER_A, REGISTER_B);
		assembler.EmitHalt(REGISTER_A);

		Processor processor = Processor::InitializeWithBuffer(assembler.GetBuffer(), 0);
		Processor_Execute(processor);

		return processor.ReadRegister<int64_t>(REGISTER_RV);
	}

	Int64 operator&(int32_t Value)
	{
		return operator&(static_cast<int64_t>(Value));
	}

	friend std::istream& operator>>(std::istream& is, Int64& obj) {
		is >> obj.m_Value;
		return is;
	}

	friend std::ostream& operator<<(std::ostream& os, const Int64& obj) {
		os << obj.m_Value;
		return os;
	}
};
