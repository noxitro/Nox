//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	transform.h
///	@brief	transform
#pragma once

#include	"component.h"
#include	"attribute_common.h"

namespace nox
{
	class Transform : public nox::Component
	{
		NOX_DECLARE_MANAGED_OBJECT(Transform, nox::Component);
	public:
		inline	Transform()noexcept :
			position_(nox::Vec3::Zero()) ,
			scale_(nox::Vec3::Zero()),
			rotation_(nox::Quat::Identity())
		{}

		inline	constexpr	const nox::Vec3& GetLocalPosition()const noexcept { return position_; }
		inline	constexpr	const nox::Vec3& GetLocalScale()const noexcept { return scale_; }
		inline	constexpr	const nox::Quat& GetLocalRotation()const noexcept { return rotation_; }
	private:
		/**
		 * @brief ローカル座標(親からみた座標
		*/
		NOX_ATTR_DECLARATION(nox::attr::dev::DisplayName(u"LocalPosition"), nox::attr::DataMember())
		Vec3 position_;

		/**
		 * @brief ローカルスケール
		*/
		NOX_ATTR_DECLARATION(nox::attr::DataMember())
		Vec3 scale_;

		/**
		 * @brief ローカルローテーション
		*/
		NOX_ATTR_DECLARATION(nox::attr::DataMember())
		Quat rotation_;

		///**
		// * @brief ワールド行列
		//*/
		//Mat4 world_matrix_;
	};
}