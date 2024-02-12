#pragma once
#include	"advanced_type.h"
#include	"unicode_converter.h"
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


		protected:
			
		};
	}

	/// @brief String
	class String : public nox::detail::StringBase<nox::U32String>
	{
	public:
		inline String() :
			string_() {}

		inline String(const String& other) :
			string_(other.string_) {}

		inline not_null<const void*> Data()const noexcept { return string_.data(); }
		inline not_null<const value_type*> CStr()const noexcept { return string_.c_str(); }

		inline	void	ShrinkToFit() { string_.shrink_to_fit(); }


#pragma region iterator
		inline	string_type::iterator begin()noexcept { return string_.begin(); }
		inline	string_type::iterator end()noexcept { return string_.end(); }

		inline	string_type::const_iterator begin()const noexcept { return string_.begin(); }
		inline	string_type::const_iterator end()const noexcept { return string_.end(); }
#pragma endregion
#pragma region 変換
		inline nox::NString ToCString()const
		{
			return unicode::ConvertNString(string_);
		}

		inline nox::WString ToWString()const
		{
			return unicode::ConvertWString(string_);
		}
#pragma endregion
	private:
		string_type string_;
	};
}