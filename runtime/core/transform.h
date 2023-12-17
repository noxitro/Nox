//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	transform.h
///	@brief	transform
#pragma once

#include	"component.h"

namespace nox
{
	class Transform : public Component
	{
	public:
		inline	constexpr	Transform()noexcept :
			local_position_(vector::Vec3Zero) ,
			local_scale_(vector::Vec3Zero) 
		{}

	private:
		/**
		 * @brief ローカル座標(親からみた座標
		*/
		Vec3 local_position_;

		/**
		 * @brief ローカルスケール
		*/
		Vec3 local_scale_;

		///**
		// * @brief ローカルローテーション
		//*/
		//Quat local_rotation_;

		///**
		// * @brief ワールド行列
		//*/
		//Mat4 world_matrix_;
	};
}