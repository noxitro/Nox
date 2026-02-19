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
			position_(nox::Vec3d::Zero()) ,
			scale_(nox::Vec3::Zero()),
			rotation_(nox::Quat::Identity())
		{}

		NOX_ATTR_DECLARE(nox::attr::dev::PropertySetter())
		inline	constexpr	void	SetLocalPosition(const nox::Vec3d& position)noexcept { position_ = position; }

		NOX_ATTR_DECLARE(nox::attr::dev::PropertyGetter())
		inline	constexpr	const nox::Vec3d& GetLocalPosition()const noexcept { return position_; }
		inline	constexpr	const nox::Vec3& GetLocalScale()const noexcept { return scale_; }
		inline	constexpr	const nox::Quat& GetLocalRotation()const noexcept { return rotation_; }
	private:
		/// @brief		ローカル座標
		/// @details	広大なフィールドにも対応できるようにdouble型で保持する
		NOX_ATTR_DECLARE(nox::attr::DataMember(), nox::attr::dev::Hide())
		nox::Vec3d position_;

		/// @brief ローカルスケール
		NOX_ATTR_DECLARE(nox::attr::DataMember())
		nox::Vec3 scale_;

		/**
		 * @brief ローカルローテーション
		*/
		NOX_ATTR_DECLARE(nox::attr::DataMember())
		nox::Quat rotation_;

		///**
		// * @brief ワールド行列
		//*/
		//Mat4 world_matrix_;
	};
}