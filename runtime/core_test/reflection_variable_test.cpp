//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	reflection_variable_test.cpp
///	@brief	nox::reflection::VariableInfo の getter / setter の回帰テスト。
///	@details	検証対象は「生成コードが書き出した getter / setter」と
///				「VariableInfo 側の引数チェック」の両方。
///				対象の型は core/test/reflection_variable_test_types.h に置いてある
///				(リフレクション生成器の解析対象に載せるための既存の作法)。

#include	"pch.h"

#include	"../kernel/kernel.h"
#include	"../reflection/reflection.h"
#include	"../core/test/reflection_variable_test_types.h"

namespace
{
	using nox::test::reflection::NonAssignableValue;
	using nox::test::reflection::ReferenceMemberTarget;
	using nox::test::reflection::VariableAccessKind;
	using nox::test::reflection::VariableAccessTarget;

	/// @brief 名前で変数情報を引く。ClassInfo に名前引きの API が無いので線形に探す。
	const nox::reflection::VariableInfo* FindVariable(
		const nox::reflection::ClassInfo& class_info,
		const std::u8string_view name)noexcept
	{
		for (const std::reference_wrapper<const nox::reflection::VariableInfo>& variable : class_info.GetVariableList())
		{
			if (variable.get().GetName() == name)
			{
				return &variable.get();
			}
		}
		return nullptr;
	}

	/// @brief テスト対象のクラス情報。登録されていなければテスト側で落とす。
	const nox::reflection::ClassInfo& GetTargetClassInfo()
	{
		const nox::reflection::ClassInfo* const class_info =
			nox::reflection::FindClassInfo(nox::reflection::Typeof<VariableAccessTarget>());
		EXPECT_NE(class_info, nullptr) << "VariableAccessTarget のリフレクション情報が登録されていない";
		return *class_info;
	}
}

//	=========================================================================
//	生成コード / VariableInfo が「そもそも登録されているか」
//	=========================================================================
TEST(ReflectionVariableTest, TargetTypeIsRegistered)
{
	const nox::reflection::ClassInfo& class_info = GetTargetClassInfo();

	//	plain_value / atomic_flag / const_value / non_assignable /
	//	bit_flag / bit_kind / static_value の 7 本
	EXPECT_EQ(class_info.GetVariableList().size(), 7u);

	EXPECT_NE(FindVariable(class_info, u8"plain_value"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"atomic_flag"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"const_value"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"non_assignable"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"bit_flag"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"bit_kind"), nullptr);
	EXPECT_NE(FindVariable(class_info, u8"static_value"), nullptr);
}

//	=========================================================================
//	getter
//	=========================================================================

///	@brief	左辺値のインスタンスを渡したメンバ getter が実際に値を返すこと。
///	@note	このテストは VariableInfo::TryGetValue の
///			「転送参照で推論した T& を Typeof して containing_type_ と比べていた」
///			バグの回帰ゲートを兼ねる。バグがあると常に nullopt が返る。
TEST(ReflectionVariableTest, MemberGetterReturnsValueForLValueInstance)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.plain_value = 42;

	const nox::reflection::ReflectionOptional<nox::int32> value = variable->TryGetValue<nox::int32>(target);
	ASSERT_TRUE(value.has_value()) << "左辺値インスタンスからメンバの値が取れていない";
	EXPECT_EQ(value.value(), 42);
}

///	@brief	const な左辺値でも読めること (修飾を剥がして判定していること)。
TEST(ReflectionVariableTest, MemberGetterReturnsValueForConstLValueInstance)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget mutable_target{};
	mutable_target.plain_value = 24;
	const VariableAccessTarget& target = mutable_target;

	const nox::reflection::ReflectionOptional<nox::int32> value = variable->TryGetValue<nox::int32>(target);
	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(value.value(), 24);
}

///	@brief	関係の無い型のインスタンスを渡したら nullopt であること。
///	@details	上の修正で判定を緩めすぎていないことのゲート。
TEST(ReflectionVariableTest, MemberGetterRejectsForeignInstanceType)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	nox::int32 foreign = 0;
	EXPECT_FALSE(variable->TryGetValue<nox::int32>(foreign).has_value());
}

///	@brief	コピー不可の型のメンバ getter は nullopt を返すこと。
///	@details	生成コードの nox::reflection::detail::ReflectionMakeOptional の
///				if constexpr がここを分岐している。
TEST(ReflectionVariableTest, NonCopyableMemberGetterReturnsNullopt)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"atomic_flag");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.atomic_flag.store(true, std::memory_order_relaxed);

	EXPECT_FALSE(variable->TryGetValue<std::atomic_bool>(target).has_value());
}

///	@brief	ビットフィールド (幅 1 / 幅 4) の getter が値を返すこと。
TEST(ReflectionVariableTest, BitFieldGetterReturnsValue)
{
	const nox::reflection::ClassInfo& class_info = GetTargetClassInfo();
	const nox::reflection::VariableInfo* const bit_flag = FindVariable(class_info, u8"bit_flag");
	const nox::reflection::VariableInfo* const bit_kind = FindVariable(class_info, u8"bit_kind");
	ASSERT_NE(bit_flag, nullptr);
	ASSERT_NE(bit_kind, nullptr);

	//	生成器がビット幅を拾えていること
	EXPECT_EQ(bit_flag->GetBitWidth(), 1);
	EXPECT_EQ(bit_kind->GetBitWidth(), 4);

	VariableAccessTarget target{};
	target.bit_flag = 1u;
	target.bit_kind = VariableAccessKind::Last;

	const nox::reflection::ReflectionOptional<nox::uint32> flag_value = bit_flag->TryGetValue<nox::uint32>(target);
	ASSERT_TRUE(flag_value.has_value());
	EXPECT_EQ(flag_value.value(), 1u);

	const nox::reflection::ReflectionOptional<VariableAccessKind> kind_value = bit_kind->TryGetValue<VariableAccessKind>(target);
	ASSERT_TRUE(kind_value.has_value());
	EXPECT_EQ(kind_value.value(), VariableAccessKind::Last);
}

///	@brief	参照メンバの getter が参照先の値を返すこと。
TEST(ReflectionVariableTest, ReferenceMemberGetterReturnsValue)
{
	const nox::reflection::ClassInfo* const class_info =
		nox::reflection::FindClassInfo(nox::reflection::Typeof<ReferenceMemberTarget>());
	ASSERT_NE(class_info, nullptr);

	const nox::reflection::VariableInfo* const variable = FindVariable(*class_info, u8"reference_value");
	ASSERT_NE(variable, nullptr);

	nox::int32 referenced = 123;
	ReferenceMemberTarget target{ .reference_value = referenced };

	const nox::reflection::ReflectionOptional<nox::int32&> value = variable->TryGetValue<nox::int32&>(target);
	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(value.value().get(), 123);
}

///	@brief	静的メンバの getter がインスタンス無しで値を返すこと。
TEST(ReflectionVariableTest, StaticVariableGetterReturnsValue)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"static_value");
	ASSERT_NE(variable, nullptr);
	ASSERT_TRUE(variable->IsStatic());

	VariableAccessTarget::static_value = 555;

	const nox::reflection::ReflectionOptional<nox::int32> value = variable->TryGetValue<nox::int32>();
	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(value.value(), 555);
}

///	@brief	静的メンバにインスタンス版の getter を使ったら nullopt であること。
TEST(ReflectionVariableTest, StaticVariableRejectsMemberGetter)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"static_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	EXPECT_FALSE(variable->TryGetValue<nox::int32>(target).has_value());
}

///	@brief	メンバ変数にインスタンス無しの getter を使ったら nullopt であること。
TEST(ReflectionVariableTest, MemberVariableRejectsStaticGetter)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	EXPECT_FALSE(variable->TryGetValue<nox::int32>().has_value());
}

//	=========================================================================
//	setter
//	=========================================================================

///	@brief	代入可能なメンバの setter が実際に書き込むこと。
TEST(ReflectionVariableTest, MemberSetterWritesValue)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.plain_value = 1;

	//	右辺値で渡した場合
	EXPECT_TRUE(variable->TrySetValue(target, nox::int32{ 77 }));
	EXPECT_EQ(target.plain_value, 77);

	//	左辺値で渡した場合 (こちらも転送参照で T& に推論される)
	nox::int32 assigned = 88;
	EXPECT_TRUE(variable->TrySetValue(target, assigned));
	EXPECT_EQ(target.plain_value, 88);
}

///	@brief	ビットフィールドの setter が実際に書き込むこと。
TEST(ReflectionVariableTest, BitFieldSetterWritesValue)
{
	const nox::reflection::ClassInfo& class_info = GetTargetClassInfo();
	const nox::reflection::VariableInfo* const bit_flag = FindVariable(class_info, u8"bit_flag");
	const nox::reflection::VariableInfo* const bit_kind = FindVariable(class_info, u8"bit_kind");
	ASSERT_NE(bit_flag, nullptr);
	ASSERT_NE(bit_kind, nullptr);

	VariableAccessTarget target{};

	EXPECT_TRUE(bit_flag->TrySetValue(target, nox::uint32{ 1u }));
	EXPECT_EQ(target.bit_flag, 1u);

	EXPECT_TRUE(bit_kind->TrySetValue(target, VariableAccessKind::Second));
	EXPECT_EQ(target.bit_kind, VariableAccessKind::Second);

	//	隣のビットを壊していないこと
	EXPECT_EQ(target.bit_flag, 1u);
}

///	@brief	const メンバの setter は黙って無視されること (従来の挙動)。
TEST(ReflectionVariableTest, ConstMemberSetterIsIgnored)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"const_value");
	ASSERT_NE(variable, nullptr);
	EXPECT_TRUE(variable->IsReadOnly());

	VariableAccessTarget target{};
	EXPECT_FALSE(variable->TrySetValue(target, nox::int32{ 99 }));
	EXPECT_EQ(target.const_value, 7);
}

///	@brief	代入不可のメンバの setter は何も書き込まないこと (従来の挙動)。
///	@details	生成コードの nox::reflection::detail::ReflectionAssignFromVoid が
///				is_assignable で分岐して何もしない。VariableInfo 側は
///				setter が居るので true を返す (「無言で無視」)。
TEST(ReflectionVariableTest, NonAssignableMemberSetterIsSilentlyIgnored)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"non_assignable");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.non_assignable.value = 3;

	NonAssignableValue source{};
	source.value = 300;

	EXPECT_TRUE(variable->TrySetValue(target, source));
	EXPECT_EQ(target.non_assignable.value, 3) << "代入不可のメンバに書き込まれている";
}

///	@brief	静的メンバの setter が実際に書き込むこと。
TEST(ReflectionVariableTest, StaticVariableSetterWritesValue)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"static_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget::static_value = 0;
	EXPECT_TRUE(variable->TrySetValue(nox::int32{ 4321 }));
	EXPECT_EQ(VariableAccessTarget::static_value, 4321);
}

///	@brief	型の合わない値を渡した setter は失敗すること。
TEST(ReflectionVariableTest, MemberSetterRejectsMismatchedValueType)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.plain_value = 5;

	EXPECT_FALSE(variable->TrySetValue(target, nox::int64{ 9 }));
	EXPECT_EQ(target.plain_value, 5);
}

///	@brief	関係の無い型のインスタンスを渡した setter は失敗すること。
TEST(ReflectionVariableTest, MemberSetterRejectsForeignInstanceType)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	nox::int32 foreign = 0;
	EXPECT_FALSE(variable->TrySetValue(foreign, nox::int32{ 1 }));
}

//	=========================================================================
//	アドレス取得
//	=========================================================================

///	@brief	メンバ変数のアドレスがインスタンス基準で取れること。
TEST(ReflectionVariableTest, MemberAddressPointsToTheField)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"plain_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	target.plain_value = 31;

	void* const address = variable->TryGetValueAddress(target);
	ASSERT_NE(address, nullptr);
	EXPECT_EQ(address, static_cast<void*>(&target.plain_value));
	EXPECT_EQ(*static_cast<nox::int32*>(address), 31);
}

///	@brief	アドレス getter を持たないメンバ (ビットフィールド) は nullptr であること。
TEST(ReflectionVariableTest, BitFieldHasNoAddress)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"bit_flag");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	EXPECT_EQ(variable->TryGetValueAddress(target), nullptr);
}

///	@brief	静的変数にインスタンス版のアドレス取得を使ったら nullptr であること。
///	@details	静的変数のときに union が保持しているのは引数 0 個の
///				getter_address_global_func_ なので、
///				インスタンス付きで呼び出してはならない。
TEST(ReflectionVariableTest, StaticVariableRejectsMemberAddress)
{
	const nox::reflection::VariableInfo* const variable = FindVariable(GetTargetClassInfo(), u8"static_value");
	ASSERT_NE(variable, nullptr);

	VariableAccessTarget target{};
	EXPECT_EQ(variable->TryGetValueAddress(target), nullptr);
}
