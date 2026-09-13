// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI;

	public class ViewModelCommand : Prism.Commands.DelegateCommand
	{
		public ViewModelCommand(Action executeMethod) : base(executeMethod) { }

		public ViewModelCommand(Action executeMethod, Func<bool> canExecuteMethod) :
			base(executeMethod, canExecuteMethod)
		{ }
	}
