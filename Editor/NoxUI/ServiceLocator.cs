// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using Microsoft.Extensions.DependencyInjection;

namespace NoxUI;

	/// <summary>
	/// アプリケーション全体の IServiceProvider への入口。
	/// App が起動時に <see cref="Current"/> を設定し、XAML から生成される View や
	/// 引数なしコンストラクタしか持てない場所 (デザイナ互換の ViewModel など) が
	/// ここから依存を解決する。通常のコンストラクタ注入で済む場所では使わないこと。
	/// </summary>
	public static class ServiceLocator
	{
		private const string NotReadyMessage = "ServiceLocator.Current が未設定。App の起動処理より前に依存解決が呼ばれている。";

		public static IServiceProvider? Current { get; set; }

		/// <summary>登録済みのサービスを取得する。未登録なら例外。</summary>
		public static T Resolve<T>() where T : notnull
		{
			IServiceProvider provider = Current ?? throw new InvalidOperationException(NotReadyMessage);
			return provider.GetRequiredService<T>();
		}

		/// <summary>
		/// 型を解決する。登録があればそれを、無ければコンストラクタ引数だけ注入して
		/// 新規生成する (Prism/Unity が未登録の具象型を暗黙に解決していた挙動の代わり)。
		/// </summary>
		public static object GetOrCreate(Type type)
		{
			IServiceProvider provider = Current ?? throw new InvalidOperationException(NotReadyMessage);
			return provider.GetService(type) ?? ActivatorUtilities.CreateInstance(provider, type);
		}
	}
