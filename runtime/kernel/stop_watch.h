//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	stop_watch.h
///	@brief	stop_watch
#pragma once
#include	<chrono>
#include	"basic_type.h"

namespace nox
{
	class TimeSpan
	{
	public:
		using rep = int64_t;
		using duration = std::chrono::nanoseconds;

		TimeSpan() noexcept : duration_(0) {}
		explicit TimeSpan(duration d) noexcept : duration_(d) {}
		TimeSpan(rep ticks) noexcept : duration_(ticks) {}

		// 静的生成
		static TimeSpan FromSeconds(double sec) noexcept { return TimeSpan(std::chrono::duration_cast<duration>(std::chrono::duration<double>(sec))); }
		static TimeSpan FromMilliseconds(double ms) noexcept { return TimeSpan(std::chrono::duration_cast<duration>(std::chrono::duration<double, std::milli>(ms))); }
		static TimeSpan FromMicroseconds(double us) noexcept { return TimeSpan(std::chrono::duration_cast<duration>(std::chrono::duration<double, std::micro>(us))); }
		static TimeSpan FromNanoseconds(rep ns) noexcept { return TimeSpan(duration(ns)); }

		// 値取得
		double TotalSeconds() const noexcept { return std::chrono::duration_cast<std::chrono::duration<double>>(duration_).count(); }
		double TotalMilliseconds() const noexcept { return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(duration_).count(); }
		double TotalMicroseconds() const noexcept { return std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(duration_).count(); }
		rep TotalNanoseconds() const noexcept { return duration_.count(); }

		// 演算子
		TimeSpan operator+(const TimeSpan& rhs) const noexcept { return TimeSpan(duration_ + rhs.duration_); }
		TimeSpan operator-(const TimeSpan& rhs) const noexcept { return TimeSpan(duration_ - rhs.duration_); }
		TimeSpan& operator+=(const TimeSpan& rhs) noexcept { duration_ += rhs.duration_; return *this; }
		TimeSpan& operator-=(const TimeSpan& rhs) noexcept { duration_ -= rhs.duration_; return *this; }

		bool operator==(const TimeSpan& rhs) const noexcept { return duration_ == rhs.duration_; }
		bool operator!=(const TimeSpan& rhs) const noexcept { return duration_ != rhs.duration_; }
		bool operator<(const TimeSpan& rhs) const noexcept { return duration_ < rhs.duration_; }
		bool operator<=(const TimeSpan& rhs) const noexcept { return duration_ <= rhs.duration_; }
		bool operator>(const TimeSpan& rhs) const noexcept { return duration_ > rhs.duration_; }
		bool operator>=(const TimeSpan& rhs) const noexcept { return duration_ >= rhs.duration_; }

		// 内部duration取得
		duration GetDuration() const noexcept { return duration_; }

	private:
		duration duration_;
	};

	/// @brief ストップウォッチクラス
	class StopWatch
	{
	public:
		
		inline constexpr StopWatch() noexcept
			: 
			start_time_{},
			elapsed_(std::chrono::nanoseconds::zero()),
			running_(false)
		{
		}

		inline constexpr StopWatch(const StopWatch&)noexcept = delete;
		inline constexpr StopWatch(StopWatch&&)noexcept = default;

		inline void Start()noexcept
		{
			if (!running_)
			{
				start_time_ = std::chrono::high_resolution_clock::now();
				running_ = true;
			}
		}

		inline void Restart()noexcept
		{
			start_time_ = std::chrono::high_resolution_clock::now();
			elapsed_ = std::chrono::nanoseconds::zero();
		}

		inline constexpr void Stop()noexcept
		{
			if (running_)
			{
				elapsed_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start_time_);
				running_ = false;
			}
		}

		inline constexpr void Reset()noexcept
		{
			elapsed_ = std::chrono::nanoseconds::zero();
			running_ = false;
		}

		/// @brief 経過時間を取得（秒単位）
		inline constexpr float ElapsedSeconds() const noexcept
		{
			return std::chrono::duration_cast<std::chrono::duration<float>>(GetElapsed()).count();
		}

		/// @brief 経過時間を取得（ミリ秒単位）
		inline constexpr float ElapsedMilliseconds() const noexcept
		{
			return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GetElapsed()).count();
		}

		inline constexpr bool IsRunning() const noexcept { return running_; }
	private:
		inline constexpr std::chrono::nanoseconds GetElapsed() const noexcept
		{
			if (running_)
			{
				return elapsed_ + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start_time_);
			}
			else
			{
				return elapsed_;
			}
		}
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
		std::chrono::nanoseconds elapsed_;
		bool running_;
	};
}