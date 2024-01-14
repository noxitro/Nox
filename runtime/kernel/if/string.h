#pragma once
#include	"advanced_type.h"

namespace nox
{
	namespace detail
	{
		template<class _StringType>
		class StringBase
		{
		public:
			using string_type = _StringType;
			using value_type = typename _StringType::value_type;

			inline StringBase() :
				string_() {}

			inline StringBase(const StringBase& other) :
				string_(other) {}

			nox::CString ToCString()const;
		private:
			_StringType string_;
		};
	}

	class String : nox::detail::StringBase<nox::U32String>
	{
	public:
		nox::CString ToCString()const;


	private:

	};
}