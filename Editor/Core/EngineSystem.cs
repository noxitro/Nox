using System;
using System.Collections.Generic;

namespace Core
{
    public enum SystemPhaseType
    {
        Init,
        Start,
        Terminate,
    }

    public abstract class SystemPhase
    {
        public string Name { get; }
        public SystemPhaseType PhaseType { get; }

        protected SystemPhase(string name, SystemPhaseType phaseType)
        {
            Name = name;
            PhaseType = phaseType;
        }
    }

    public abstract class SystemPhaseInit : SystemPhase
    {
        protected SystemPhaseInit(string name) : base(name, SystemPhaseType.Init)
        {
        }
    }

    public sealed class SystemPhaseInit<T> : SystemPhaseInit where T : Core.EngineSystem
    {
        private readonly Action<T> _Proc;

        public SystemPhaseInit(string name, Action<T> proc)
            : base($"{typeof(T).Name}.{name}")
        {
            _Proc = proc;
        }

        public Action CreateProc(T engineSystem)
        {
            return () => _Proc(engineSystem);
        }
    }

    public abstract class SystemPhaseTerminate : SystemPhase
    {
        protected SystemPhaseTerminate(string name) : base(name, SystemPhaseType.Terminate)
        {
        }
    }

    public sealed class SystemPhaseTerminate<T> : SystemPhaseTerminate where T : Core.EngineSystem
    {
        private readonly Action<T> _Proc;

        public SystemPhaseTerminate(string name, Action<T> proc)
            : base($"{typeof(T).Name}.{name}")
        {
            _Proc = proc;
        }

        public Action CreateProc(T engineSystem)
        {
            return () => _Proc(engineSystem);
        }
    }

    public readonly struct PhaseRegister
    {
        public SystemPhase Phase { readonly get; init; }
        public Action Proc { readonly get; init; }

        /// <summary>
        /// Phases that must run after this phase.
        /// </summary>
        public SystemPhase[] Dependents { readonly get; init; }

        /// <summary>
        /// Phases that must run before this phase.
        /// </summary>
        public SystemPhase[] Dependencies { readonly get; init; }

        private static PhaseRegister CreateCore(SystemPhase phase, Action proc, SystemPhase[]? dependencies, SystemPhase[]? dependents)
        {
            return new PhaseRegister
            {
                Phase = phase,
                Proc = proc,
                Dependents = dependents ?? Array.Empty<SystemPhase>(),
                Dependencies = dependencies ?? Array.Empty<SystemPhase>(),
            };
        }

        public static PhaseRegister Create<T>(SystemPhaseInit<T> phase, T engineSystem, SystemPhase[]? dependencies = null, SystemPhase[]? dependents = null) where T : EngineSystem
        {
            return CreateCore(phase, phase.CreateProc(engineSystem), dependencies, dependents);
        }

        public static PhaseRegister Create<T>(SystemPhaseTerminate<T> phase, T engineSystem, SystemPhase[]? dependencies = null, SystemPhase[]? dependents = null) where T : EngineSystem
        {
            return CreateCore(phase, phase.CreateProc(engineSystem), dependencies, dependents);
        }
    }

    public abstract class EngineSystem
    {
        public abstract PhaseRegister[] GetPhaseRegisterList();
    }
}
