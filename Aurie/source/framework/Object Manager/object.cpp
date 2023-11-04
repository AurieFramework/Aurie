#include "object.hpp"

namespace Aurie
{
	AurieObjectType Aurie::ObGetObjectType(
		IN AurieObject* Object
	)
	{
		return Object->GetObjectType();
	}
}