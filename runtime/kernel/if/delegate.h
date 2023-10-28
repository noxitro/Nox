///	@file	delegate.h
///	@brief	delegate
#pragma once

#if false
namespace nox
{
	template<class _NativeFunctionType>
	class Delegate
	{
	public:
		using FuncType = std::decay_t<_NativeFunctionType>;


	public:
		template<class... _Args>
		inline	void	Invoke(const _Args&... args)const
		{
			for (const Function<FuncType>& handle : handle_list_)
			{
				handle.Invoke(args...);
			}
		}

		template<class _R, class... _Args>
			requires(!std::is_void_v<FunctionReturnType<FuncType>>&& std::is_same_v<_R, FunctionReturnType<FuncType>>)
		inline	bool	Invoke(_R& result, const _Args&... args)const
		{
			bool is_dirty = false;

			for (const Function<FuncType>& handle : handle_list_)
			{
				if (is_dirty == true)
				{
					handle.Invoke(args...);
				}
				else
				{
					result = handle.Invoke(args...);
					is_dirty = true;
				}
			}

			return is_dirty;
		}


#pragma region 関数の登録/登録解除
		template<concepts::MemberFunctionPointer _Func, concepts::ClassUnion _InstanceType>
		inline	void	Add(_Func func, _InstanceType& instance)
		{
			handle_list_.emplace_back(Function<_NativeFunctionType>(func, instance));
		}

		template<concepts::MemberFunctionPointer _Func, concepts::ClassUnion _InstanceType>
		inline	void	Remove(_Func func, _InstanceType& instance)
		{
			std::erase_if(handle_list_, [func, &instance](const Function<FuncType>& handle)noexcept {
				return handle.Equal(func, instance);
				});
		}
#pragma endregion
	private:
		inline	void	Unregister(const std::type_info& typeinfo, const void* instance_addr = nullptr)noexcept
		{

		}

	private:
		Vector<Function<FuncType>> handle_list_;
	};
}
#endif // false
