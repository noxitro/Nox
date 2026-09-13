// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;

namespace Nox.Threading.Tasks;

public class Task
{
    public static System.Threading.Tasks.Task Run(Action action)
    {
        try
        {
            return System.Threading.Tasks.Task.Run(action);
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.ToString());
            throw;
        }
    }
}
