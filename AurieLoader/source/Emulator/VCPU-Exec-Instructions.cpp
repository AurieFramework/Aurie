#include "VCPU.hpp"

namespace Nisemono
{
	namespace Instructions
	{
		void Add(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() += Source.Cast<int64_t>();
		}
		void Add(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() += Immediate;
		}

		void Sub(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() -= Source.Cast<int64_t>();
		}

		void Sub(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() -= Immediate;
		}

		void Mul(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() *= Source.Cast<int64_t>();
		}
		void Mul(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() *= Immediate;
		}

		void Div(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() /= Source.Cast<int64_t>();
		}

		void Div(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() /= Immediate;
		}

		void Xor(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() ^= Source.Cast<int64_t>();
		}

		void Xor(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() ^= Immediate;
		}

		void Halt(
			IN Processor& Context, 
			IN ProcessorRegister_t& Target
		)
		{
			Context.WriteRegister(REGISTER_RV, Target.Cast<uint64_t>());
		}

		void And(
			IN ProcessorRegister_t& Target,
			IN const ProcessorRegister_t& Source
		)
		{
			Target.Cast<int64_t>() &= Source.Cast<int64_t>();
		}

		void And(
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			Target.Cast<int64_t>() &= Immediate;
		}

		void Cmp(
			IN ProcessorState_t& State, 
			IN ProcessorRegister_t& Target,
			IN int64_t Immediate
		)
		{
			State.ZF = Target.Cast<int64_t>() == Immediate;
		}

		void Cmp(
			IN ProcessorState_t& State,
			IN ProcessorRegister_t& Target, 
			IN const ProcessorRegister_t& Source
		)
		{
			State.ZF = Target.Cast<int64_t>() == Source.Cast<int64_t>();
		}
	}
}