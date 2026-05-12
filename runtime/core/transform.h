//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	transform.h
///	@brief	transform
#pragma once

#include	"component.h"
#include	"attribute_common.h"
#include	"attribute_dev_common.h"

namespace nox
{
	class Transform : public nox::Component
	{
		NOX_DECLARE_OBJECT(Transform, nox::Component);
	public:
		inline	Transform()noexcept :
			position_{},
			scale_(nox::Vec3::One()),
			rotation_(nox::Quat::Identity())
		{}

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	void	SetLocalPosition(const nox::Position& position)noexcept { position_ = position; }

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	const nox::Position& GetLocalPosition()const noexcept { return position_; }

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	void	SetLocalScale(const nox::Vec3& scale)noexcept { scale_ = scale; }

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	const nox::Vec3& GetLocalScale()const noexcept { return scale_; }

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	void	SetLocalRotation(const nox::Quat& rotation)noexcept { rotation_ = rotation; }

		NOX_ATTR_DECLARE(nox::attr::dev::Property())
		inline	constexpr	const nox::Quat& GetLocalRotation()const noexcept { return rotation_; }

		inline nox::Mat4 GetLocalMatrix()const noexcept
		{
			return {};
		}
	private:
		/// @brief		ローカル座標
		/// @details	広大なフィールドにも対応できるようにdouble型で保持する
		NOX_ATTR_DECLARE(nox::attr::DataMember(), nox::attr::dev::Hide())
		nox::Position position_;

		NOX_ATTR_DECLARE(nox::attr::DataMember(), nox::attr::dev::Hide())
		nox::Vec3 scale_;

		NOX_ATTR_DECLARE(nox::attr::DataMember(), nox::attr::dev::Hide())
		nox::Quat rotation_;

		///**
		// * @brief ワールド行列
		//*/
		//Mat4 world_matrix_;
	};
}
